# WFS Designer #

WFS Designer is an application for performing wave field synthesis with large speaker arrays.  In contrast to other wave field synthesis applications that provide a top-down 2D view of the audio scene, WFS Designer allows positioning of virtual sources in a full three-dimensional space. Loudspeakers are often arranged in a horizontal line for WFS configurations. In order to position a virtual source in the vertical dimension, however, the loudspeaker array must be split into at least two rows at different heights.

<img src="http://www.mattmontag.com/media/2011/07/wfsdesigner4.png" width="500" />

WFS Designer is designed to be a cross-platform application, but has thus far been developed in Microsoft Visual Studio 2008 Professional and compiled for the Windows platform.

If you are running Windows and just want to try it out, it's packaged for [download here on Dropbox](https://www.dropbox.com/s/aqdh2weh4domrsa/WFSDesigner.zip). If that link goes bad, everything you need should be in the [/bin](https://github.com/mmontag/WFS-Designer/tree/master/bin) folder here.

## Dependencies ##

WFS Designer depends on Qt (4.7.1 or better), libsndfile, fftw, and portaudio, so you'll need those if you wish to compile it yourself.
Project files are included for VS 2008. These project files are setup with the dependencies in the following file structure:

    /fftw/
    /portaudio/
    /libsndfile/
    /WFSDesigner/[project files]
    
    C:/Qt/4.7.1/

So the dependencies are set up side by side with the main WFSDesigner project folder, and Qt is installed at C:/Qt/4.7.1. Of course, you can set up your own project files with the dependencies located wherever you see fit.

See more about WFS Designer at http://www.mattmontag.com/projects/wfs-designer.