# TUISIC

TUI Online Music Streaming application with MCP support


https://github.com/user-attachments/assets/ef349a06-0d7e-488b-aa3d-de928d9b0eef



First app of its kind, It let's you search and play online songs from cli hassle free. Now it supports AI integration through [MCP](https://modelcontextprotocol.io/docs/getting-started/intro) (BETA). Checkout Demo's [below](#demos)

## Features
- Vim motions
- No browser, No Ads
- Easy downloads
- Multiple song sources
- Playlist support
- Easy Sharing
- Favourites List
- Configuration file
- Daemon mode (BETA, press 'w' to toggle) 
- [MPRIS DBUS](https://wiki.archlinux.org/title/MPRIS) support ( via `playerctl` )
- Cava Visualizer (BETA)
- Support for AI Integration via [MCP](https://modelcontextprotocol.io/docs/getting-started/intro) (BETA)

## Shortcuts

| Key | Action |
| --- | --- |
| `q` | quit |
| `w` | toggle daemon mode |
| `>` | next song |
| `<` | previous song |
| `<space>` | play/pause |
| `Alt+l` | copy url |
| `d` | download song |
| `a` | add to favourites |
| `.` | seek forward |
| `,` | seek backward |
| `m` | mute |



## Sources
It fetches songs from some platforms:
- JioSaavn
- SoundCloud
- LastFM
- ForestFM
- YouTube
- YouTube Music (on the way)

## Installation: 

1. Check [releases](https://github.com/Dark-Kernel/tuisic/releases)

2. Using [AUR](https://aur.archlinux.org/packages/tuisic-git) package

```sh
yay -S tuisic-git
```

### Building from source

#### Build Options
| CMake Flag       | Description                       | Default |
| ---------------- | --------------------------------- | ------- |
| `-DWITH_MPRIS`   | Enable MPRIS (sdbus-c++) support  | ON      |
| `-DWITH_CAVA`    | Enable Cavacore-based visualizer  | OFF     |


#### Before Installation
Update the [desktop/tuisic.desktop](./desktop/tuisic.desktop) file.

```
Exec=alacritty -e tuisic # Change terminal accordingly
```

#### Debian

1. Install dependencies

```
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libfftw3-dev libmpv-dev libcurl4-openssl-dev libfmt-dev libsystemd-dev rapidjson-dev libpulse-dev
```

2. If you want to use [MPRIS](https://wiki.archlinux.org/title/MPRIS) then install [sdbus-cpp](https://github.com/Kistler-Group/sdbus-cpp)

```
git clone --depth 1 https://github.com/Kistler-Group/sdbus-cpp.git
cd sdbus-cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --build . --target install
```

3. Build

```
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
```

#### Arch Linux

1. Install dependencies

```sh
sudo pacman -S curl mpv fmt yt-dlp fftw sdbus-cpp rapidjson
```

2. Build

```
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
```

#### MacOS

1. Install dependencies

```
brew install cmake pkg-config fftw mpv curl fmt rapidjson
```

2. Set environment variables for Homebrew paths

```
# Detect Homebrew prefix (Apple Silicon uses /opt/homebrew, Intel uses /usr/local)
BREW_PREFIX=$(brew --prefix)
export HOMEBREW_PREFIX=${BREW_PREFIX}
export PKG_CONFIG_PATH=${BREW_PREFIX}/lib/pkgconfig:${BREW_PREFIX}/opt/curl/lib/pkgconfig:${BREW_PREFIX}/opt/fmt/lib/pkgconfig
export CMAKE_PREFIX_PATH=${BREW_PREFIX}
export LDFLAGS=-L${BREW_PREFIX}/lib
export CPPFLAGS=-I${BREW_PREFIX}/include
```

3. Build

```
cmake .. \
  -DWITH_MPRIS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${HOMEBREW_PREFIX}"

cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
```

### MISC Build, Compile & Install

```sh
mkdir build && cd build
cmake .. # -DWITH_MPRIS=OFF -DWITH_CAVA=ON
make
sudo make install
```

### DEMOS

1. Screenshots: [here](https://blogs.sumit.engineer/showcase/) scroll way down.

<img width="1366" height="768" alt="1762971302" src="https://github.com/user-attachments/assets/afd5e0ca-9600-4075-97d2-9a15713a17f6" />
<img width="1366" height="768" alt="2025-09-17_21-34-11" src="https://github.com/user-attachments/assets/61123987-ea48-4d75-9954-fec9e52f7714" />
<hr>

2. Visualizer:

https://github.com/user-attachments/assets/1f512307-7c70-466d-9cd0-529e195871d0


3. MCP Server:


https://github.com/user-attachments/assets/7601312a-9bca-4970-90a3-4a917fbe2fda




## Thanks to all.

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [libmpv](https://github.com/mpv-player/mpv/tree/master/libmpv)
- [libcurl](https://curl.se/libcurl/)
- [Cava](https://github.com/karlstav/cava.git)


## Contribution 
It's open for contribution, read [CONTRIBUTING.md](./CONTRIBUTING.md) for more information.
