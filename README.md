JonoF's TekWar Port
===================
by Jonathon Fowler

 * Email: jf@jonof.id.au
 * Source code: https://github.com/jonof/jftekwar

This is the source code for my port of [William Shatner's TekWar by Capstone Software](https://en.wikipedia.org/wiki/William_Shatner%27s_TekWar) using [my port of Ken Silverman's Build game engine](https://github.com/jonof/jfbuild), based on the code released by [Les Bird](https://lesbird.github.io/capstone/).

Minimum system requirements
---------------------------

* 32 or 64-bit CPU. These have been tried first-hand:
  * Intel x86, x86_64
  * PowerPC 32-bit (big-endian)
  * ARM 32-bit hard-float, 64-bit
* A modern operating system:
  * Linux, BSD, possibly other systems supported by [SDL 2.0](http://libsdl.org/).
  * macOS 10.15+
  * Windows Vista, 7, 8/10+
* Optional: 3D acceleration with OpenGL 2.0 or OpenGL ES 2.0 capable hardware.

You will require game data from an original release of TekWar, specifically:

 * All files with an `.art` extension.
 * All files with a `.map` extension.
 * `lookup.dat`
 * `nlookup.dat`
 * `songs`
 * `sounds`
 * All files with a `.smk` extension.

Use `palette.dat` from the _rsrc_ directory of this repository.

Compilation
-----------

Before you begin, clone this repository or unpack the source archive. If you cloned using
Git, be sure to initialise the submodules of this repository (i.e. `git submodule update --init`).

Now, based on your chosen OS and compiler:

### Linux and BSD

1. Install the compiler toolchain and SDL2 development packages, e.g.
   * Debian 9: `sudo apt-get install build-essential libsdl2-dev`
   * FreeBSD 11: `sudo pkg install gmake sdl2 pkgconf`
2. Install optional sound support development packages.
   * Debian 9: `sudo apt-get install libvorbis-dev libfluidsynth-dev`
   * FreeBSD 11: `sudo pkg install libvorbis fluidsynth`
3. Install GTK+ 3 development packages if you want launch windows and editor file choosers, e.g.
   * Debian 9: `sudo apt-get install libgtk-3-dev`
   * FreeBSD 11: `sudo pkg install gtk3`
4. Open a terminal, change into the source code directory, and compile the game with: `make` or `gmake` (BSD)
5. Assuming that was successful, run the game with: `./tekwar`

### macOS

1. [Install Xcode from the Mac App Store](https://itunes.apple.com/au/app/xcode/id497799835?mt=12).
2. Fetch and install the SDL 2.0 development package:
   1. Fetch _SDL2-2.0.x.dmg_ from http://libsdl.org/download-2.0.php.
   2. Copy _SDL2.framework_ found in the DMG file to `~/Library/Frameworks`. Create the
      _Frameworks_ directory if it doesn't exist on your system.
3. Open _tekwar.xcodeproj_ from within the JFTekWar source code's _xcode_ folder.
4. From the Product menu choose Run.

### Windows using Microsoft Visual C++ 2015 (or newer) and NMAKE

1. If needed, [install Visual Studio Community 2017 for free from
   Microsoft](https://docs.microsoft.com/en-us/visualstudio/install/install-visual-studio).
   Terms and conditions apply. Install at minimum these components:
   * VC++ 2015.3 v140 toolset for desktop (x86,x64)
   * Windows Universal CRT SDK
   * Windows 8.1 SDK
2. Open the command-line build prompt. e.g. _VS2015 x64 Native Tools Command Prompt_
   or _VS2015 x86 Native Tools Command Prompt_.
3. Change into the JFTekWar source code folder, then compile the game with: `nmake /f Makefile.msvc`
5. Assuming success, run the game with: `tekwar`

Compilation options
-------------------

Some engine features may be enabled or disabled at compile time. These can be passed
to the MAKE tool, or written to a Makefile.user (Makefile.msvcuser for MSVC) file in
the source directory.

These options are available:

 * `RELEASE=1` – build with optimisations for release.
 * `RELEASE=0` – build for debugging.
 * `USE_POLYMOST=1` – enable the true 3D renderer.
 * `USE_POLYMOST=0` – disable the true 3D renderer.
 * `USE_OPENGL=1` – enable use of OpenGL 2.x acceleration.
 * `USE_OPENGL=3` – enable use of OpenGL 3.x acceleration.
 * `USE_OPENGL=USE_GL2` – enable use of OpenGL 2.x acceleration. (Not a valid setting for MSVC.)
 * `USE_OPENGL=USE_GL3` – enable use of OpenGL 3.x acceleration. (Not a valid setting for MSVC.)
 * `USE_OPENGL=USE_GLES2` – enable use of OpenGL ES 2.0 acceleration. (Not a valid setting for MSVC.)
 * `USE_OPENGL=0` – disable use of OpenGL acceleration.
 * `WITHOUT_GTK=1` – disable use of GTK+ to provide launch windows and load/save file choosers.

Warnings
--------

1. Let's not talk about the licensing.


Enjoy!

Jonathon Fowler


