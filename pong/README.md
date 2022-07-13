# Pong! Read Me

This project is less about the game of pong and more about game engine code design. While 
it's perfectly possible to throw together a pong implementation in a half day (or less), 
the result would most likey be throw away code. Most of the work has gone into creating the 
reusable game framework. 


## Default Controls:
ESC 		- Show Previous Menu Level / Hide Menu
Arrow Up 	- Menu Up / Player 2 paddle up
Arrow Down	- Menu Down / Player 2 paddle down
W			- Player 1 paddle up
s			- Player 1 paddle down
Return		- Select Menu Item


## Features:
- Single player versus AI game mode
- Cooperative game mode
- Configurable controls
- Configurable match scores
- Sound effects


## 3rd Party Libraries:
- OpenAL
- SDL (included)


## TODO:
- Add support for modifying game vars and binds in the menu system.
- key configuration and gamevar modifications aren't preserved across runs.
- non-coop multiplayer modes aren't implemented.


## Release Notes:

### New in version 0.1.4 (02-25-08)
Version bump for v3d libraries API sync.


### New in version 0.1.3 (01-17-08)
Rewrites in ui, font, audio, command library api's. Game menus are finally working again.


### New in version 0.1.2 (11-07-07)
Bugfixes - initialization bug broke initial game score.


### New in version 0.1.1 (11-02-07)
Separate classes for rendering - Scene graph classes no longer handle this function. New
callback system using boost signals library.


### New in version 0.1.0 (10-24-07)
Latest sync with Vertical3D/QuantumXML libraries


### New in version 0.0.9
No longer dependent on Xerces for xml support (using QuantumXML instead).


### New in Version 0.0.7 (12-17-06)
Only a few minor code cleanups. Any significant changes are within libhookah.


### New in Version 0.0.6 (12-16-06)

All of the non-pong specific code bits were moved into a separate generic engine library.
No other game changes exist in this build. The actual pong implementation has been reduced
to ~15kb of code within 9 source files (and 4 classes). The resulting engine library 
accounts for ~38kb in 15 classes within 28 sources files.
