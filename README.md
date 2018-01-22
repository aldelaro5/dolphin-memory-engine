# Dolphin Memory Engine

![Screenshot](https://raw.githubusercontent.com/aldelaro5/Dolphin-memory-engine/master/Docs/screenshot.png)

A RAM search specifically designed to search, track, and edit the emulated memory of [the Dolphin emulator](https://github.com/dolphin-emu/dolphin) at runtime. The primary goal is to make research, tool-assisted speedruns, and reverse engineering of GameCube & Wii games more convenient and easier than with the alternative solution, Cheat Engine. Its name is derived from Cheat Engine to symbolize that.

The GUI has been designed with convenience in mind, without disrupting the performance of the emulation. Qt 5 is used to help accomplish this.

For binary releases of this program, refer to [the "releases" page](https://github.com/aldelaro5/Dolphin-memory-engine/releases) on [the Github repository](https://github.com/aldelaro5/Dolphin-memory-engine).


## System requirements
Any x86_64 based system should theoretically work, however, please note that Mac OS is currently _not_ supported. Additionally, 32-bit x86 based systems are unsupported as Dolphin does not support them either. (Support for them was dropped a while ago.)

You absolutely need to have Dolphin running ***and*** _have the emulation started_ for this program to be of any use. As such, your system needs to meet Dolphin's [system requirements](https://github.com/dolphin-emu/dolphin#system-requirements). Additionally, have at least 250 MB of memory free.

On Linux, you need to install the Qt 5 package(s) for your respective distribution.


## How to Build
### Microsoft Windows
> *You will need Microsoft Visual Studio 2015 or 2017 with Visual C++ installed. Previous versions may work but are untested. The solution works best with VS2017, but with a small adjustment, it is possible to have VS2015 work.*

Before proceeding, make sure you have initialized the Qt submodule by running the command `git submodule update --init` at the repository's root. The files should appear at the `Externals\Qt` directory.

Once this is done, open Visual Studio and open the solution located in the `Source` directory. If you are using Visual Studio 2017, this is all you need to do, simply select the build configuration and build the solution. If you are using Visual Studio 2015 however, you may have to change the toolset of the project to the one that comes with Visual Studio 2015. To do so, right click on the project from the Solution Explorer and click properties. From there, change the "Platform Toolset" to the one that you have installed. Please note that this will change the settings in the `vcxproj` file so if you plan to submit a Pull Request, make sure to not stage this change.


### Linux
> _You will need CMake and Qt 5. Please refer to your distribution's documentation for specific instructions on how to install them if necessary._

To build for your system, simply run the following commands from the `Source` directory:

	mkdir build && cd build
	cmake ..
	make

The compiled binaries should be appear in the directory named `build`.


## General usage
First, open Dolphin and start a game, then run this program. Make sure that it reports that the memory type matches the system being emulated; Wii for Wii games and GameCube for GameCube games.

>_Added as part of the enhancements from the earlier GameCube hardware, the Wii has an extra region of memory onboard. Consequently, the presence of this extra memory affects what is a valid watch address and therefore what regions of memory the scanner will look though._

If the hooking process is successful, the UI should be enabled, otherwise, you will need to click the hook button before the program can be of any use.

Once hooked, you can do scans just like Cheat Engine as well as manage your watch list. You can save and load your watch list to disk by using the file menu. The watch list files are in JSON and can be manually edited in the text editor of your choice.

If the program unhooks itself from Dolphin, it means a read/write operation failed, likely caused by the emulation halting in some way. Just boot a game again to solve this; your watch list and scan will be retained if this happens.

Finally, the program also includes a memory viewer which shows an hexadecimal view and an ASCII view of the memory. Simply click on the corresponding button or right click on a watch to browse the memory using the memory viewer.


## License
This program is licensed under the MIT license which grants you the permission to do  anything you wish to with the software, as long as you preserve all copyright notices. (See the file LICENSE for the legal text.)
