# Installation Guide

This guide covers how to install and use the VIBE parser in your projects.

## Quick Start

### From Source

```bash
git clone https://github.com/1ay1/vibe.git
cd vibe
make
```

The `vibe_example` executable will be built and ready to use.

## Usage Options

### Option 1: Direct Integration (Recommended)

Copy the parser files directly into your project:

```bash
cp vibe.h vibe.c /path/to/your/project/
```

Then compile them with your project:

```bash
gcc -std=c11 -c vibe.c
gcc -std=c11 -o myapp myapp.c vibe.o
```

**Pros:**
- No system-wide installation needed
- Easy to track specific versions
- Simple dependency management
- Works on all platforms

### Option 2: System Installation

Install the library system-wide (requires root/sudo):

```bash
make
sudo make install
```

This installs:
- `/usr/local/include/vibe.h` - Header file
- `/usr/local/lib/libvibe.a` - Static library

Then link against the installed library:

```bash
gcc -std=c11 -o myapp myapp.c -lvibe
```

**Pros:**
- Available system-wide
- Standard library location
- Easy linking

**Cons:**
- Requires sudo/root
- System-wide changes
- Version conflicts possible

### Option 3: Local Build Directory

Keep the VIBE parser in a separate directory and link to it:

```bash
# Build VIBE
cd /path/to/vibe
make

# Build your project
cd /path/to/your/project
gcc -std=c11 -I/path/to/vibe -c myapp.c
gcc -std=c11 -o myapp myapp.o /path/to/vibe/vibe.o
```

## Requirements

### Compiler Requirements

- **C11 compliant compiler**
  - GCC 4.9+
  - Clang 3.1+
  - MSVC 2015+ (with C11 support)

- **POSIX environment** (for `strdup`)
  - Linux: Native support
  - macOS: Native support
  - Windows: MinGW, Cygwin, or WSL

### Build Tools

- Make (GNU Make recommended)
- ar (for static library creation)

## Platform-Specific Instructions

### Linux (Ubuntu/Debian)

```bash
# Install build tools if needed
sudo apt-get update
sudo apt-get install build-essential

# Build VIBE
git clone https://github.com/1ay1/vibe.git
cd vibe
make

# Run tests
make test

# Optional: Install system-wide
sudo make install
```

### macOS

```bash
# Xcode Command Line Tools provide gcc/clang
xcode-select --install

# Build VIBE
git clone https://github.com/1ay1/vibe.git
cd vibe
make

# Run tests
make test

# Optional: Install system-wide
sudo make install
```

### Windows

#### Option 1: MinGW/MSYS2

```bash
# Install MSYS2 from https://www.msys2.org/
# In MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc make

# Build VIBE
git clone https://github.com/1ay1/vibe.git
cd vibe
make
```

#### Option 2: WSL (Windows Subsystem for Linux)

```bash
# Follow Linux instructions in WSL
wsl
git clone https://github.com/1ay1/vibe.git
cd vibe
make
```

#### Option 3: Visual Studio

Create a new C project and add `vibe.c` and `vibe.h` to your project files. No Makefile needed.

## Using VIBE in Your Project

### Basic Usage

```c
#include "vibe.h"
#include <stdio.h>

int main() {
    // Create parser
    VibeParser* parser = vibe_parser_new();
    
    // Parse file
    VibeValue* config = vibe_parse_file(parser, "config.vibe");
    
    if (!config) {
        VibeError error = vibe_get_last_error(parser);
        fprintf(stderr, "Error: %s\n", error.message);
        vibe_parser_free(parser);
        return 1;
    }
    
    // Use configuration
    const char* name = vibe_get_string(config, "app.name");
    printf("App name: %s\n", name);
    
    // Cleanup
    vibe_value_free(config);
    vibe_parser_free(parser);
    
    return 0;
}
```

### CMake Integration

If you use CMake, add this to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject C)

set(CMAKE_C_STANDARD 11)

# Add VIBE parser
add_library(vibe STATIC vibe.c)
target_include_directories(vibe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Your executable
add_executable(myapp main.c)
target_link_libraries(myapp vibe)
```

### Meson Integration

For Meson build system, add to `meson.build`:

```meson
project('myproject', 'c', version: '1.0', default_options: ['c_std=c11'])

vibe_lib = static_library('vibe', 'vibe.c')
vibe_dep = declare_dependency(link_with: vibe_lib, include_directories: '.')

executable('myapp', 'main.c', dependencies: vibe_dep)
```

## Verification

### Test Installation

```bash
# Build the example
make

# Run tests
make test

# Should output:
# === Running VIBE Parser Tests ===
# Testing simple.vibe...
# Parse successful!
# ...
# ✓ All tests passed!
```

### Check Library Installation (if installed system-wide)

```bash
# Check header
ls -l /usr/local/include/vibe.h

# Check library
ls -l /usr/local/lib/libvibe.a

# Test linking
echo '#include <vibe.h>
int main() { return 0; }' | gcc -xc - -lvibe -o test && ./test
```

## Troubleshooting

### "strdup not declared"

Add before includes:
```c
#define _POSIX_C_SOURCE 200809L
#include "vibe.h"
```

Or compile with:
```bash
gcc -D_POSIX_C_SOURCE=200809L -std=c11 myapp.c vibe.c
```

### "cannot find -lvibe"

If you installed system-wide but get linking errors:

```bash
# Update library cache (Linux)
sudo ldconfig

# Or specify library path explicitly
gcc myapp.c -L/usr/local/lib -lvibe
```

### Compilation Warnings

Ensure you're using C11:
```bash
gcc -std=c11 -Wall -Wextra myapp.c vibe.c
```

## Uninstallation

### Remove System Installation

```bash
sudo rm /usr/local/include/vibe.h
sudo rm /usr/local/lib/libvibe.a
```

### Remove Build Files

```bash
make clean
```

## Next Steps

- Read the [README.md](README.md) for API documentation
- Check out [examples/](examples/) for usage examples
- Review [SPECIFICATION.md](SPECIFICATION.md) for format details
- See [CONTRIBUTING.md](CONTRIBUTING.md) to contribute

## Support

- 📖 Documentation: See README.md
- 🐛 Issues: https://github.com/1ay1/vibe/issues
- 💬 Discussions: https://github.com/1ay1/vibe/discussions

---

Keep calm and VIBE on! 🌊
