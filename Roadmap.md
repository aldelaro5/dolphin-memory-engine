This file explains the features that are planned so far in the future of the project.

# Features that MUST be done before the 1.0 release

* Memory viewer with real time updates (if possible, with little animation showing what bytes updated), must also provide in place editing.
* Make the UI good (fix every sizing issues, better space management, consider the possibility to have dockWidget, status bar or toolbar and figure out something about hding / showing big parts of the UI when the user wants it).
* Cheat Engine's CT file importer (need to figure out something about whether or not the table was stored using pointers or static addresses).

# Features that SHOULD be done before the 1.0 release (optional, but would preferably go in before)

* CSV exporter (this would greatly help people who wants to document their research and it would be possible with simple search and replace to have a TASVideos compatible table for ressources pages).
* Add more organisation data about the watches (such as color display).
* Add sorting support on the watch list.
* Add optional alignement option in the scanner (it has to be optional because the GameCube and Wii supports unaligned addresses, but as told in dolphin-dev IRC, it's typically not used, would make the results count much closer to Cheat Engine's defaults.

# Features that are in planning, but needs more analysis

* Ability to add "object" types and possibly manage arrays of these objects, like a typical debugger.  So far, the idea is to be able to add a group template that would, from a common base address, add predefined watches using the base address + a user defined offset.  Maybe could allow nesting so you could have an array of these objects, this is mainly usefull where a game has an array of specifically formatted and known data format.
* Consider the possiblity of having timer duration settings, currently, 10ms is alright for watch list update and freeze, but the result list was put to 100ms to reduce performaance drops within Dolphin, however, users with better hardware could decrease the timers without impact, not too hard to implement, but needx more testing and UI consideration
