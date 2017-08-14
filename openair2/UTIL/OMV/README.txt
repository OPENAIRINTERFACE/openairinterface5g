OMV: openair mobility visualisor
To compile: 
1. install the required package: apt-get install qtcreator or go to http://qt.nokia.com/downloads to download the SDK 
   and install manually  
2. to generate the .pro used to setup the make files, run: qmake -project QT+=opengl  LIBS+=-lGLU
3. to generate the Makefile: qmake-qt4
4. to compile: make all

if the glut library is not installed, install it with: sudo apt-get install freeglut3 freeglut3-dev
For newer versions of Ubuntu (>= 11.10) you have to install another package: sudo apt-get install binutils-gold

To Test in standalone mode :
./OMV (for the moment, it only displays the window and default values for the area dimensions and node number)
