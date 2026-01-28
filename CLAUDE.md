# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a **modernized implementation** of the **Categorical Programming Language (CPL)** from 1985-1987, developed by Tatsuya Hagino at the University of Edinburgh.

**Version 4.0.0 (2026)** - Fully ported from Franz Lisp to SBCL (Common Lisp) and from K&R C to ANSI C (C11).

The codebase consists of two main components:

1. **CPL Interpreter** - Ported from Franz Lisp to SBCL (Common Lisp)
2. **Window Manager System** - Modernized from K&R C to ANSI C (C11)

### What's New in Version 4.0

- **SBCL (Common Lisp)** - Complete port from Franz Lisp to modern Common Lisp
- **ANSI C (C11)** - Modernized from K&R C with sgtty to ANSI C with POSIX termios
- **CMake Build System** - Modern cross-platform build system
- **POSIX Compliance** - Updated PTY handling, signal handling, and terminal I/O
- **Memory Safety** - Replaced unsafe string operations with safe alternatives
- **Works on Modern Systems** - Tested on macOS, Linux, and BSD

## Building and Running (Modernized Version)

### Prerequisites

- **SBCL** (Steel Bank Common Lisp) version 2.0.0 or later
- **CMake** version 3.15 or later
- **C Compiler** - gcc 9.0+ or clang 10.0+
- **ncurses** development library
- **POSIX-compliant OS** (Linux, macOS, BSD)

On macOS with Homebrew:
```bash
brew install sbcl cmake ncurses
```

On Ubuntu/Debian:
```bash
sudo apt-get install sbcl cmake build-essential libncurses-dev
```

### Building with CMake

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
make

# Optional: Run tests
ctest

# Optional: Install
sudo make install
```

This builds:
- `cpl` - The CPL interpreter (SBCL executable)
- `wm` - The window manager (C executable)

### Running the CPL Interpreter

After building:

```bash
# From build directory
./cpl
```

Or if installed:
```bash
cpl
```

You should see:
```
Categorical Programming Language (CPL) v4.0.0
Original version: 3.0 (1987)
Modernized for SBCL - January 2026

Type 'help' for help, 'quit' to exit

