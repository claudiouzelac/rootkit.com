Makefiles provided by Nate Lawson (nate@rootlabs.com) and Loris Degioanni

Import libraries for the cygwin compiler are in the lib directory (*.a)

To build, cd to the examples directory and type "make".  This will build
all the examples except netmeter.  Alternatively, you can type "make" in
each example you want to build.  To remove all objects and executables,
type "make clean".

TODO:
I didn't want to deal with the resource files for netmeter although cygwin
supports these.  Perhaps someone can build a makefile for the netmeter
example.
