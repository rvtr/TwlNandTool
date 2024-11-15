# ![app icon](icon.bmp) TwlNandTool

This is the best DSi NAND repair tool out there, offering features such as:
- Fixing MBR
- Formatting FAT partitions
- Fixing Stage2 (NandFirm)
- Recovering deleted HWInfo secure
- Installing minimal firmware titles and unlaunch
- Lots of useful NAND diagnostic info

Why do these matter? You can create a new working NAND from scratch! No backups needed (but still recommended)!

## Notes
- I am including my own hostile and outdated fork of libfat. This is to block `nand_Startup()` during `fatMount()`. Without this having a NAND re-mount would run `nand_Startup()` more than once and break every NAND R/W function until reboot...