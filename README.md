# PS2 Loader

## Description

- I wanted to do a PS2 game loader like I did for PS1.
- Tested on PCSX2 Nightly (v1.7.5577 Compiled on Feb 24 2024).
- Tested on SCPH-50004 (mechapwn'd) (ISO file was patched with filepatcher to be a master disc and launched using wLE).

## Compiling and usage

To build this, you'll need the (latest recommended) [PS2SDK](https://github.com/ps2dev/ps2dev) (on Windows, you might need to do extra steps if using msys2).

Once you have the SDK ready, you can start compiling it by going into the git cloned or downloaded folder.

Now that you are in the source directory, you can do ```make``` and it should compile and create an elf file.

Once you have compiled and have an elf file, now you get to extract all the game files, combine them, and hex edit some bytes (since CRASH.BD and BH are a little different between versions) to make the game's elf binary read the correct files. (I recommend renaming the elf binary to ```SLES_999.01``` and using SYSTEM.CNF that was included in the source).

Once you have combined all files correctly, I recommend using [PS2ImageMaker](https://github.com/Smartkin/PS2ImageMaker) to create the.iso file (you could use UltraISO too, but it felt slow).

Now you can run the .iso file on an emulator or on real hardware with or without a modchip (not tested with a modchip).

If you want, you could just place game files on a disc and launch this elf from USB or from the disc directly.

## Credits

Thanks to [PS2dev team](https://github.com/ps2dev) for the SDK, examples, and documentation.
