# Simple MP3 Player

A silly little MP3 player application for Windows 11, featuring both command-line and graphical interfaces. Built in C, because i don’t like the ones that exists and I’m tired of using Apple Music.

## Features

- **Dual Interface**
  - Command-line interface (TUI)
  - Graphical user interface (GUI) with more features

- **Audio Playback**
  - Play, pause, stop, and seek, very basic stuff
  - Volume control
  - Simple Equalizer
  - Multiple playback modes

- **Library Management**
  - Automatic MP3 file scanning
  - Continuous background monitoring
  - Support for ID3v1 and ID3v2 tags
  - Album art display
  - Sorting by multiple criteria (title, artist, album, year, genre, track)

- **Metadata Support**
  - Title, artist, album, year, genre, and track number
  - Embedded album art (JPEG, PNG)

## Requirements

- Windows 11
- Compatible C compiler
- BASS audio library

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/mp3player.git
   cd mp3player
   ```

2. Install dependencies:
   - Download and install the [BASS audio library](https://www.un4seen.com/)
   - Ensure GDI+ is available (included in Windows)

3. Build the project:
   ```bash
   make
   ```

## Usage

### Command Line Interface

Start the player with:
```bash
.\bin\mp3player.exe [folder_path]
```
Where `[folder_path]` is the optional path of the folder to scan (default: current folder).

Available commands for the TUI:
- `scan [directory]` - Manually scan a directory
- `monitor [interval]` - Start continuous background scanning (interval in seconds, default: 60)
- `stop` - Stop continuous scanning
- `list` - Show all detected MP3 files
- `info [number]` - Show detailed information about an MP3 file
- `sort [criterion]` - Sort MP3 files (title, artist, album, year, genre, track)
- `filter [type] [text]` - Filter MP3 files (title, artist, album, genre, year)
- `reset` - Reset display to complete list
- `gui` - Start graphical interface
- `quit` - Exit program

### Graphical Interface

- List and grid view modes (not implemented yet!)
- Album art display
- Playback controls
- Equalizer settings
- File information panel (looks ugly)

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- [BASS Audio Library](https://www.un4seen.com/) for audio playback
