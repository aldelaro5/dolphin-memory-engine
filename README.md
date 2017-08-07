# Dolphin memory engine
A RAM search specifically made to search, track and edit at runtime the emulated memory of the Dolphin emulator (a Nintendo Wii and Nintendo GameCube emulator).  Its primary goal is to make research, TASing and reverse engineering of games more convenient and easier than the alternative solution, Cheat Engine.  Its name is derived from Cheat Engine to symbolize that.

It uses Qt5 as its GUI and was made with convenience in mind without disrupting the performance of the emulator.

For binary releases of this program, refer to the "release" tab on the Github repository page.

# System requirements
Any x64 based system should theoritically work, however, please note that macos is NOT supported at the moment. x86 based systems are unsupported simply because Dolphin no longer supports it since a while ago.

You absolutely need to have Dolphin running AND have an emulation started for this program to be of any use.  Because of this, your system needs to match Dolphin's own requirements which you can find [here](https://github.com/dolphin-emu/dolphin#system-requirements).  Additionally, have at least 250MB of free RAM especially when scanning with MEM2 enabled, it should only take this much when doing that.

On Linux, you need to install the Qt5 package of your respective distribution.

# How to build
## Windows
You need Microsoft Visual Studio 2017 since it has native support for CMake (I had a lot of problem to put a solution file so I might add it in the future for previous versions of Visual Studio).  You also need to install the Qt open source package which you can download the installer [here](http://download.qt.io/official_releases/qt/5.9/5.9.1/qt-opensource-windows-x86-5.9.1.exe). Then, simply open the folder "Source" in Visual Studio which should automatically configure the build.  Only use the x64 build configurations as x86 is not supported.

## Linux
You need to install CMake and Qt5, refer to your distribution for specific instructions on how to do that.  Then, run the following commands from the "Source" directory:

`mkdir build && cd build`

`cmake ..`

`make`

The binaries should be built in a directory named "build".

# General usage
First, open Dolphin and start a game, then run this program.  Make sure that the program reports that MEM2 is enabled for Wii games and disabled for GameCube games, MEM2 is an extra memory region exclusive to the Wii and this option affects valid watch addresses as well as where the scanner will look.  Auto detect will detect the right choice most of the time, but some rare cases won't work so simply use the toggle button if you think it got it wrong.  If the hooking process is successfull, the UI should be enabled, otherwise, you ahve to manually press the hook button before you can use this program.

Once hooked, you can do scans just like Cheat Engine as well as manage your watch list.  You can save and load your watch list to disk by using the file menu.  Note, the watch list files uses JSON internally so you can edit them manually with a text editor.

In case the program unhook itself from Dolphin, it means a read or write failed which should normally mean that Dolphin would have crashed or the emulation was stopped.  Start Dolphin and boot a game again to solve this, your watch list and scan aren't lost if this happens.

# License
This program is licensed under the MIT license which grants you the permission "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software" as long as you keep the copright notice in the license file.
