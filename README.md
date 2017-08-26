# Dolphin Memory Engine
A RAM search specifically made to search, track, and edit the emulated memory of [the Dolphin emulator](https://github.com/dolphin-emu/dolphin) at runtime. The primary goal is to make research, tool-assisted speedruns, and reverse engineering of GameCube & Wii games more convenient and easier than with the alternative solution, Cheat Engine. Its name is derived from Cheat Engine to symbolize that.

It uses Qt 5 for the GUI and was made with convenience in mind, without disrupting the performance of the emulation.

For binary releases of this program, refer to [the "releases" page](https://github.com/aldelaro5/Dolphin-memory-engine/releases) on [the Github repository](https://github.com/aldelaro5/Dolphin-memory-engine).


## System requirements
Any x86_64 based system should theoretically work, however, please note that Mac OS is currently _not_ supported. Additionally, 32-bit x86 based systems are unsupported as Dolphin does not support them either. (Support for them was dropped a while ago.)

You absolutely need to have Dolphin running ***and*** _have the emulation started_ for this program to be of any use. As such, your system needs to meet Dolphin's [system requirements](https://github.com/dolphin-emu/dolphin#system-requirements). Additionally, have at least 250 MB of memory free; especially when scanning with MEM2 enabled, as it should only take this much when doing that.

On Linux, you need to install the Qt 5 package of your respective distribution.


## How to Build
### Microsoft Windows
> *You will need Microsoft Visual Studio 2017 since it has native support for CMake (I had a lot of trouble trying put a solution file. It might be added in the future for previous versions of Visual Studio). You also need to install [the Qt open source](http://download.qt.io/official_releases/qt/5.9/5.9.1/qt-opensource-windows-x86-5.9.1.exe) package if you have not already done so.*

During the installation, you only need to check the component corresponding to your version of MSVC (for example, check msvc2017 x64 if you use Visual Studio 2017).  After the installation is done, make sure the environment variable ``CMAKE_PREFIX_PATH`` contains the path to Qt. (Which is installed to ``C:\Qt\<Qt version>\<MSVC version>`` by default.)

Then, simply open the ``Source`` folder in Visual Studio which should automatically configure the build.

>_Remember, only use the x64 build configurations as 32-bit x86 platforms are not supported by Dolphin._


### Linux
> _You will need CMake and Qt 5. Please refer to your distribution's documentation for specific instructions on how to install them if necessary._

To build for your system, simply run the following commands from the "Source" directory:

	mkdir build && cd build
	cmake ..
	make

The compiled binaries should be appear in the directory named “build”.


## General usage
First, open Dolphin and start a game, then run this program. Make sure that it reports that MEM2 is enabled for Wii games and disabled for GameCube games. The auto detection will usually make the right choice, but there are some rare cases that it will fail. In this case just simply use the toggle button to correct the setting.

>_MEM2 is an extra memory region exclusive to the Wii. This setting affects what are considered valid watch addresses as well as where the scanner will look._

If the hooking process is successful, the UI should be enabled, otherwise, you will need to click the hook button before the program can be of any use.

Once hooked, you can do scans just like Cheat Engine as well as manage your watch list. You can save and load your watch list to disk by using the file menu. Note, the watch list files uses JSON internally so you can edit them manually with a text editor.

If the program unhooks itself from Dolphin, it means a read/write failed which normally means that the emulation has stopped in some way. Just boot a game again to solve this; your watch list and scan will be retained if this happens.


## License
This program is licensed under the MIT license which grants you the permission to anything with the software as long as you preserve all copyright notices. (See the file LICENSE for the legal text.)
