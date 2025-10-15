# VIBE 🌊

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Version](https://img.shields.io/badge/version-1.0-green.svg)](https://github.com/1ay1/vibe)

**VIBE** (Values In Bracket Expression) is a hierarchical configuration file format designed for human readability and fast machine parsing. It combines the visual clarity of structured formats with the simplicity of minimal syntax.

> Like its namesake, VIBE aims to create good vibes in your development workflow by eliminating the frustration of complex configuration formats. No more YAML indentation nightmares or JSON comma catastrophes - just smooth, readable config that flows naturally.

## ✨ Features

- 🎯 **Simple Syntax** - Only 5 token types, minimal punctuation
- 👀 **Visual Hierarchy** - Structure is immediately apparent
- ⚡ **Fast Parsing** - Single-pass O(n) with no backtracking
- 🔒 **Unambiguous** - One canonical way to represent data
- 💬 **Comments** - Built-in support with `#`
- 🌍 **Unicode Support** - UTF-8 strings with proper escaping
- 🎨 **Flexible Whitespace** - No significant indentation rules
- 🔧 **Type Inference** - Automatic detection of integers, floats, booleans, strings

## 📖 Quick Example

```vibe
# Application configuration
application {
  name "My Awesome App"
  version 2.1.0
  debug false
}

server {
  host localhost
  port 8080

  ssl {
    enabled true
    cert_path /etc/ssl/cert.pem
  }
}

# Server pool
servers [
  prod1.example.com
  prod2.example.com
  prod3.example.com
]

# Feature flags
features {
  new_ui true
  beta_api false
  analytics true
}
```

## 🚀 Quick Start

### Installation

```bash
git clone https://github.com/1ay1/vibe.git
cd vibe
make
```

### Basic Usage

```c
#include "vibe.h"

int main() {
    VibeParser* parser = vibe_parser_new();
    VibeValue* config = vibe_parse_file(parser, "config.vibe");

    if (!config) {
        VibeError error = vibe_get_last_error(parser);
        fprintf(stderr, "Error: %s\n", error.message);
        return 1;
    }

    // Access values with dot notation
    const char* name = vibe_get_string(config, "application.name");
    int64_t port = vibe_get_int(config, "server.port");
    bool debug = vibe_get_bool(config, "application.debug");

    printf("App: %s, Port: %lld, Debug: %s\n",
           name, port, debug ? "true" : "false");

    vibe_value_free(config);
    vibe_parser_free(parser);
    return 0;
}
```

Compile and run:

```bash
gcc -o myapp myapp.c vibe.c -std=c11
./myapp
```

## 📚 Documentation

### Syntax Overview

#### Simple Assignment
```vibe
key value
```

#### Objects
```vibe
database {
  host localhost
  port 5432
}
```

#### Arrays
```vibe
ports [8080 8081 8082]

servers [
  server1.com
  server2.com
  server3.com
]
```

#### Data Types

- **Integers**: `42`, `-17`
- **Floats**: `3.14`, `-0.5`
- **Booleans**: `true`, `false`
- **Strings**: `"quoted"` or `unquoted` (for simple values)

#### Comments
```vibe
# Full line comment
key value  # Inline comment
```

#### String Escaping
```vibe
message "Hello \"World\""
path "C:\\Users\\Name"
unicode "Hello 世界"
```

### API Reference

#### Parser Management
```c
VibeParser* vibe_parser_new(void);
void vibe_parser_free(VibeParser* parser);
```

#### Parsing Functions
```c
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);
```

#### Value Access
```c
VibeValue* vibe_get(VibeValue* root, const char* path);
const char* vibe_get_string(VibeValue* value, const char* path);
int64_t vibe_get_int(VibeValue* value, const char* path);
double vibe_get_float(VibeValue* value, const char* path);
bool vibe_get_bool(VibeValue* value, const char* path);
VibeArray* vibe_get_array(VibeValue* value, const char* path);
VibeObject* vibe_get_object(VibeValue* value, const char* path);
```

#### Object & Array Operations
```c
void vibe_object_set(VibeObject* obj, const char* key, VibeValue* value);
VibeValue* vibe_object_get(VibeObject* obj, const char* key);
void vibe_array_push(VibeArray* arr, VibeValue* value);
VibeValue* vibe_array_get(VibeArray* arr, size_t index);
```

#### Cleanup
```c
void vibe_value_free(VibeValue* value);
```

#### Error Handling
```c
VibeError vibe_get_last_error(VibeParser* parser);
void vibe_error_free(VibeError* error);
```

### Full Specification

For the complete format specification, see [SPECIFICATION.md](SPECIFICATION.md).

## 🎯 Why VIBE?

| Feature | VIBE | JSON | YAML | TOML |
|---------|------|------|------|------|
| Human Readable | ✅ | ❌ | ✅ | ✅ |
| Minimal Syntax | ✅ | ❌ | ✅ | ✅ |
| Visual Hierarchy | ✅ | ✅ | ❌ | ❌ |
| Fast Parsing | ✅ | ✅ | ❌ | ✅ |
| No Indentation Rules | ✅ | ✅ | ❌ | ✅ |
| Comments | ✅ | ❌ | ✅ | ✅ |
| Single Pass Parse | ✅ | ✅ | ❌ | ✅ |
| No Reserved Words | ✅ | ❌ | ❌ | ❌ |

**Choose VIBE when:**
- Configuration files need to be human-readable and editable
- Fast parsing is important
- Visual structure clarity is valued
- Simple syntax is preferred
- You want to avoid indentation sensitivity (YAML) or verbosity (JSON/XML)

## 🔨 Building & Testing

### Build
```bash
make                 # Build everything
make clean           # Clean build artifacts
```

### Run Examples
```bash
make demo            # Quick demo
make test            # Run all tests
./vibe_example simple.vibe
./vibe_example config.vibe
```

### Integration

To use VIBE in your C project:

1. Copy `vibe.h` and `vibe.c` to your project
2. Add `vibe.c` to your build system
3. Include `vibe.h` in your source files
4. Link and compile with C11 support

```bash
gcc -std=c11 -c vibe.c
gcc -std=c11 -o myapp myapp.c vibe.o
```

## 📋 Examples

Check out the `examples/` directory for more usage examples:
- `simple.vibe` - Basic configuration
- `config.vibe` - Complex nested structure
- `example.c` - Complete C usage example

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Areas for Contribution
- Additional language implementations (Python, Rust, Go, JavaScript)
- Schema validation tools
- Editor plugins (VS Code, Vim, Emacs)
- Conversion tools (JSON↔VIBE, YAML↔VIBE, TOML↔VIBE)
- Performance optimizations
- More comprehensive test suite

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🗺️ Roadmap

- [x] Core parser implementation (C)
- [x] Complete specification
- [ ] Python bindings
- [ ] Rust implementation
- [ ] JavaScript/Node.js implementation
- [ ] Go implementation
- [ ] VS Code syntax highlighting extension
- [ ] Schema validation support
- [ ] Format converter tools
- [ ] Benchmark suite

## 📞 Support

- 📖 Read the [Specification](SPECIFICATION.md)
- 💬 Open an [Issue](https://github.com/1ay1/vibe/issues)
- 🌟 Star the project if you find it useful!

## 🎉 Acknowledgments

Inspired by the need for a configuration format that's:
- Simpler than YAML
- More readable than JSON
- Faster to parse than both
- More flexible than TOML

---

**Keep calm and VIBE on!** 🌊

*Configuration doesn't have to be complicated. Sometimes the best solution is the one that just feels right.*
