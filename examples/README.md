# VIBE Examples

This directory contains example VIBE configuration files and usage examples.

## Configuration Files

### simple.vibe
Basic VIBE file demonstrating core features:
- Simple key-value pairs
- Nested objects
- Arrays

### config.vibe
Complex real-world configuration showing:
- Deep nesting
- Multiple data types
- Arrays and objects
- Comments
- Path-style values

### web_server.vibe
Web server configuration example with:
- Virtual hosts
- SSL configuration
- Logging settings
- Rate limiting

### database.vibe
Database configuration showing:
- Connection pooling
- Replica sets
- Migration settings

## C Examples

### example.c
Complete working example showing how to:
- Parse VIBE files
- Access values using dot notation
- Handle errors
- Print configuration structure

## Running Examples

```bash
cd ..
make
./vibe_example examples/simple.vibe
./vibe_example examples/config.vibe
./vibe_example examples/web_server.vibe
```

## Creating Your Own

1. Create a `.vibe` file with your configuration
2. Use the parser to load it
3. Access values with `vibe_get_*()` functions

See `example.c` for a complete reference implementation.
