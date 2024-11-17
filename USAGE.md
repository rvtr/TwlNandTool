# TwlNandTool Usage

With great power (TwlNandTool) comes great resposibility. It can be easy to cause further damage to your console if you don't know what you're doing.

I will add a friendly built in "tutorial" at some point, but until then please do not use any function without reading this page first.

## Table of Contents

  - [FileSystem Menu](#filesystem-menu)
    - [Read MBR](#--read-mbr)
    - [Repair MBR](#--repair-mbr)
    - [Format TWL_MAIN](#--format-twl_main)
    - [Format TWL_PHOTO](#--format-twl_photo)
    - [Mount TWL_MAIN](#--mount-twl_main)
    - [Unmount TWL_MAIN](#--unmount-twl_main)
    - [Mount NitroFS](#--mount-nitrofs)
  - [NandFirm Menu](#nandfirm-menu)
    - [Check NandFirm](#--check-nandfirm)
    - [Import NandFirm](#--import-nandfirm)
    - [Import NandFirm (SDMC)](#--import-nandfirm-(sdmc))
  - [Sys File Menu](#sys-file-menu)
    - [Find HWINFO\_S.dat](#--find-hwinfo_sdat)
    - [Find HWINFO\_S.dat (deep)](#--find-hwinfo_sdat-(deep))
    - [Init HWINFO\_N.dat](#--init-hwinfo_ndat)
    - [Init HWINFO\_S.dat](#--init-hwinfo_sdat)
    - [Init cert.sys](#--init-certsys)
    - [Init TWLFontTable](#--init-twlfonttable)
  - [Chip Info Menu](#chip-info-menu)
    - [CID Info](#--cid-info)
    - [ConsoleID Info](#--consoleid-info)

## FileSystem Menu
All of these functions deal with the filesystem. This includes MBR, partitions, and TwlNandTool's own included files.
### - Read MBR
Read and verify the Master Boot Record. This is the first 512b of NAND (encrypted) that determines where the partitions are.
### - Repair MBR
Repair the Master Boot Record if corrupted.
### - Format TWL_MAIN
Format TWL\_MAIN. This is the partition where firmware, system files, games, and saves are stored. **Formatting this is an immediate brick unless you know what you are doing.**
### - Format TWL_PHOTO
Format TWL\_PHOTO. This is the partition where all photos are stored.
### - Mount TWL_MAIN
Mount TWL\_MAIN in order to access NAND files and to lock the formatting options.
### - Unmount TWL_MAIN
Unmount TWL\_MAIN in order to unlock formatting, or to repair MBR. The unmount will prevent you from accessing NAND files.
### - Mount NitroFS
Mount TwlNandTool's filesystem in order to install included system files.

## NandFirm Menu
These functions deal with the stage2 bootloader- officially called NandFirm. NandFirm is what boots the home menu.
### - Check NandFirm
Verifies that NandFirm is not corrupted.
### - Import NandFirm
Install the standard NandFirm. The versions I include are slightly updated for reasons mentioned in the [README](README.md).
- `v2265-9336` (prod)
- `v2725-9336` (dev)
### - Import NandFirm (SDMC)
Install the [SDMC Launcher](https://randommeaninglesscharacters.com/dsidev/sdmc_launcher.html) NandFirm. It is completely different from the standard NandFirm, and it will leave the home menu inaccessible. This is only for extreme edge cases and should be avoided.

**DO NOT INSTALL THIS UNLESS OTHERWISE TOLD.**

## Sys File Menu
These functions deal with system files that are required for the DSi to boot. Much of this is focused around HWInfo Secure, the file that sets your region and serial number. It is impossible to recreate due to signing, and without it you will be permanently forced to use unlaunch.
### - Find HWINFO\_S.dat
Check if HWINFO_S.dat exists in TWL\_MAIN (requires [mounting](#--mount-twl_main)). If deleted, this function will check commonly used offsets (no mount). This is the quickest search and usually finds HWINFO\_S.dat.
### - Find HWINFO\_S.dat (deep)
Search the entire TWL\_MAIN (no mount) byte by byte until HWINFO\_S.dat is found. This is a last resort option and will take a very long time to complete.
### - Init HWINFO\_N.dat
Create a new HWINFO\_N.dat.
### - Init HWINFO\_S.dat
Create a new HWINFO\_S.dat in TWL\_MAIN by running the [Find HWINFO\_S.dat](#--find-hwinfo_sdat) function.
### - Init cert.sys
Create a new certificate chain.
### - Init TWLFontTable
Install the font files required by the firmware. This is region specific, so it will depend on [Init HWINFO\_S.dat](#--init-hwinfo_sdat) having recovered and saved a HWINFO\_S.dat (or it will use HWINFO_S.dat if already there). If no HWINFO\_S.dat exists, the region will default to USA and the respective TWLFontData will be installed.

## Chip Info Menu
These functions show info on the DSi's chips.
### - CID Info
Displays the NAND ID, along with a breakdown of what it means (manufacturer ID, date, type, etc).
### - ConsoleID Info
Displays the CPU ID, and a guesstimate of the manufacturing date.