cpl>
```

### Running the Window Manager

```bash
# From build directory
./wm
```

### Running Tests

```bash
# From project root
cd test
./run-tests.sh
```

Or with CMake:
```bash
cd build
ctest
```

## Building the Original Franz Lisp Version (Historical)

The original Franz Lisp version is preserved in the source files with `.l` extensions.

**Note:** Franz Lisp is obsolete and no longer maintained. Use the modernized SBCL version instead.

To build the original version (requires Franz Lisp, circa 1987):

```bash
cd src
make lcat    # Requires Franz Lisp compiler
./lcat
```

## CPL Language Architecture

CPL is based on category theory and uses a unique approach to data types:

### Object Declaration System

CPL has no primitive data types. All types are defined through categorical characterizations:

- **Right objects** - Specify terminal objects and right adjoints (e.g., products, exponentiation)
- **Left objects** - Specify initial objects and left adjoints (e.g., natural numbers, lists)

Example object declarations are in the `system/` directory:
- `Nat` - Natural numbers, booleans, arithmetic operations
- `List` - Lists with constructors and operations
- `InfList` - Infinite lists (final co-algebras)
- `CoNat` - Co-natural numbers
- `Sort` - Sorting operations

### Reduction Rules

CPL programs are morphisms (compositions of unit morphisms, factorizers, and functors). The system simplifies terms using reduction rules that:

1. Reduce terms to compositions of co-units and right factorizers
2. Apply productive/coproductive restrictions
3. Generate canonical forms for ground terms

### Key Concepts

- **Factorizers** - Give unique arrows satisfying universal properties (e.g., `pair`, `curry`, `pr`)
- **Unit morphisms** - Natural transformations associated with object declarations
- **Productive/Coproductive** - Restrictions on functors for reduction rules to apply

## File Structure

### Source Directory (`src/`)

**Modernized SBCL Implementation:**
- `cpl-package.lisp` - Package definition
- `franz-compat.lisp` - Franz Lisp compatibility layer (~500 lines)
- `wcat.lisp` - Main CPL interpreter (ported from wcat.l, ~2,248 lines)
- `wcathelp.lisp` - Help system (ported from wcathelp.l)
- `wdia.lisp` - Diagram editor (ported from wdia.l)
- `wmlib.lisp` - Window management library (ported from wmlib.l)
- `wtrace.lisp` - Tracing utilities (ported from wtrace.l)
- `tv.lisp` - Terminal viewer/editor (ported from tv.l)
- `cpl.asd` - ASDF system definition
- `build-executable.lisp` - Executable builder script

**Original Franz Lisp Implementation (Historical):**
- `wcat.l` - Original Franz Lisp source
- `wcathelp.l`, `wdia.l`, `wmlib.l`, `wtrace.l`, `tv.l` - Original modules

**Modernized C Implementation (ANSI C / C11):**
- `wm.c` - Window manager main program (modernized to C11, POSIX PTY, sigaction)
- `winlib.c/h` - Window library functions (safe string operations)
- `display.c/h` - Display management (ANSI C prototypes)
- `term.c/h` - Terminal control (sgtty → termios conversion)
- `config.h.in` - CMake configuration template
- `termcap` - Terminal capability database

**Build System:**
- `CMakeLists.txt` - CMake build configuration (root)
- `makefile` - Original makefile (historical, for Franz Lisp)

**Documentation:**
- `handout.tex` - Complete CPL language specification and examples
- `exm.tex` - Example CPL session demonstrations
- `CLAUDE.md` - This file

### System Directory (`system/`)

Contains CPL object definitions and example programs:
- `Nat` - Natural numbers with addition, multiplication, etc.
- `List` - Lists with constructors and operations
- `InfList` - Infinite lists (final co-algebras)
- `CoNat` - Co-natural numbers
- `Sort` - Sorting operations
- `log` - Session transcript showing CPL in use

Object definitions follow the syntax: `left/right object ... with ... is ... end object;`
Morphisms defined with `let name = expression;`

### Test Directory (`test/`)

Test suite for the modernized CPL system:
- `franz-compat-test.lisp` - Unit tests for Franz Lisp compatibility layer
- `integration-test.lisp` - Integration tests for system files
- `run-tests.lisp` - Standalone test runner
- `run-tests.sh` - Shell script for running tests

### Build Directory (`build/`)

Created by CMake (not in repository):
- `cpl` - Built CPL interpreter executable
- `wm` - Built window manager executable
- Object files and build artifacts

## Modernization Details

### Franz Lisp → SBCL (Common Lisp)

The CPL interpreter was fully ported from Franz Lisp to SBCL-compatible Common Lisp:

**Key Changes:**
- **Package System**: Added proper `defpackage` and `in-package` declarations
- **Special Variables**: Converted `(declare (special ...))` to `(defvar ...)`
- **Function Definitions**: Converted `(def name (lambda ...))` to `(defun name ...)`
- **Macros**: Converted `(def name (macro ...))` to `(defmacro name ...)`
- **nlambda**: Converted unevaluated-argument functions to macros
- **lexpr**: Converted variable-argument functions to `&rest` with helpers
- **I/O Functions**: Implemented Franz primitives (tyi, tyo, tyipeek) using CL equivalents
- **Type Predicates**: Replaced `dtpr` with `consp`, `bcdp` with `compiled-function-p`
- **Property Lists**: Standardized to Common Lisp `get`/`setf` patterns
- **Error Handling**: Converted `errset` to `handler-case`
- **Function Manipulation**: Implemented `getd`/`putd` using `symbol-function`

**Compatibility Layer** (`franz-compat.lisp`):
Provides ~50 Franz Lisp primitives, making the port transparent to the CPL code.

### K&R C → ANSI C (C11)

The window manager was modernized from 1980s K&R C to modern ANSI C:

**Terminal Interface (term.c):**
- **sgtty → termios**: Complete rewrite of terminal I/O
  - `struct sgttyb` → `struct termios`
  - `gtty()`/`stty()` → `tcgetattr()`/`tcsetattr()`
  - Flags converted to POSIX termios equivalents
- **File Descriptors**: Hardcoded 0/1/2 → `STDIN_FILENO`/`STDOUT_FILENO`/`STDERR_FILENO`

**PTY Handling (wm.c):**
- **BSD → POSIX**: Manual `/dev/ptyXX` iteration → `openpty()` from `<pty.h>`
- **Cross-platform**: Works on Linux, macOS, and BSD

**Signal Handling (wm.c):**
- **signal() → sigaction()**: Reliable POSIX signal handling
- Added `SA_RESTART` and `SA_NOCLDSTOP` flags for robustness

**Function Prototypes (all .c files):**
```c
// K&R style (old):
doaddch(ch, p, proc)
char ch;
int p;
struct pro_str *proc;
{ ... }

