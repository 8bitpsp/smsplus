SMS Plus PSP
============

Sega Master System and Game Gear emulator

&copy; 2007-2010 Akop Karapetyan  
&copy; 1998-2004 Charles MacDonald

New Features
------------

#### Version 1.3.1 (October 16 2010)

*   Adds support for TMS9918-based display modes, making games like F16 Fighting Falcon and Rozetta no Shouzou playable
*   Saving changes to controls is now automatic (no longer need to press X to save)

Installation
------------

Unzip `smsppsp.zip` into `/PSP/GAME/` folder on the memory stick.

Game ROM’s can reside anywhere (the ROMS subdirectory is recommended, but not necessary). SMS+ PSP can load ROM’s from ZIP files.

Time Rewinding
--------------

Starting with version 1.2.55, SMS Plus PSP includes a “time rewind” feature, which allows you to rewind recent gameplay (approximately 20 seconds). Map ‘Special: Rewind’ to any PSP button to enable this feature.

Controls
--------

During emulation:

| PSP controls                    | Emulated controls            |
| ------------------------------- | ---------------------------- |
| Analog stick up/down/left/right | D-pad up/down/left/right     |
| D-pad up/down/left/right        | D-pad up/down/left/right     |
| [ ] (square)                    | Button 2                     |
| X (cross)                       | Button 1                     |
| [L] + [R]                       | Return to the emulator menu  |
| Start                           | Pause/Start                  |

By default, button configuration changes are not retained after button mapping is modified. To save changes, press X (cross) after desired mapping is configured. To load the default mapping press ^ (triangle).

Compiling
---------

To compile, ensure that [zlib](svn://svn.pspdev.org/psp/trunk/zlib) and [libpng](svn://svn.pspdev.org/psp/trunk/libpng) are installed, and run make:

`make -f Makefile.psp`

Version History
---------------

#### 1.2.55 (January 18 2009)

*   Time Rewind feature by DaveX – “rewind” recent gameplay (approximately 20 seconds). Map ‘Special: Rewind’ to any PSP button to enable this feature

#### 1.2.5 (December 08 2008)

*   When switching games, latest save state will be automatically highlighted
*   Rapid fire support – map any button to A or B autofire (‘Controls’ tab). Change the rate of autofire in the ‘Options’ tab
*   Snapshots are now saved to PSP’s PHOTO directory (`/PSP/PHOTO`), and can be viewed in PSP’s image viewer
*   File selector snapshots – while browsing for games with the file selector, pause momentarily to display the first snapshot for the game

#### 1.2.4 (September 11 2007)

*   Added animations to the menu UI
*   Fixed buffer overflow error affecting units without a battery

#### 1.2.3 (July 20 2007)

*   20% speedup in emulation speed with new 8-bit video rendering
*   Option to turn off FM emulation for an additional 48% improvement in emulation speed. This means that with FM emulation disabled, all games will play at 100% speed with VSync enabled
*   Various small bug fixes

#### 1.2.2 (July 14 2007)

*   Fixed a bug that caused subsequently loaded games to crash
*   Added an option to use a faster, less accurate sound emulation engine (improves performance by 3-4 fps)
*   Added an option to remove the leftmost vertical column for SMS games

#### 1.2.1 (July 11 2007)

*   Initial release

Credits
-------

Charles MacDonald (SMS Plus)  
Simon Tatham (fixed.fd font)  
Gilles Vollant (minizip)  
Ruka (PNG saving/loading)  
DaveX (time rewind)
