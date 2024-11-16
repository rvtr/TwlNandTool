# ![app icon](icon.bmp) TwlNandTool

This is the best DSi NAND repair tool out there, offering features such as:
- Fixing MBR
- Formatting FAT partitions
- Fixing Stage2 (NandFirm)
- Recovering deleted HWInfo secure
- Installing minimal firmware titles and unlaunch
- Lots of useful NAND diagnostic info

Why do these matter? You can create a new working NAND from scratch! No backups needed (but still recommended)!

![FileSystem menu](.github/filesystem.png)![NAND CID info](.github/cidinfo.png)

## Notes
- I am including my own hostile and outdated fork of libfat. This is to block `nand_Startup()` during `fatMount()`. Without this having a NAND re-mount would run `nand_Startup()` more than once and break every NAND R/W function until reboot...
- I do not use the release NandFirm/stage2/bootloader (v2435-8325). Instead I use newer NandFirms as listed below. These NandFirms are able to run unlaunch, however they will stop the installer from working ("unknown bootcode version"). Unlaunch installs carry a brick risk by sometimes erasing the Launcher TMD, so this will somewhat forcefully encourage users to move to a [safer installer](https://github.com/edo9300/unlaunch-installer). Normally I'm against intentionally breaking things but this will prevent future bricks.
	- v2265-9336 (prod)
	- v2725-9336 (dev)
- My HWInfo Secure recovery works by searching specific locations for HWInfo. As far as I can tell an officially created HWInfo Secure will *only* start at one of the below 3 offsets. Homebrew (like this tool) will likely create it elsewhere.
	- `0x790000`: retail and pandas
	- `0x784000`: ISTC (needs confirmation) and ISTD
	- `0x794000`: Happened when I deleted HWInfo and let SystemUpdater create it

## Including system files
*B-b-but you have system files like the bootloader!* Sorry, these files are required to boot a DSi and almost always cannot be custom made due to signing. This is a tool meant to fix every possible software brick, and to do so in an accessible way. Most people are not going to know how to recover a bootloader, find the right FontTable for their region, or get the correct and safe wlanfirm/sysmenuVersion. Even given a well written guide, I know the "what's a hex editor" or "what's a byte" messages all too well. It is not safe to force people to chase after things they may not understand. Especially when one wrong file will cause a brick, and the owner will be none the wiser.