// ANSI C style (new):
int doaddch(char ch, int p, struct pro_str *proc) {
    ...
}
```

**String Safety (winlib.c):**
- `strcpy()` → `strncpy()`/`snprintf()`
- `sprintf()` → `snprintf()` with buffer size checks
- Added bounds checking to prevent buffer overflows

**Memory Safety:**
- Removed `register` keywords (obsolete)
- Added `const` qualifiers for read-only data
- Fixed implicit type declarations
- Added proper `NULL` pointer checks

## CPL Command Reference

From within the CPL interpreter (`cpl`):

- `edit` - Input or edit objects and morphisms
- `show` - Display objects and morphisms
- `delete` - Remove objects and morphisms
- `let` - Define morphisms (e.g., `let add=ev.pair(...)`)
- `simp` - Simplify morphisms (execute reduction)
- `expand` - Expand auxiliary morphisms
- `read` - Load object definitions from file (e.g., `read Nat`)
- `load` - Load saved session file
- `save` - Save definitions to file
- `diagram` - Open diagram editor
- `set` - Configure flags (e.g., `set trace on`, `set trace off`)
- `scroll` - Scroll the show window
- `help` - Display help message
- `quit` - Exit the system

## Quick Start Guide

### Example Session

```bash
# Build the system
mkdir build && cd build
cmake ..
make

# Run CPL
./cpl

# At the CPL prompt:
cpl> read Nat                          # Load natural numbers
cpl> show object nat                   # Show the nat object definition
cpl> let add=eval.pair(pr(...))       # Define addition
cpl> simp add.pair(s.0,s.s.0)         # Compute 1 + 2
s.s.s.0                                # Result: 3

cpl> read List                         # Load lists
cpl> show object list                  # Show list definition
cpl> simp seq.s.s.s.0                 # Generate list [1,2,3]

cpl> set trace on                      # Enable tracing
cpl> simp add.pair(s.0,s.0)           # See reduction steps

cpl> help                              # Show help
cpl> quit                              # Exit
```

### Testing Natural Number Arithmetic

```lisp
cpl> read Nat
cpl> simp add.pair(s.s.0, s.s.s.0)    # 2 + 3 = 5
s.s.s.s.s.0

cpl> simp mult.pair(s.s.0, s.s.s.0)   # 2 * 3 = 6
s.s.s.s.s.s.0

cpl> simp iszero.s.s.0                 # iszero(2) = false
false

cpl> simp iszero.0                     # iszero(0) = true
true
```

### Working with Lists

```lisp
cpl> read List
cpl> simp seq.s.s.s.0                 # Generate [1,2,3]
cons.pair(s.0,cons.pair(s.s.0,cons.pair(s.s.s.0,nil.!)))

cpl> simp head.(seq.s.s.s.0)          # First element
s.0

cpl> simp tail.(seq.s.s.s.0)          # Rest of list
cons.pair(s.s.0,cons.pair(s.s.s.0,nil.!))
```

## Common Patterns

### Defining Data Types

Terminal object:
```
right object 1 with !
end object;
```

Product:
```
right object prod(a,b) with pair is
  pi1:prod->a
  pi2:prod->b
end object;
```

Natural numbers:
```
left object nat with pr is
  0:1->nat
  s:nat->nat
end object;
```

Lists:
```
left object list(p) with prl is
  nil:1->list
  cons:prod(p,list)->list
