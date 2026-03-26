# BuckshotESP (Game version - v2.2.0.6)

Shell ESP for Buckshot Roulette. Shows the shell order if you're hosting, or tracks items and calculates odds if you're a client.

Contact: [DesirePro](https://t.me/desirepro) | free source and product!

## Demo 

[![BuckshotESP Demo](https://github.com/user-attachments/assets/9c5cd71b-cae2-4ade-9bfb-cc56861bc3b3)](https://youtu.be/WzOyQDW531o)

> Click the image to watch on YouTube

## What it does

- First run: finds the game automatically (reads Steam path from registry), patches the exe, saves a backup
- Host (singleplayer / multiplayer host): full shell sequence in the overlay, updates in real time as shells get fired
- Client (multiplayer): shows current shell after any item use, tracks live/blank counts, calculates probability, logs every event (shots, beer ejects, inverter flips, phone reveals)
- Overlay sits in the top-right corner, transparent, click-through, always on top

## How it works

The patcher swaps a few GDScript files inside the game's PCK. Those scripts write the shell data to a text file every frame. The ESP reads that file and renders the overlay. No memory injection, no DLL hooks.

GDRE Tools is used for PCK patching. If you don't have it, the ESP installs it automatically via winget (or downloads from GitHub if winget isn't available).

## Building

Needs MSVC (VS 2022) and C++17. Open the folder in VS via CMake, or:

```
cmake -B build -A x64
cmake --build build --config Release
```

Or just run `build.bat` from a Developer Command Prompt.

Manual:
```
cl /nologo /EHsc /O2 /std:c++17 /W3 /MT /I"src" /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /Fe:BuckshotESP.exe src\main.cxx src\config.cxx src\patcher.cxx src\esp.cxx src\overlay.cxx user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib
```

The binary is statically linked (`/MT`), no vcruntime dependency. Runs on any Windows 10/11 out of the box.

## Usage

1. Run `BuckshotESP.exe`
2. It finds the game automatically. If it can't, a file picker opens
3. Patches the game on first run (backup saved as `.backup` next to the original)
4. Start the game, overlay appears
5. ESC to quit

## Project layout

```
src/
├── main.cxx              # Entry point, main game loop
├── config.cxx            # Game path and patch status management
├── config.hxx
├── patcher.cxx           # Finds Steam via registry, auto-detects game, runs GDRE patcher
├── patcher.hxx
├── esp.cxx               # Parses shell data, tracks rounds/shots/items
├── esp.hxx
├── overlay.cxx           # GDI overlay rendering
├── overlay.hxx
└── embedded_scripts.hxx  # Patched GDScript files (generated, ~400KB)

CMakeLists.txt
build.bat
```

## Item tracking (client mode)

| Item | What happens |
|------|-------------|
| Magnifier | Shows current shell |
| Beer | Ejects current shell, count updates |
| Inverter | Flips current shell, counts adjust |
| Phone | Shows shell at position N (only if you used it) |
| Handsaw, Cigarettes, Adrenaline, etc. | Leaks current shell from packet |

Every item use sends `current_shell_in_chamber` to all clients. The inverter packet contains the pre-flip value, ESP flips it automatically.

## Notes

- Multiplayer client can't see the full sequence (the host strips it from packets). You get the current shell after any item/shot event, plus probability based on remaining counts.
- To unpatch: rename `Buckshot Roulette.exe.backup` back to `Buckshot Roulette.exe`, or verify game files through Steam.
- Game version tested: v2.2.0.6 (Godot 4.1.1)
