# Roadmap

This file explains the features that are planned so far in the future of the project.


## Features to be implemented before 1.0:

* DONE - Memory viewer with real time updates
	* If possible, with a little animation showing what bytes updated
	* Must also provide in place editing.
* Greatly improve the UI.
	* DONE - Fix all sizing issues
	* DONE - Better manage UI space
	* Consider the possibility to have dockWidget, status bar or toolbar and figure out something about hiding/showing the larger parts of the UI as the user desires.
* Cheat Engine CT file importer
	* Need to figure out something about whether or not the table is stored using pointers or static addresses.


## Features that preferably be implemented before 1.0:

* DONE - CSV exporter
	* This would greatly help people who want to document their research. It would be possible with simple search and replace to have a TASVideos compatible table for resource pages.
* Add more organization data about watches, such as a color display.
* Add support for sorting on the watch list.
* Add an optional alignment setting in the scanner.
	* This must be optional as the GameCube and Wii support unaligned addresses, but as told in dolphin-dev IRC, it's typically not used, would make the results count much closer to Cheat Engine's defaults.


## Features in planning that need more analysis:

* Ability to add “object” types and possibly manage arrays of these objects, like a typical debugger.
	* So far, the idea is to be able to add a group template that would, from a common base address, add predefined watches using the base address + a user defined offset.  Maybe could allow nesting so you could have an array of these objects, this is mainly useful where a game has an array of specifically formatted and known data format.
* Consider the possibility of having timer duration settings.
	* Not too hard to implement, but needs more testing and UI consideration.
	* Currently, 10ms is alright for the watch list update and freeze, but the result list was put to 100ms to reduce performance drops within Dolphin. However, users with better hardware could decrease the timer interval without any impact.
