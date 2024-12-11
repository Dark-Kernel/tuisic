# TUISIC

TUI Online Music Streaming application.


https://github.com/user-attachments/assets/c37dbf88-68fa-4bd9-8289-ad6937bd77a7


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
