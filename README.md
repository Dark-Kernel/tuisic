# TUISIC

TUI Online Music Streaming application.

https://github.com/user-attachments/assets/8e97eced-cf83-447b-8160-b2871ca4cb61

It let's you search and play online songs from cli hassle free.

## Sources
It fetches songs from some platforms:
- SoundCloud
- LastFM
- ForestFM
- YouTube ( Yet to be implemented )

## Dependencies 
```sh
sudo pacman -S curl mpv
```

## 
Build, Compile & Run

```sh
mkdir build && cd build
cmake ..
make
./tuisic
```

## What it uses ?

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [libmpv](https://github.com/mpv-player/mpv/tree/master/libmpv)
- [libcurl](https://curl.se/libcurl/)


## Contribution 
It's open for contribution, read [CONTRIBUTING.md](./CONTRIBUTING.md) for more information.
