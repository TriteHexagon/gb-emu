This is a Game Boy Color emulator.

The CPU, timer, audio, graphics, joypad, and saving are emulated.

The CPU has been tested using Blargg's cpu_instrs tests, with all tests passing.

# Building in a Unix-like environment

Ensure that g++, make, CMake, pkg-config, and SDL2 are installed.

Enter the following on the command line:

```
cmake .
make
```

# Building on Windows

Ensure that Visual Studio, CMake, and Python are installed.

Run the `setup_sdl2_vc.py` script to download SDL2 for MSVC.

```
python setup_sdl2_vc.py
```

Generate the Visual Studio project with CMake.

```
cmake .
```

Open `gb_emu_project.sln` in Visual Studio and build the solution.
