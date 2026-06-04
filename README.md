# Aquilon

Aquilon is a small SDL3-based 2D sandbox game prototype with procedural terrain, chunk streaming, simple player movement, a lightweight GUI, and structured logging.

## Features

- Procedural world generation with chunk loading around the player
- Tile rendering with SDL3_image textures
- Powerful audio system using SDL3_mixer
- Basic player movement and camera follow
- GUI window with close / drag behavior
- Colored console logging and file logging to `aquilon.log`
- Runtime renderer backend, log level, and VSync selection through `aquilon.cfg`
- New C-style networking module scaffolded for future multiplayer work

## Requirements

- CMake 3.16 or newer
- A C++17 compiler
- SDL3, SDL3_image, and SDL3_ttf are fetched automatically by CMake

## Linux Setup

Debian/Ubuntu based systems:

```bash
sudo apt update
sudo apt install -y \
  cmake g++ ninja-build \
  libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev
```

## Linux Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

If you prefer Ninja locally, you can use it with the same build type:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/Debug/Aquilon.exe
```

On Windows with the generated Visual Studio build, the executable is usually in `build/Debug/Aquilon.exe`.
On Linux, the executable is usually `build/Aquilon`.

The game reads `aquilon.cfg` from the same directory as the executable. If the file does not exist, Aquilon creates it on first launch with default values.

## Config

`aquilon.cfg`:

```ini
# Aquilon configuration
renderer = auto
log_level = trace
vsync = on
window_width = 1280
window_height = 720
chunk_distance = 4
move_up = W, Up
move_down = S, Down
move_left = A, Left
move_right = D, Right
inventory_toggle = E
inventory_close = Escape
zoom_in = Equals, Keypad +
zoom_out = Minus, Keypad -
```

Accepted renderer values include `auto`, `software`, `gpu`, `vulkan`, `opengl`, `direct3d11`, `direct3d12`, and `metal`.
Key bindings use SDL key names and can list multiple keys separated by commas.

## Resources

- Textures: `resources/textures/`
- Font: `resources/fonts/arial.ttf`

## Notes

- If a texture fails to load, the game logs the exact file and SDL error.
- The world keeps chunks loaded around the player and unloads distant ones to limit memory growth.
- `chunk_distance` controls how many chunks are kept loaded in each direction around the player.
- Networking is currently transport-level only: connect, host, send framed packets, and poll events.
