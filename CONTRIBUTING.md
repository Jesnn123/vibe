# Contributing to VIBE

First off, thank you for considering contributing to VIBE! 🎉

The VIBE format aims to bring good vibes to configuration files, and we welcome contributions that align with this philosophy.

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on constructive feedback
- Keep the vibe positive! 🌊

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates.

When creating a bug report, include:
- **Clear title** describing the issue
- **Steps to reproduce** the problem
- **Expected behavior** vs actual behavior
- **VIBE file** that causes the issue (if applicable)
- **Environment details** (OS, compiler version, etc.)
- **Error messages** or stack traces

Example:
```markdown
## Bug: Parser crashes on empty arrays

**Steps to reproduce:**
1. Create file with: `items []`
2. Run `vibe_example test.vibe`

**Expected:** Parse successfully with empty array
**Actual:** Segmentation fault

**Environment:** Ubuntu 22.04, GCC 11.3
```

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues.

Include:
- **Clear use case** - why is this needed?
- **Proposed solution** - how should it work?
- **Alternatives considered** - what other approaches did you think about?
- **Backward compatibility** - does this break existing code?

### Pull Requests

1. **Fork** the repository
2. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/amazing-feature
   ```
3. **Make your changes**
4. **Test thoroughly**
5. **Commit** with clear messages:
   ```bash
   git commit -m "Add feature: Support for hexadecimal numbers"
   ```
6. **Push** to your fork:
   ```bash
   git push origin feature/amazing-feature
   ```
7. **Open a Pull Request**

## Development Setup

### Prerequisites

- GCC 11+ or Clang 13+ (C11 support required)
- Make
- Git

### Building

```bash
git clone https://github.com/1ay1/vibe.git
cd vibe
make
```

### Testing

```bash
make test
```

Run the example program:
```bash
./vibe_example simple.vibe
./vibe_example config.vibe
```

### Code Style

We follow a consistent C coding style:

#### Naming Conventions
- **Functions**: `snake_case` with `vibe_` prefix for public API
  - `vibe_parse_file()`, `vibe_get_string()`
- **Types**: `PascalCase` with `Vibe` prefix
  - `VibeValue`, `VibeParser`, `VibeArray`
- **Constants**: `UPPER_SNAKE_CASE`
  - `MAX_NESTING_DEPTH`, `TOKEN_EOF`
- **Private functions**: `snake_case` with `static` keyword
  - `static Token next_token(...)`

#### Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style
  ```c
  if (condition) {
      do_something();
  } else {
      do_other();
  }
  ```
- **Line length**: Aim for 100 characters max
- **Comments**: Use `/* */` for multi-line, `//` for single line

#### Example
```c
/* Parse a value from a token */
static VibeValue* parse_value_from_token(Token* token) {
    if (!token) return NULL;
    
    switch (token->type) {
        case TOKEN_BOOLEAN:
            return vibe_value_new_boolean(strcmp(token->value, "true") == 0);
        
        case TOKEN_NUMBER: {
            if (strchr(token->value, '.')) {
                return vibe_value_new_float(atof(token->value));
            }
            return vibe_value_new_integer(atoll(token->value));
        }
        
        default:
            return NULL;
    }
}
```

### Memory Management

- **Always** free allocated memory
- Use `calloc()` for zero-initialization
- Document ownership in function comments
- No memory leaks - test with valgrind:
  ```bash
  valgrind --leak-check=full ./vibe_example config.vibe
  ```

### Documentation

- Document all public API functions in `vibe.h`
- Use clear, concise comments
- Update README.md for user-facing changes
- Update SPECIFICATION.md for format changes

Example API documentation:
```c
/**
 * Parse a VIBE configuration file.
 * 
 * @param parser The parser instance
 * @param filename Path to the .vibe file
 * @return Parsed configuration root, or NULL on error
 * 
 * On error, use vibe_get_last_error() to retrieve error details.
 * Caller is responsible for freeing the returned value with vibe_value_free().
 */
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);
```

## Testing Guidelines

### Test Categories

1. **Unit Tests** - Individual functions
2. **Integration Tests** - End-to-end parsing
3. **Error Tests** - Invalid input handling
4. **Edge Cases** - Boundary conditions

### Creating Test Files

Add test cases to the `examples/` directory:

```vibe
# test_feature.vibe
# Tests: New feature description

test_value 42
nested {
  key value
}
```

### Test Checklist

- [ ] Compiles without warnings (`-Wall -Wextra`)
- [ ] Passes all existing tests
- [ ] No memory leaks (valgrind)
- [ ] Handles errors gracefully
- [ ] Works on Linux, macOS, and Windows (if possible)

## Areas for Contribution

### High Priority
- 🐛 **Bug fixes** - especially memory leaks, crashes
- 📝 **Documentation improvements** - examples, guides, comments
- 🧪 **More tests** - edge cases, error conditions
- 🚀 **Performance** - optimization opportunities

### Medium Priority
- 🔧 **Editor support** - syntax highlighting for VS Code, Vim, etc.
- 🔄 **Conversion tools** - JSON↔VIBE, YAML↔VIBE
- 📊 **Schema validation** - define and validate structure
- 🌍 **Internationalization** - better Unicode support

### New Implementations
We'd love implementations in other languages:
- 🐍 Python
- 🦀 Rust  
- 🐹 Go
- 🟨 JavaScript/TypeScript
- ☕ Java
- 💎 Ruby
- 🐘 PHP

Each implementation should:
- Follow the [SPECIFICATION.md](SPECIFICATION.md)
- Include comprehensive tests
- Provide clear documentation
- Maintain the VIBE philosophy (simple, fast, readable)

## Specification Changes

Changes to the VIBE format specification require:

1. **Rationale** - clear explanation of why it's needed
2. **Backward compatibility** - how it affects existing files
3. **Reference implementation** - working code demonstrating the change
4. **Community discussion** - open an issue first
5. **Documentation** - update SPECIFICATION.md

## Recognition

Contributors will be:
- Listed in the README.md acknowledgments
- Mentioned in CHANGELOG.md
- Credited in release notes
- Granted eternal good vibes 🌊

## Questions?

- 💬 Open an [Issue](https://github.com/1ay1/vibe/issues)
- 📧 Email the maintainers
- 🌟 Check existing [Discussions](https://github.com/1ay1/vibe/discussions)

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

Thank you for helping make configuration files vibe better! 🎉

*Keep calm and VIBE on!* 🌊
