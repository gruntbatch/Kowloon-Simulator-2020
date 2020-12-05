Cyberpunk 1997
==============

Building
--------

This project relies on GLEW for OpenGL stuff and SDL2 for system stuff. You can build this project with Make.

Clone this repository, and enter it.

    git clone https://github.com/gruntbatch/Cyberpunk1997.git
	cd Cyberpunk1997

__macOS__

You'll need to install the Command Line Tools for XCode:

    xcode-select --install

Install GLEW and SDL2 using Homebrew:

    brew install glew SDL2
	
__Windows__

Windows doesn't come with Make, so you'll need to [install it manually](http://gnuwin32.sourceforge.net/packages/make.htm), or use chocolatey:

    choco install make

You'll need to install [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/). The Community edition should be fine. When installing, select the _Desktop development with C++_ option.

Download the latest Windows binaries for [GLEW](http://glew.sourceforge.net/) and [SDL2](https://www.libsdl.org/download-2.0.php). Note that SDL2 supplies both _runtime_ and _development_ binaries. You'll want the _development_ binaries. Extract your downloads and copy their contents to the `lib` folder. You should end up with something like this:

    Cyberpunk1997/lib/LIBRARIES.md
	Cyberpunk1997/lib/glew-2.1.0/...
	Cyberpunk1997/lib/SDL2-2.0.12/...
	
[Open a developer command prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160). It shouldn't matter which one, but I use the _x64_x86 Cross Tools Command Prompt_. Navigate to the Cyberpunk1997 directory.

__Building__

By default, Make only builds the executable, and installs it in the `bin` directory. On Windows, this will also copy `glew.dll` and SDL2.dll` to the `bin` directory.

    make

In order to process and package the assets, call:

	make assets

To both build the executable and process assets at the same time, simply call:

    make all
	
For funsies, you can build the executable, process assets, and launch the game using:

    make run
	
If you need to clean things up:

	make clean
