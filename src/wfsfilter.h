#pragma once
#include <QDebug>
#include "sndfile.h"
#include "fftw3.h"

class WFSFilter
{
public:
	QString				fileName;
	SNDFILE*			sfFile;
	SF_INFO				sfInfo;
	int					buffersize;
	int					firsize;
	int					overflowsize;
	float*				m_FIR;
	float*				m_BUFFER;
	fftwf_complex*		m_FIR_FFT;
	fftwf_complex*		m_BUFFER_FFT;
	fftwf_plan			firPlan;
	fftwf_plan			fPlan;
	fftwf_plan			iPlan;
	float*				m_RESULT;
	float*				m_OVERFLOW;
	//float*				FREQGAINS; //TODO: make internal HIGHPASS/LOWPASS and CUTOFF FREQ methods
	int					oCursor;
	float				gain;

	WFSFilter(int bsize, int fsize)
	{
		buffersize = bsize;
		firsize = fsize;
		init();
	};
	~WFSFilter(void){
		//fftw_cleanup_threads();
	};
	void init() {
		/*
		temporary impulse response convolution test
		*/
		// initialize our data structure 
		sfInfo.format = 0;
		//buffersize = 65536; //1048576; //131072; //65536;
		oCursor = 0;

		/*

		Test m_FIR from file on disk... (part 1)

		fileName = QString("Debug\\church_ir.wav");
		// try opening the file, and send info to the sfInfo struct
		sfFile = sf_open(fileName.toLocal8Bit().data(), SFM_READ, &sfInfo); // convert QString to char*
		firsize = sfInfo.frames;

		*/

		overflowsize = firsize-1;

		m_FIR = new float[buffersize+firsize-1]; //zero pad
		m_BUFFER = new float[buffersize+firsize-1];
		m_OVERFLOW = new float[overflowsize];
		memset(m_FIR,0,(buffersize+firsize-1)*sizeof(float)); //fill zeros
		memset(m_BUFFER,0,(buffersize+firsize-1)*sizeof(float)); 
		memset(m_OVERFLOW,0,(overflowsize)*sizeof(float));
		m_FIR_FFT = new fftwf_complex[buffersize+firsize-1];
		m_BUFFER_FFT = new fftwf_complex[buffersize+firsize-1];
		m_RESULT = new float[buffersize+firsize-1];

		/*

		FREQGAINS = new float[buffersize+firsize-1];
		for(int i = 0; i < (buffersize+firsize-1); i++) {
			FREQGAINS[i] = 1.0;
		}

		//Temporary testing purposes: 
		for(int i = 185; i < (buffersize+firsize-1); i++) {
			FREQGAINS[i] = 0.0;
		}
		FREQGAINS[0] = 0.0;

		*/

		/*

		Test m_FIR from file on disk... (part 2)

		//read m_FIR from disk
		float* temp = new float[sfInfo.frames*sfInfo.channels]; //float buffer size of wave file
		int sfDidRead = sf_read_float(sfFile, temp, sfInfo.frames*sfInfo.channels);
		qDebug() << QString("[filter] libsndfile read %1 items for m_FIR.").arg(sfDidRead);

		//fill m_FIR with first channel data.
		for(int i = 0; i < firsize; i++) {
			m_FIR[i] = temp[i*sfInfo.channels] * 0.25;
		}
		delete temp;
		
		*/
		
		//enable multithreading
		//fftw_init_threads();
		//fftw_plan_with_nthreads(2);

		//create plans for this convolution length
		firPlan = fftwf_plan_dft_r2c_1d(firsize+buffersize-1, m_FIR, m_FIR_FFT, FFTW_ESTIMATE);
		fPlan = fftwf_plan_dft_r2c_1d(firsize+buffersize-1, m_BUFFER, m_BUFFER_FFT, FFTW_ESTIMATE);
		iPlan = fftwf_plan_dft_c2r_1d(firsize+buffersize-1, m_BUFFER_FFT, m_RESULT, FFTW_ESTIMATE);

		//perform one-time FFT on m_FIR
		fftwf_execute(firPlan);

	};
	/* 
	TODO: Make convolution work on variable m_BUFFER size (accepts size argument)
	and variable m_FIR size. This will require new FFTW plans when size changes.

	2/27/11
	buffer processing uses variable in/out buffer sizes but a fixed m_FIR size
	this has to change soon

	*/
	void process(float* in, float* out, int size) {
		//(The rest of m_BUFFER should never be touched)
		//memset(m_BUFFER,0,buffersize+firsize-1);

		//copy input to beginning of m_BUFFER, overwriting 
		memcpy(m_BUFFER, in, size*sizeof(float));													// PORTAUDIO m_BUFFER SIZE
		//perform forward FFT on m_BUFFER and store in m_BUFFER_FFT
		fftwf_execute(fPlan);

		//perform forward FFT on m_FIR and store in m_FIR_FFT
		fftwf_execute(firPlan);

		//perform high-pass/low-pass filtering by directly scaling FFT Magnitude 
		//Disabled because truncation causes clicking. Possibly fix by windowing with overlap.
		/* 
		Which bins to set to zero?
		From http://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html#Real_002ddata-DFT-Array-Format:
		"...we only store the lower half (non-negative frequencies), plus one element, 
		of the last dimension of the data from the ordinary complex transform."
		Therefore, FFTW frequency bins are intuitive: DC [][][]...[][][] Nyquist
		*/
		//for(int i = 185; i<(buffersize+firsize-1)-4000; i++) {
			//m_FIR_FFT[i][0] *= FREQGAINS[i]; //scale real component of m_BUFFER_FFT by FREQGAINS[i]
			//m_FIR_FFT[i][1] *= FREQGAINS[i];
		//}

		//complex multiply-in-place (convolution)
		complexMultInPlace(m_FIR_FFT, m_BUFFER_FFT, buffersize+firsize-1);

		//perform backward (inverse) FFT on m_BUFFER_FFT and store in m_RESULT
		fftwf_execute(iPlan);
		
		// Overlap-and-Add
		// 1. Copy the front of current frame's m_RESULT into outgoing buffer.
		memcpy(out, m_RESULT, size*sizeof(float));												// PORTAUDIO m_BUFFER SIZE

		// 2. Mix m_OVERFLOW from prior frames with outgoing buffer, incrementing the oCursor.
		for(int i = 0; i < size; i++) {															// PORTAUDIO m_BUFFER SIZE
			out[i] += m_OVERFLOW[oCursor];
			m_OVERFLOW[oCursor] = 0.0;
			oCursor = (oCursor + 1)%(overflowsize);
		}
		// 3. Mix the rest of m_RESULT with m_OVERFLOW buffer at the updated oCursor position.
		for(int j = 0; j < (firsize-1); j++) {
			m_OVERFLOW[(oCursor+j)%(overflowsize)] += m_RESULT[size+j];						// PORTAUDIO m_BUFFER SIZE
		}

		//For debugging/bypass...
		//memcpy(out, in, size*sizeof(float));												// PORTAUDIO m_BUFFER SIZE

	};

private:
	void complexMultInPlace(const fftwf_complex* buffer1, fftwf_complex* buffer2inplace, int size){

		//const float s = scale ? 1.0/(width*height) : 1.0;

		for(int i = 0; i < size; i++){

			const float a_re = buffer1[i][0];
			const float a_im = buffer1[i][1];

			const float b_re = buffer2inplace[i][0];
			const float b_im = buffer2inplace[i][1];

			/* 
			Scale array: Users should note that FFTW computes an unnormalized DFT. 
			Thus, computing a forward followed by a backward transform 
			(or vice versa) results in the original array scaled by n. 
			*/
			buffer2inplace[i][0] = (a_re*b_re - a_im*b_im)/(float)size;
			buffer2inplace[i][1] = (a_re*b_im + a_im*b_re)/(float)size;

		}
	};
};
