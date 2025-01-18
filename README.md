# TUISIC

TUI Online Music Streaming application.

https://github.com/user-attachments/assets/264c5324-e985-48e6-83c4-a8c60a4d3e14



It let's you search and play online songs from cli hassle free.

## Sources
It fetches songs from some platforms:
- SoundCloud
- LastFM
- ForestFM
- YouTube ( Yet to be implemented )

## Installation: 
1. Using [AUR](https://aur.archlinux.org/packages/tuisic-git) package

```sh
yay -S tuisic-git
```

2. Building from source

### Dependencies 
```sh
sudo pacman -S curl mpv fmt yt-dlp
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