end object;
```

### Working with Morphisms

Composition uses `.` operator: `f.g` means g followed by f (reverse order)

Example morphisms:
- `add` - Addition on natural numbers
- `mult` - Multiplication
- `append` - List concatenation
- `head` - First element of list
- `tail` - Rest of list

### Simplification

Use `simp` to reduce morphisms to canonical form:
```
cpl>simp add.pair(s.0,s.s.0)
```

Enable tracing to see reduction steps:
```
cpl>set trace on
cpl>simp add.pair(s.0,s.s.0)
```

## Historical Context

This implementation demonstrates categorical programming concepts from 1985-1987, predating modern functional programming languages. The system implements:

- Initial algebras and final co-algebras
- Primitive recursion and corecursion
- Cartesian closed category operations
- Dual constructions (products/coproducts, nat/co_nat, lists/infinite lists)

The window manager component (`wm.c`) provides a sophisticated terminal multiplexing system that allowed running multiple processes in separate windows - an early precursor to modern terminal multiplexers.

## Troubleshooting

### Build Issues

**SBCL not found:**
```bash
# Install SBCL
brew install sbcl         # macOS
sudo apt install sbcl     # Ubuntu/Debian
```

**CMake version too old:**
```bash
# Upgrade CMake
brew upgrade cmake        # macOS
# Or download from cmake.org
```

**Missing ncurses:**
```bash
brew install ncurses      # macOS
sudo apt install libncurses-dev  # Ubuntu/Debian
```

**PTY/openpty errors:**
- On some systems, `openpty()` requires linking with `-lutil`
- CMake should handle this automatically
- Check `config.h` for `HAVE_PTY_H` or `HAVE_UTIL_H`

### Runtime Issues

**"undefined function" errors in SBCL:**
- Make sure all .lisp files are loaded in the correct order
- Check that franz-compat.lisp is loaded first
- Verify ASDF system definition dependencies

**Terminal not behaving correctly:**
- Ensure TERM environment variable is set
- Check that termcap database is accessible
- Try: `export TERM=xterm-256color`

**Window manager crashes:**
- Check terminal compatibility
- Verify PTY devices are accessible: `ls -la /dev/ptmx`
- On Linux, ensure `/dev/pts` is mounted

### System File Loading

**"File not found" when executing `read Nat`:**
```bash
# CPL looks for system files in:
# 1. Current directory
# 2. ./system/
# 3. /usr/local/share/cpl/system/ (if installed)

# Run from project root or set path:
cd /path/to/cpl-hagino
./build/cpl
```

### Testing

**Tests fail to run:**
```bash
# Ensure you're in the right directory
cd /path/to/cpl-hagino/test
./run-tests.sh

# Or use CMake:
cd build
ctest --verbose
```

## Known Limitations

1. **Window Manager**: The window manager (`wm`) requires a terminal with specific capabilities. Modern terminal emulators should work, but ancient or minimal terminals may not.

2. **Unicode**: The system was designed for ASCII. Unicode support is limited.

3. **Performance**: This is a research interpreter from 1987. Don't expect modern performance characteristics.

4. **Platform Support**: Tested on:
   - macOS 14+ (ARM64 and x86_64)
   - Ubuntu 20.04+ (x86_64)
   - Other POSIX systems should work but are untested

## Contributing

This is a historical modernization project. When making changes:

1. **Preserve semantics**: The categorical reduction rules must remain identical to the original
2. **Test thoroughly**: Run the test suite after any changes
3. **Document**: Update CLAUDE.md for significant changes
4. **Code style**: Follow existing conventions (Common Lisp and C11 standards)

## References

- **Original Paper**: Hagino, T. (1987). "A Categorical Programming Language". PhD Thesis, University of Edinburgh.
- **Category Theory**: Mac Lane, S. (1971). "Categories for the Working Mathematician"
- **Initial Algebras**: Lambek, J. (1968). "A fixpoint theorem for complete categories"
- **Final Coalgebras**: Aczel, P. (1988). "Non-well-founded sets"

## License

This is a historical academic implementation. The original code was developed at the University of Edinburgh (1985-1987).

The modernization (2026) maintains the academic nature of the project. Use for educational and research purposes.

## Acknowledgments

- **Tatsuya Hagino** - Original implementation (1985-1987)
- **University of Edinburgh** - Where CPL was developed
- **SBCL Development Team** - Modern Common Lisp platform
- **GNU/Linux and BSD communities** - POSIX standards and tools

---

**Version**: 4.0.0 (2026)  
**Original Version**: 3.0 (1987)  
**Language**: SBCL (Common Lisp) + ANSI C (C11)  
**Status**: Fully functional modernization
