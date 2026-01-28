# Window Manager (wm) User Guide

## Overview

`wm` is a terminal multiplexing system included in the CPL system. This historical tool, developed in the 1980s, provides functionality similar to modern `tmux` or `screen`.

## Basic Usage

### Starting wm

```bash
# From the build directory
cd build
./wm

# Or if installed
wm
```

### Basic Window Commands

Window Manager uses **control characters** to manipulate windows. The default command prefix is `Ctrl-Z`.

#### Main Commands

| Command | Function |
|---------|----------|
| `Ctrl-Z c` | Create a new window (create) |
| `Ctrl-Z n` | Move to next window (next) |
| `Ctrl-Z p` | Move to previous window (previous) |
| `Ctrl-Z k` | Kill current window (kill) |
| `Ctrl-Z l` | List all windows (list) |
| `Ctrl-Z r` | Redraw screen (redraw) |
| `Ctrl-Z z` | Send literal `Ctrl-Z` character |
| `Ctrl-Z q` | Quit Window Manager (quit) |

### Usage Examples

#### 1. Basic Session

```bash
# Start wm
./wm

# Create a new window
# Press Ctrl-Z then c

# Switch to another window
# Press Ctrl-Z n

# Check window list
# Press Ctrl-Z l
```

#### 2. Running Multiple Programs Simultaneously

```bash
# Window 1: CPL interpreter
./cpl

# Press Ctrl-Z c to create a new window
# Window 2: Text editor
vi myprogram.cpl

# Press Ctrl-Z c for another window
# Window 3: System monitoring
top

# Navigate between windows with Ctrl-Z n / Ctrl-Z p
```

## Technical Details

### Architecture

`wm` operates using the following mechanisms:

1. **PTY (Pseudo-Terminal)** - Assigns a virtual terminal to each window
2. **Process Management** - Runs independent processes in each window
3. **Signal Handling** - Detects process termination via `SIGCHLD`
4. **Terminal Control** - Controls terminal raw mode with termios

### Source Code Structure

- `wm.c` - Main program (PTY management, signal handling)
- `winlib.c/h` - Window library functions
- `display.c/h` - Display management
- `term.c/h` - Terminal control (termios)
- `termcap` - Terminal capability database

### Modernization Highlights

Version 4.0 includes the following modernizations:

- **BSD PTY → POSIX openpty()** - From manual `/dev/ptyXX` iteration to standard API
- **signal() → sigaction()** - More reliable signal handling
- **sgtty → termios** - From legacy terminal control API to POSIX standard
- **K&R C → ANSI C (C11)** - Function prototypes, safe string operations

## Troubleshooting

### Terminal Not Working Correctly

```bash
# Set TERM environment variable
export TERM=xterm-256color

# Or
export TERM=xterm
```

### Cannot Access PTY Devices

```bash
# Linux: Check if /dev/pts is mounted
mount | grep devpts

# Check PTY master device
ls -la /dev/ptmx
```

### Cannot Create Windows

- You may have reached the system's PTY limit
- Close existing windows and try again

### Cannot Send Control Characters

- Use `Ctrl-Z z` to send literal `Ctrl-Z`
- If conflicting with shell job control, check your shell configuration

## Limitations

1. **Terminal Compatibility** - Requires specific terminal capabilities (cursor addressing and screen control)
2. **Unicode Support** - Designed for ASCII
3. **Performance** - This is a 1987 research interpreter
4. **Platforms** - POSIX-compliant systems (Linux, macOS, BSD)

## Historical Background

This window manager was developed in 1985-1987. It was groundbreaking for its time:

- Running multiple processes in independent "windows"
- Terminal multiplexing
- Process switching

This implementation was a precursor to modern `tmux` and `screen`.

## Further Information

- Complete specification: See `handout.tex`
- Implementation details: See comments in `src/wm.c`
- Build instructions: See "Building and Running" section in `CLAUDE.md`
