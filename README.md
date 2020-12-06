Cyberpunk 1997
==============

Prerequisites
-------------

If you know what you're up to, skip ahead. Otherwise, here's a small shopping list of things we'll need:

  - Build tools (XCode, MSVC, etc)
  - Git
  - Make
  - GLEW
  - SDL2
  
Getting these will be different across most platforms, so we'll cover them individually.

__Windows__

Install [Git](https://git-scm.com/downloads) and [Make](http://gnuwin32.sourceforge.net/packages/make.htm).

You'll need to install [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/). The Community edition should be fine. When installing, select the _Desktop development with C++_ option.

Note: in order to use the compiler, [you must use a developer command prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160). Use whatever prompt is appropriate; I use the _x64_x86 Cross Tools Command Prompt_ because I am on a 64-bit (x64) machine and am compiling a 32-bit (x86) program.

Download the latest Windows binaries for [GLEW](http://glew.sourceforge.net/) and [SDL2](https://www.libsdl.org/download-2.0.php). Note that SDL2 supplies both _runtime_ and _development_ binaries. You'll want the _development_ binaries.

In order to install these libraries, you must first clone this repository. In an appropriate command prompt, enter:

	git clone https://github.com/gruntbatch/Cyberpunk1997.git

Extract the downloaded libraries and copy their contents to the `lib` folder of the repository. You should end up with something like this:

    Cyberpunk1997/lib/LIBRARIES.md
	Cyberpunk1997/lib/glew-2.1.0/...
	Cyberpunk1997/lib/SDL2-2.0.12/...

__macOS__

You'll need to install the Command Line Tools for XCode. This will also install a copy of Git and Make:

    xcode-select --install

Install [Homebrew](https://brew.sh/), and then use it to install GLEW and SDL2:

    brew install glew SDL2

Building
--------

In a command prompt, clone this repository (if you haven't already), and enter it:

	git clone https://github.com/gruntbatch/Cyberpunk1997.git
	cd Cyberpunk1997

By default, Make only builds the executable, and installs it in the `bin` directory. On Windows, this will also copy `glew.dll` and `SDL2.dll` to the `bin` directory.

    make

In order to process and package the assets, call:

	make assets

To both build the executable and process assets at the same time, simply call:

    make all

For funsies, you can build the executable, process assets, and launch the game using:

    make run

If you need to clean things up:

	make clean

Asset Pipeline
--------------

To build all of the assets, call:

    make assets

Any assets that can be committed to Git should be placed in the `assets` folder. These are mostly text files, such as shaders or scripts. The contents of this folder are committed to and shared via the Git repository.

Binary assets should go in the `assets_bin` filder. These files are shared using Google Drive or something better suited to handling binary assets. Files that should go here include basically everything that can't be edited in Notepad:

  - `.blend`
  - `.png`
  - `.psd`
  - etc.

For further documentation on asset file types, look at the README files in `assets` and `assets_bin`.
