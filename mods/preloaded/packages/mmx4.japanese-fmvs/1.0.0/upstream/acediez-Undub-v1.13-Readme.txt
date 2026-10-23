========================================================================================
Mega Man X4 Undub (v1.13) by acediez - 2021.08.09
========================================================================================

RHDB Project Thread: 

	http://www.romhacking.net/forum/index.php?topic=26255.0
	
Target file:

	Mega Man X4 (USA).bin
	MD5: 1C425B49BB25C45B3964B2D565DD0EC0
	http://redump.org/disc/7075/
	
The patch format is "xdelta", which can be applied using xdeltaUI:

	xdeltaUI in RHDN
	https://www.romhacking.net/utilities/598/
	
	
========================================================================================
Patch Variants
========================================================================================

UNDUB:

	The original version of the project. Doesn't include any changes to script or font.

	Video subtitles based on Hondoori's translation. Made specifically for the PS1 version
	(320x240), so the font size and style is optimized for a low resolution.


UNDUB + RETRANSLATION:

	Includes the script from NectarHime's relocalization of the PC version (Final Weaponproject)
	Additional tweaks to font and text spacing (variable font width) are exclusive to this release.

	Instead of the videos from the Final Weapon projects, it uses the videos from the
	original Undub version, as they're are optimized for the PS1 version.


========================================================================================
Additional Hacks
========================================================================================

This project doesn't make any adjustment to gameplay, but some smaller tweaks could
be added gere in the form of hex edits.

If you've never replaced hexadecimal data in a file before, here's some basic instructions:

1. 	Download: 	HxD (https://mh-nexus.de/en/hxd/)
				EDC/ECC recalculator (https://www.romhacking.net/utilities/1264/)
2. 	After patching, open the modified BIN in HxD
3. 	Press "Ctrl+G" to jump to an address. Use "hex" mode, set offset relative to "begin".
	Paste the addresses listed, and you should find the original values I'm providing.
4. 	Paste the modified hex data with "Ctrl+B" (while Ctrl+V is a "add-paste", Ctrl+B
	is a "overwrite-paste"). Make sure the hexadecimal side of the window is selected
	(the middle column), and the pointer is at the correct address (check the status bar
	at the bottom).
5. 	Repeat 3 and 4 for all the changes you want to make.
6. 	Save and exit.
7. 	Run the modified BIN through the EDC/ECC recalculator. This step is not necessary
	for certain emulators, but it is to play on original hardware and on the more
	accurate emulators.
	
When there's multiple entries listed, you need to modify all of them for the change to
work correctly.

Also note that these addresses will not match an unmodified Mega Man X4 BIN.


Exit Stage Button Always Available (8 Main Stages)
-----------------------------------------------------------

Modification #1:

BIN Address:
	Undub								20C220A0
	Undub+Retranslation					20C220A0

HEX Data:
	Original: 							03004010
	Modified: 							00000000

	
Modification #2:

BIN Address:
	Undub								20C3E4CC
	Undub+Retranslation					20C3E4CC

HEX Data:
	Original: 							03004010
	Modified: 							00000000
 

Disable Double Tap Dash Input
-----------------------------------------------------------

BIN Address:
	Undub								20C42534
	Undub+Retranslation					20C42534

HEX Data:
	Original: 							8800A280
	Modified: 							00000000
 
 
Change Default Button Configuration
-----------------------------------------------------------

BIN Address:
	Undub								20D18E10
	Undub+Retranslation					20D18E10	
	
						A	 B    C    D    E    F    G
HEX Data Original:		8000 1000 0200 4000 2000 0800 0400 

	Dash				O
	Giga Attack			R2
	Weapon Select L		L1
	Weapon Select R		L2

						A	 B    C    D    E    F    G	
HEX Data Example:		8000 1000 0800 4000 0400 0200 0100

	Dash				L1
	Giga Attack			R1
	Weapon Select L		L2
	Weapon Select R		R2
	
Reference:

	A = X-BUSTER, Z-SABER
	B = JUMP
	C = DASH
	D = SPECIAL WEAPONS, Z-BUSTER
	E = WEAPON SELECT R
	F = WEAPON SELECT L
	G = GIGA ATTACK

	8000 = Square
	1000 = X
	0200 = Circle
	4000 = Triangle
	0800 = L1
	0400 = L2
	0100 = R2
	0200 = R1

========================================================================================
Credits
========================================================================================

acediez: 		Project author.

DarkSamus993: 	ARC files and table of content format documentation.
				XA audio tables documentation
				Maverick voice clips restoration
				
LameGuy64: 		Author of mkpsxiso.

NectarHime: 	Author of the PC version's Final Weapon relocalization project.
				https://www.reddit.com/r/Megaman/comments/b8a4k9/final_weapon_a_retranslation_undub_and_musicsound/
