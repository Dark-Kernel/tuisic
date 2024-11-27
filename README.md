# TUISIC

TUI Online Music Streaming application.

It let's you search and play online songs from cli hassle free.

## Sources
It fetches songs from some platforms:
- SoundCloud
- LastFM
- ForestFM
- YouTube ( Yet to be implemented )

## 
Build, Compile & Run

```sh
mkdir build && cd build
cmake ..
make
./tuisic
```

## What it uses ?

It uses FTXUI which provides efficient TUI interface.
MPV is used as player.
CURL to make requests.
