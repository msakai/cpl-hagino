# Categorical Programming Language (CPL)

**Version 4.0.0** - Modernized Implementation (2026)

A modernized port of Tatsuya Hagino's **Categorical Programming Language** from 1985-1987, fully ported from Franz Lisp to SBCL (Common Lisp) and from K&R C to ANSI C (C11).

## Overview

CPL is a purely categorical programming language based on category theory. It uses:
- **Initial algebras** for inductive data types (natural numbers, lists)
- **Final coalgebras** for coinductive data types (infinite lists, streams)
- **Universal properties** via factorizers (products, coproducts, exponentiation)
- **Reduction rules** for program evaluation

This implementation predates modern functional programming languages and provides a unique perspective on category-theoretic computation.

## Quick Start

### Prerequisites

- SBCL (Steel Bank Common Lisp) 2.0.0+
- CMake 3.15+
- C compiler (gcc 9.0+ or clang 10.0+)
- ncurses library

### Build

```bash
mkdir build && cd build
cmake ..
make
```

### Run

```bash
./cpl
```

### Example Session

```lisp
cpl> read Nat                          # Load natural numbers
cpl> simp add.pair(s.s.0, s.s.s.0)    # Compute 2 + 3
s.s.s.s.s.0                            # Result: 5

cpl> read List                         # Load lists
cpl> simp seq.s.s.s.0                 # Generate [1,2,3]
cons.pair(s.0,cons.pair(s.s.0,cons.pair(s.s.s.0,nil.!)))

cpl> help                              # Show help
cpl> quit                              # Exit
```

## What's New in 4.0

### Lisp Modernization (Franz Lisp → SBCL)
- ✅ Complete port of ~4,500 lines of Franz Lisp to Common Lisp
- ✅ Franz Lisp compatibility layer (~500 lines)
- ✅ All 6 Lisp modules ported: wcat, wmlib, wdia, wcathelp, wtrace, tv
- ✅ ASDF build system
- ✅ Standalone executable generation

### C Modernization (K&R C → ANSI C)
- ✅ Terminal interface: sgtty → POSIX termios
- ✅ PTY handling: BSD /dev/ptyXX → POSIX openpty()
- ✅ Signal handling: signal() → sigaction()
- ✅ All functions converted to ANSI C prototypes
- ✅ Safe string operations (snprintf, bounds checking)
- ✅ Memory safety improvements

### Build System
- ✅ CMake cross-platform build system
- ✅ Automatic SBCL detection and executable building
- ✅ CTest integration
- ✅ Platform detection (Linux, macOS, BSD)

### Testing
- ✅ Unit tests for compatibility layer
- ✅ Integration tests for system files
- ✅ Test automation scripts

## Project Structure

```
cpl-hagino/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── CLAUDE.md               # Comprehensive documentation
│
├── src/                    # Source code
│   ├── *.lisp              # Modernized SBCL implementation
│   ├── *.l                 # Original Franz Lisp (historical)
│   ├── *.c, *.h            # Modernized C implementation
│   ├── cpl.asd             # ASDF system definition
│   └── build-executable.lisp
│
├── system/                 # CPL object definitions
│   ├── Nat                 # Natural numbers
│   ├── List                # Lists
│   ├── InfList             # Infinite lists
│   ├── CoNat               # Co-natural numbers
│   └── Sort                # Sorting
│
├── test/                   # Test suite
│   ├── franz-compat-test.lisp
│   ├── integration-test.lisp
│   └── run-tests.sh
│
└── build/                  # Build directory (created by CMake)
    ├── cpl                 # CPL interpreter executable
    └── wm                  # Window manager executable
```

## Documentation

See [CLAUDE.md](CLAUDE.md) for comprehensive documentation including:
- Detailed build instructions
- CPL language reference
- Modernization details
- Example programs
- Troubleshooting guide

## Testing

```bash
# Run all tests
cd test
./run-tests.sh

# Or with CMake
cd build
ctest
```

## Components

### CPL Interpreter (`cpl`)
The categorical programming language interpreter with:
- Object and morphism parser
- Categorical type checker
- Reduction/simplification engine
- Interactive REPL
- Diagram editor
- Tracing system

### Window Manager (`wm`)
Terminal multiplexer with:
- Multiple process management
- Window creation and switching
- Terminal I/O routing
- History and scrollback

## Historical Context

CPL was developed by Tatsuya Hagino at the University of Edinburgh (1985-1987) as part of his PhD research on categorical programming. It demonstrates:

- Pure category-theoretic computation
- Initial algebras (inductive types)
- Final coalgebras (coinductive types)
- Universal constructions
- Primitive recursion and corecursion

This predates Haskell (1990), OCaml (1996), and modern functional programming languages.

## Platform Support

Tested on:
- macOS 14+ (ARM64, x86_64)
- Ubuntu 20.04+ (x86_64)
- Should work on any POSIX-compliant system with SBCL and ncurses

## License

Historical academic implementation. Original code developed at the University of Edinburgh (1985-1987).

Modernization (2026) maintains the academic nature. Use for educational and research purposes.

## References

- Hagino, T. (1987). "A Categorical Programming Language". PhD Thesis, University of Edinburgh.
- Mac Lane, S. (1971). "Categories for the Working Mathematician"
- Lambek, J. (1968). "A fixpoint theorem for complete categories"

## Acknowledgments

- **Tatsuya Hagino** - Original implementation
- **University of Edinburgh** - Where CPL was developed
- **SBCL Development Team** - Modern Common Lisp platform

---

**Status**: ✅ Fully functional modernization
**Version**: 4.0.0 (January 2026)
**Original**: 3.0 (1987)
