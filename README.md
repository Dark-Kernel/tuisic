# TUISIC

TUI Online Music Streaming application.


https://github.com/user-attachments/assets/ef349a06-0d7e-488b-aa3d-de928d9b0eef



First app of its kind, It let's you search and play online songs from cli hassle free.

## Features
- Vim motions
- Easy downloads
- Multiple song sources
- Playlist support
- Copy urls
- Add to favourites
- Configuration file
- Daemon mode (BETA, press 'w' to toggle) 
- [MPRIS DBUS](https://wiki.archlinux.org/title/MPRIS) support ( via `playerctl` )

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
1. Using [AUR](https://aur.archlinux.org/packages/tuisic-git) package

```sh
yay -S tuisic-git
```

2. Building from source

### Dependencies 
```sh
sudo pacman -S curl mpv fmt yt-dlp fftw sdbus-cpp
```

### Build, Compile & Run

```sh
mkdir build && cd build
cmake ..
make
sudo make install
```

## Thanks to all.

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [libmpv](https://github.com/mpv-player/mpv/tree/master/libmpv)
- [libcurl](https://curl.se/libcurl/)
- [Cava](https://github.com/karlstav/cava.git)


## Contribution 
It's open for contribution, read [CONTRIBUTING.md](./CONTRIBUTING.md) for more information.
