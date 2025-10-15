# VIBE API Reference

Complete API documentation for the VIBE C parser library.

## Table of Contents

- [Parser Management](#parser-management)
- [Parsing Functions](#parsing-functions)
- [Value Creation](#value-creation)
- [Value Access](#value-access)
- [Object Operations](#object-operations)
- [Array Operations](#array-operations)
- [Memory Management](#memory-management)
- [Error Handling](#error-handling)
- [Type System](#type-system)
- [Usage Examples](#usage-examples)

---

## Parser Management

### `vibe_parser_new()`

Creates a new parser instance.

```c
VibeParser* vibe_parser_new(void);
```

**Returns:**
- Pointer to a new `VibeParser` instance
- `NULL` if memory allocation fails

**Example:**
```c
VibeParser* parser = vibe_parser_new();
if (!parser) {
    fprintf(stderr, "Failed to create parser\n");
    return 1;
}
```

**Notes:**
- Must be freed with `vibe_parser_free()` when done
- Parser can be reused for multiple parse operations
- Not thread-safe (use one parser per thread)

---

### `vibe_parser_free()`

Frees a parser instance and its resources.

```c
void vibe_parser_free(VibeParser* parser);
```

**Parameters:**
- `parser` - Parser instance to free (can be `NULL`)

**Example:**
```c
vibe_parser_free(parser);
```

**Notes:**
- Safe to call with `NULL`
- Does not free parsed values (use `vibe_value_free()` for those)
- Frees error messages if any

---

## Parsing Functions

### `vibe_parse_string()`

Parses a VIBE string into a value tree.

```c
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
```

**Parameters:**
- `parser` - Parser instance
- `input` - Null-terminated VIBE string to parse

**Returns:**
- Root `VibeValue` object containing parsed configuration
- `NULL` on parse error (check with `vibe_get_last_error()`)

**Example:**
```c
const char* config =
    "app {\n"
    "  name \"My App\"\n"
    "  port 8080\n"
    "}\n";

VibeValue* root = vibe_parse_string(parser, config);
if (!root) {
    VibeError error = vibe_get_last_error(parser);
    fprintf(stderr, "Error at line %d: %s\n", error.line, error.message);
    return 1;
}
```

**Notes:**
- Input must be valid UTF-8
- Parser keeps first error encountered
- Returned value must be freed with `vibe_value_free()`

---

### `vibe_parse_file()`

Parses a VIBE file into a value tree.

```c
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);
```

**Parameters:**
- `parser` - Parser instance
- `filename` - Path to VIBE file

**Returns:**
- Root `VibeValue` object containing parsed configuration
- `NULL` on error (file not found or parse error)

**Example:**
```c
VibeValue* config = vibe_parse_file(parser, "config.vibe");
if (!config) {
    VibeError error = vibe_get_last_error(parser);
    fprintf(stderr, "Error: %s\n", error.message);
    return 1;
}
```

**Notes:**
- Reads entire file into memory
- File must be UTF-8 encoded
- Returns detailed error for both file I/O and parse errors

---

## Value Creation

### `vibe_value_new_integer()`

Creates a new integer value.

```c
VibeValue* vibe_value_new_integer(int64_t value);
```

**Parameters:**
- `value` - 64-bit signed integer

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_INTEGER`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* port = vibe_value_new_integer(8080);
```

---

### `vibe_value_new_float()`

Creates a new floating-point value.

```c
VibeValue* vibe_value_new_float(double value);
```

**Parameters:**
- `value` - Double-precision floating-point number

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_FLOAT`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* version = vibe_value_new_float(1.5);
```

---

### `vibe_value_new_boolean()`

Creates a new boolean value.

```c
VibeValue* vibe_value_new_boolean(bool value);
```

**Parameters:**
- `value` - Boolean (`true` or `false`)

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_BOOLEAN`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* debug = vibe_value_new_boolean(true);
```

---

### `vibe_value_new_string()`

Creates a new string value.

```c
VibeValue* vibe_value_new_string(const char* value);
```

**Parameters:**
- `value` - Null-terminated UTF-8 string (copied internally)

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_STRING`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* name = vibe_value_new_string("My Application");
```

**Notes:**
- String is copied (caller retains ownership of input)
- Supports full UTF-8

---

### `vibe_value_new_array()`

Creates a new empty array.

```c
VibeValue* vibe_value_new_array(void);
```

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_ARRAY`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* servers = vibe_value_new_array();
vibe_array_push(servers->as_array, vibe_value_new_string("server1.com"));
vibe_array_push(servers->as_array, vibe_value_new_string("server2.com"));
```

---

### `vibe_value_new_object()`

Creates a new empty object.

```c
VibeValue* vibe_value_new_object(void);
```

**Returns:**
- New `VibeValue` of type `VIBE_TYPE_OBJECT`
- `NULL` if allocation fails

**Example:**
```c
VibeValue* config = vibe_value_new_object();
vibe_object_set(config->as_object, "port", vibe_value_new_integer(8080));
vibe_object_set(config->as_object, "host", vibe_value_new_string("localhost"));
```

---

## Value Access

### `vibe_get()`

Accesses a value using dot notation path.

```c
VibeValue* vibe_get(VibeValue* root, const char* path);
```

**Parameters:**
- `root` - Root value to search from
- `path` - Dot-separated path (e.g., `"server.ssl.enabled"`)

**Returns:**
- Pointer to the value at the path
- `NULL` if path not found

**Example:**
```c
VibeValue* ssl_enabled = vibe_get(config, "server.ssl.enabled");
if (ssl_enabled && ssl_enabled->type == VIBE_TYPE_BOOLEAN) {
    printf("SSL: %s\n", ssl_enabled->as_boolean ? "enabled" : "disabled");
}
```

**Notes:**
- Path is case-sensitive
- Returns borrowed reference (don't free)
- `NULL` path returns the root value itself

---

### `vibe_get_string()`

Gets a string value from a path.

```c
const char* vibe_get_string(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- String value at the path
- `NULL` if not found or not a string

**Example:**
```c
const char* app_name = vibe_get_string(config, "app.name");
if (app_name) {
    printf("Application: %s\n", app_name);
}

// Direct access (no path)
VibeValue* val = vibe_get(config, "app.name");
const char* name = vibe_get_string(val, NULL);
```

**Notes:**
- Returns borrowed pointer (don't free)
- Returns `NULL` if value is not a string
- Path can be `NULL` to access value directly

---

### `vibe_get_int()`

Gets an integer value from a path.

```c
int64_t vibe_get_int(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- Integer value at the path
- `0` if not found or not an integer

**Example:**
```c
int64_t port = vibe_get_int(config, "server.port");
printf("Port: %lld\n", (long long)port);

// Check if value exists and is correct type
VibeValue* port_val = vibe_get(config, "server.port");
if (port_val && port_val->type == VIBE_TYPE_INTEGER) {
    printf("Port: %lld\n", (long long)port_val->as_integer);
} else {
    printf("Port not configured or wrong type\n");
}
```

**Notes:**
- Returns `0` on error (check type if `0` is a valid value)
- Use `vibe_get()` first if you need to distinguish missing vs. zero

---

### `vibe_get_float()`

Gets a float value from a path.

```c
double vibe_get_float(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- Float value at the path
- `0.0` if not found or not a float

**Example:**
```c
double timeout = vibe_get_float(config, "server.timeout");
printf("Timeout: %.2f seconds\n", timeout);
```

---

### `vibe_get_bool()`

Gets a boolean value from a path.

```c
bool vibe_get_bool(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- Boolean value at the path
- `false` if not found or not a boolean

**Example:**
```c
bool debug = vibe_get_bool(config, "app.debug");
if (debug) {
    printf("Debug mode enabled\n");
}
```

**Notes:**
- Returns `false` on error
- Check type if `false` is a meaningful value

---

### `vibe_get_array()`

Gets an array from a path.

```c
VibeArray* vibe_get_array(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- Pointer to `VibeArray` structure
- `NULL` if not found or not an array

**Example:**
```c
VibeArray* servers = vibe_get_array(config, "servers");
if (servers) {
    for (size_t i = 0; i < servers->count; i++) {
        VibeValue* server = servers->values[i];
        if (server->type == VIBE_TYPE_STRING) {
            printf("Server %zu: %s\n", i, server->as_string);
        }
    }
}
```

---

### `vibe_get_object()`

Gets an object from a path.

```c
VibeObject* vibe_get_object(VibeValue* value, const char* path);
```

**Parameters:**
- `value` - Root value to search from
- `path` - Dot-separated path (or `NULL` for direct access)

**Returns:**
- Pointer to `VibeObject` structure
- `NULL` if not found or not an object

**Example:**
```c
VibeObject* ssl = vibe_get_object(config, "server.ssl");
if (ssl) {
    for (size_t i = 0; i < ssl->count; i++) {
        printf("%s: ", ssl->entries[i].key);
        vibe_value_print(ssl->entries[i].value, 0);
        printf("\n");
    }
}
```

---

## Object Operations

### `vibe_object_set()`

Sets a key-value pair in an object.

```c
void vibe_object_set(VibeObject* obj, const char* key, VibeValue* value);
```

**Parameters:**
- `obj` - Object to modify
- `key` - Key name (will be copied)
- `value` - Value to set (ownership transferred)

**Example:**
```c
VibeObject* config = vibe_value_new_object()->as_object;
vibe_object_set(config, "port", vibe_value_new_integer(8080));
vibe_object_set(config, "host", vibe_value_new_string("localhost"));
```

**Notes:**
- If key exists, replaces old value (old value is freed)
- Key is copied (caller retains ownership)
- Value ownership transfers to object

---

### `vibe_object_get()`

Gets a value from an object by key.

```c
VibeValue* vibe_object_get(VibeObject* obj, const char* key);
```

**Parameters:**
- `obj` - Object to search
- `key` - Key name

**Returns:**
- Value associated with key
- `NULL` if key not found

**Example:**
```c
VibeValue* port = vibe_object_get(config->as_object, "port");
if (port && port->type == VIBE_TYPE_INTEGER) {
    printf("Port: %lld\n", (long long)port->as_integer);
}
```

---

## Array Operations

### `vibe_array_push()`

Appends a value to an array.

```c
void vibe_array_push(VibeArray* arr, VibeValue* value);
```

**Parameters:**
- `arr` - Array to modify
- `value` - Value to append (ownership transferred)

**Example:**
```c
VibeArray* servers = vibe_value_new_array()->as_array;
vibe_array_push(servers, vibe_value_new_string("server1.com"));
vibe_array_push(servers, vibe_value_new_string("server2.com"));
vibe_array_push(servers, vibe_value_new_string("server3.com"));
```

---

### `vibe_array_get()`

Gets a value from an array by index.

```c
VibeValue* vibe_array_get(VibeArray* arr, size_t index);
```

**Parameters:**
- `arr` - Array to access
- `index` - Zero-based index

**Returns:**
- Value at index
- `NULL` if index out of bounds

**Example:**
```c
VibeArray* servers = vibe_get_array(config, "servers");
if (servers) {
    VibeValue* first = vibe_array_get(servers, 0);
    if (first && first->type == VIBE_TYPE_STRING) {
        printf("First server: %s\n", first->as_string);
    }
}
```

---

## Memory Management

### `vibe_value_free()`

Recursively frees a value and all its children.

```c
void vibe_value_free(VibeValue* value);
```

**Parameters:**
- `value` - Value to free (can be `NULL`)

**Example:**
```c
VibeValue* config = vibe_parse_file(parser, "config.vibe");
// Use config...
vibe_value_free(config);  // Frees entire tree
```

**Notes:**
- Recursively frees nested objects and arrays
- Safe to call with `NULL`
- Frees all strings, arrays, and objects in the tree

---

## Error Handling

### `vibe_get_last_error()`

Gets the last error from a parser.

```c
VibeError vibe_get_last_error(VibeParser* parser);
```

**Parameters:**
- `parser` - Parser instance

**Returns:**
- `VibeError` structure with error information

**Example:**
```c
VibeValue* config = vibe_parse_file(parser, "config.vibe");
if (!config) {
    VibeError error = vibe_get_last_error(parser);
    if (error.has_error) {
        fprintf(stderr, "Parse error at line %d, column %d:\n",
                error.line, error.column);
        fprintf(stderr, "  %s\n", error.message);
    }
}
```

**VibeError Structure:**
```c
typedef struct {
    bool has_error;      // True if error occurred
    char* message;       // Error message (owned by parser)
    int line;            // Line number (1-indexed)
    int column;          // Column number (1-indexed)
} VibeError;
```

---

### `vibe_error_free()`

Frees error message memory.

```c
void vibe_error_free(VibeError* error);
```

**Parameters:**
- `error` - Error structure to free

**Example:**
```c
// Usually not needed - errors are freed with parser
VibeError error = vibe_get_last_error(parser);
// Use error...
vibe_error_free(&error);  // Only if you need to free early
```

**Notes:**
- Usually not needed - errors freed with parser
- Only use if you need to free error before parser

---

## Type System

### VibeType Enum

```c
typedef enum {
    VIBE_TYPE_NULL = 0,
    VIBE_TYPE_INTEGER,
    VIBE_TYPE_FLOAT,
    VIBE_TYPE_BOOLEAN,
    VIBE_TYPE_STRING,
    VIBE_TYPE_ARRAY,
    VIBE_TYPE_OBJECT
} VibeType;
```

### VibeValue Structure

```c
struct VibeValue {
    VibeType type;
    union {
        int64_t as_integer;
        double as_float;
        bool as_boolean;
        char* as_string;
        VibeArray* as_array;
        VibeObject* as_object;
    };
};
```

**Usage:**
```c
VibeValue* val = vibe_get(config, "server.port");
switch (val->type) {
    case VIBE_TYPE_INTEGER:
        printf("Port: %lld\n", (long long)val->as_integer);
        break;
    case VIBE_TYPE_STRING:
        printf("Port: %s\n", val->as_string);
        break;
    default:
        printf("Port has unexpected type\n");
}
```

---

## Usage Examples

### Complete Example

```c
#include "vibe.h"
#include <stdio.h>

int main() {
    // 1. Create parser
    VibeParser* parser = vibe_parser_new();
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        return 1;
    }

    // 2. Parse file
    VibeValue* config = vibe_parse_file(parser, "config.vibe");
    if (!config) {
        VibeError error = vibe_get_last_error(parser);
        fprintf(stderr, "Error at line %d: %s\n",
                error.line, error.message);
        vibe_parser_free(parser);
        return 1;
    }

    // 3. Access values using dot notation
    const char* app_name = vibe_get_string(config, "app.name");
    int64_t port = vibe_get_int(config, "server.port");
    bool debug = vibe_get_bool(config, "app.debug");

    printf("Application: %s\n", app_name ? app_name : "Unknown");
    printf("Port: %lld\n", (long long)port);
    printf("Debug: %s\n", debug ? "enabled" : "disabled");

    // 4. Access arrays
    VibeArray* servers = vibe_get_array(config, "servers");
    if (servers) {
        printf("Servers:\n");
        for (size_t i = 0; i < servers->count; i++) {
            VibeValue* server = servers->values[i];
            if (server->type == VIBE_TYPE_STRING) {
                printf("  - %s\n", server->as_string);
            }
        }
    }

    // 5. Access nested objects
    VibeObject* ssl = vibe_get_object(config, "server.ssl");
    if (ssl) {
        printf("SSL Configuration:\n");
        for (size_t i = 0; i < ssl->count; i++) {
            printf("  %s = ", ssl->entries[i].key);
            vibe_value_print(ssl->entries[i].value, 0);
            printf("\n");
        }
    }

    // 6. Cleanup
    vibe_value_free(config);
    vibe_parser_free(parser);

    return 0;
}
```

### Building Configs Programmatically

```c
// Create a config from scratch
VibeValue* config = vibe_value_new_object();

// Add simple values
vibe_object_set(config->as_object, "port", vibe_value_new_integer(8080));
vibe_object_set(config->as_object, "host", vibe_value_new_string("localhost"));
vibe_object_set(config->as_object, "debug", vibe_value_new_boolean(true));

// Add nested object
VibeValue* ssl = vibe_value_new_object();
vibe_object_set(ssl->as_object, "enabled", vibe_value_new_boolean(true));
vibe_object_set(ssl->as_object, "cert", vibe_value_new_string("/etc/ssl/cert.pem"));
vibe_object_set(config->as_object, "ssl", ssl);

// Add array
VibeValue* servers = vibe_value_new_array();
vibe_array_push(servers->as_array, vibe_value_new_string("server1.com"));
vibe_array_push(servers->as_array, vibe_value_new_string("server2.com"));
vibe_object_set(config->as_object, "servers", servers);

// Use config...

vibe_value_free(config);
```

---

## Debug Helper

### `vibe_value_print()`

Prints a value tree (for debugging).

```c
void vibe_value_print(VibeValue* value, int indent);
```

**Parameters:**
- `value` - Value to print
- `indent` - Indentation level (use 0 for root)

**Example:**
```c
VibeValue* config = vibe_parse_file(parser, "config.vibe");
printf("Configuration:\n");
vibe_value_print(config, 0);
```

---

## Best Practices

1. **Always check return values:**
   ```c
   VibeValue* config = vibe_parse_file(parser, "config.vibe");
   if (!config) {
       // Handle error
   }
   ```

2. **Check types before accessing:**
   ```c
   VibeValue* port = vibe_get(config, "server.port");
   if (port && port->type == VIBE_TYPE_INTEGER) {
       use_port(port->as_integer);
   }
   ```

3. **Always free resources:**
   ```c
   vibe_value_free(config);
   vibe_parser_free(parser);
   ```

4. **Use dot notation for clarity:**
   ```c
   // Good
   const char* name = vibe_get_string(config, "app.name");

   // Also works but more verbose
   VibeObject* app = vibe_get_object(config, "app");
   VibeValue* name_val = vibe_object_get(app, "name");
   ```

---

For more examples, see the [examples/](../examples/) directory.
