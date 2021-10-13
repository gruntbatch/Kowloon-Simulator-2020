Kowloon Simulator 2020
======================

[Download here.](https://gruntbatch.itch.io/kowloon-sumulator-2020)

This was made for a game jam. The code is janky. I don't recommend using it.

Building
--------

You'll need to install [Make for Windows](http://gnuwin32.sourceforge.net/packages/make.htm) and [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/).

Download the latest Windows binaries for [GLEW](http://glew.sourceforge.net/) and [SDL2](https://www.libsdl.org/download-2.0.php). Note that SDL2 supplies both _runtime_ and _development_ binaries. You'll want the _development_ binaries. Extract the downloaded libraries and copy their contents to the `lib` folder of the repository. You should end up with something like this:

    Kowloon/lib/LIBRARIES.md
	Kowloon/lib/glew-2.1.0/...
	Kowloon/lib/SDL2-2.0.12/...

Build the program by calling

    make exe
	
The executable can be found here:

    Kowloon/bin/Kowloon.exe
