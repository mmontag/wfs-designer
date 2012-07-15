#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

class Biquad
{
public:
	enum FilterType {
		LinkwitzRileyHighPass,
		LinkwitzRileyLowPass
	};
	int fType;
	float fs, theta, omega, kappa, delta;
	float a0, a1, a2, b1, b2, z1, z2, z3, z4;
	Biquad(float sampleRate, int filterType){
		fs = sampleRate;
		fType = filterType;
		/*
		Host must call these functions for proper behavior.
		*/
		//prepareForPlay();
		//updateFilterCoeffs(crossoverFrequency);
	};
	~Biquad(void){};

	void prepareForPlay(void){
		z1 = 0;
		z2 = 0;
		z3 = 0;
		z4 = 0;
	};
	void updateFilterCoeffs(float fc){
		switch(fType) {
			case(LinkwitzRileyLowPass): 
				/*
					2nd Order Linkwitz-Riley Low-pass Filter
				*/
				theta = 2*M_PI*fc/fs;
				omega = 2*M_PI*fc;
				kappa = omega/tan(theta);
				delta = kappa*kappa + omega*omega + 2*kappa*omega;
				a0 = omega*omega / delta;
				a1 = 2*omega*omega / delta;
				a2 = omega*omega / delta;
				b1 = (-2*kappa*kappa + 2*omega*omega) / delta;
				b2 = (-2*kappa*omega + kappa*kappa + omega*omega) / delta;
				break;
			case(LinkwitzRileyHighPass): 
				/*
					2nd Order Linkwitz-Riley High-pass Filter
				*/
				theta = 2*M_PI*fc/fs;
				omega = 2*M_PI*fc;
				kappa = omega/tan(theta);
				delta = kappa*kappa + omega*omega + 2*kappa*omega;
				a0 = kappa*kappa / delta;
				a1 = -2*kappa*kappa / delta;
				a2 = kappa*kappa / delta;
				b1 = (-2*kappa*kappa + 2*omega*omega) / delta;
				b2 = (-2*kappa*omega + kappa*kappa + omega*omega) / delta;
				break;
			default:
				; //we shouldn't get here...
		}
	};
	void process(float* in, float* out, int size) {
		for(int i = 0; i < size; i++) {

			out[i] = a0*in[i] + a1*z1 + a2*z2 - b1*z3 - b2*z4;

			// update delayed sample values
			z2 = z1;		// input 2 samples ago
			z1 = in[i];		// input 1 sample ago
			z4 = z3;		// output 2 samples ago
			z3 = out[i];		// output 1 sample ago
		}
	};
	void processReplacing(float* in, int size){
		float out = 0;
		for(int i = 0; i < size; i++) {

			out = a0*in[i] + a1*z1 + a2*z2 - b1*z3 - b2*z4;

			// update delayed sample values
			z2 = z1;		// input 2 samples ago
			z1 = in[i];		// input 1 sample ago
			z4 = z3;		// output 2 samples ago
			z3 = out;		// output 1 sample ago


			in[i] = out;
		}
	};
	float processSingle(float in) {
		float out = a0*in + a1*z1 + a2*z2 - b1*z3 - b2*z4;
		// update delayed sample values
		z2 = z1;		// input 2 samples ago
		z1 = in;		// input 1 sample ago
		z4 = z3;		// output 2 samples ago
		z3 = out;		// output 1 sample ago
		return out;
	};
};