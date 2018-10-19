This is a work-in-progress Game Boy emulator.

The CPU, timer, graphics, joypad, and save files are emulated. Audio remains unemulated.

The CPU has been tested using Blargg's cpu_instrs tests, with all tests passing.

To build the emulator in a Unix-like environment:
* First, ensure that pkg-config and SDL2 are installed.
* Then, enter `make` on the command line.
