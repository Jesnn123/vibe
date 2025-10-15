---
layout: default
title: VIBE Specification
description: Complete technical specification for the VIBE configuration language
permalink: /specification/
---

# VIBE Language Specification

<div class="spec-intro">
  <p class="lead">
    This document provides the complete technical specification for VIBE (Visual, Intuitive, Basic, Expressive), 
    a human-friendly configuration language designed for clarity and ease of use.
  </p>
</div>

## Table of Contents

1. [Overview](#overview)
2. [Design Goals](#design-goals)
3. [Lexical Structure](#lexical-structure)
4. [Grammar](#grammar)
5. [Data Types](#data-types)
6. [Syntax Rules](#syntax-rules)
7. [Comments](#comments)
8. [String Literals](#string-literals)
9. [Path Notation](#path-notation)
10. [Error Handling](#error-handling)
11. [Implementation Guidelines](#implementation-guidelines)

## Overview {#overview}

VIBE is a configuration language that prioritizes human readability and writing comfort while maintaining parsing efficiency. It draws inspiration from the best aspects of JSON, YAML, and TOML while avoiding their common pitfalls.

### Key Characteristics

- **Whitespace-based separation** (no commas required)
- **Automatic type inference** from value appearance
- **Hierarchical structure** with unlimited nesting
- **Comment support** anywhere in the file
- **Unicode support** in quoted strings
- **Minimal punctuation** for clean appearance

## Design Goals {#design-goals}

### 1. Visual Hierarchy
Configuration structure should be immediately apparent from visual layout, using indentation and nesting to show relationships.

### 2. Minimal Syntax
Reduce syntactic noise by eliminating unnecessary punctuation like commas, semicolons, and excessive quotes.

### 3. Unambiguous Grammar
The language grammar should be deterministic and free from parsing conflicts, enabling fast and reliable parsing.

### 4. Fast Parsing
Simple lexical rules and grammar structure optimize for parsing speed, suitable for large configuration files.

### 5. Type Safety
Automatic type inference with clear, predictable rules prevents common configuration errors.

### 6. Human-Friendly
Prioritize readability and writability for humans over machine optimization.

## Lexical Structure {#lexical-structure}

### Character Encoding
VIBE files must be encoded in UTF-8. Unicode characters are supported in:
- Quoted strings
- Comments
- **Not supported** in identifiers (ASCII only for performance)

### Whitespace
- **Spaces** (U+0020)
- **Horizontal tabs** (U+0009)
- **Carriage returns** (U+000D)
- **Line feeds** (U+000A)

Whitespace is significant as a separator but not for structure (unlike Python or YAML).

### Character Classes

```
ALPHA     = [a-zA-Z]
DIGIT     = [0-9]
ALNUM     = ALPHA | DIGIT
IDENT_START = ALPHA | "_"
IDENT_CHAR  = ALNUM | "_" | "-"
UNQUOTED_CHAR = [0x21-0x7E] - [{}[]#] - WHITESPACE
```

### Tokens

VIBE recognizes these token types:

1. **IDENTIFIER** - `[a-zA-Z_][a-zA-Z0-9_-]*`
2. **STRING** - Quoted or unquoted text
3. **LEFT_BRACE** - `{`
4. **RIGHT_BRACE** - `}`
5. **LEFT_BRACKET** - `[`
6. **RIGHT_BRACKET** - `]`
7. **COMMENT** - `#` to end of line

## Grammar {#grammar}

### Formal Grammar (EBNF)

```ebnf
config        = { statement } ;
statement     = ( assignment | object | array | comment ) [ comment ] newline ;
assignment    = identifier scalar_value ;
object        = identifier "{" { statement } "}" ;
array         = identifier "[" [ value_list ] "]" ;
value_list    = scalar_value { ( whitespace | newline ) scalar_value } ;
scalar_value  = string | number | boolean ;
comment       = "#" { any_char - newline } ;
identifier    = ( letter | "_" ) { letter | digit | "_" | "-" } ;
string        = quoted_string | unquoted_string ;
quoted_string = '"' { char | escape_sequence } '"' ;
unquoted_string = unquoted_char { unquoted_char } ;
number        = [ "-" ] ( integer | float ) ;
integer       = digit { digit } ;
float         = digit { digit } "." digit { digit } ;
boolean       = "true" | "false" ;
escape_sequence = "\" ( '"' | "\" | "n" | "t" | "r" | "u" hex_digit hex_digit hex_digit hex_digit ) ;
```

### Parsing Precedence

1. **Comments** - Recognized first, consume to end of line
2. **Quoted strings** - Delimited content parsed as-is
3. **Structural tokens** - Braces and brackets
4. **Identifiers** - ASCII alphanumeric + underscore + hyphen
5. **Unquoted strings** - Everything else

## Data Types {#data-types}

VIBE supports five fundamental data types with automatic inference:

### Type Inference Algorithm

```
function inferType(value):
    stripped = strip_whitespace(value)
    
    if starts_with(stripped, '"') and ends_with(stripped, '"')
        return STRING (remove quotes)
    else if stripped matches /^-?\d+$/
        return INTEGER
    else if stripped matches /^-?\d+\.\d+$/
        return FLOAT
    else if stripped == "true" or stripped == "false"
        return BOOLEAN
    else
        return STRING (unquoted)
```

### Integer Type

**Definition**: Sequence of digits, optionally preceded by minus sign

**Range**: Implementation-defined, must support at least 64-bit signed integers

**Examples**:
```vibe
count 42
port 8080
negative -17
zero 0
large 9223372036854775807
```

**Invalid**:
```vibe
with_comma 1,000    # Comma not allowed
with_space 1 000    # Space not allowed
leading_zero 007    # Discouraged but not invalid
```

### Float Type

**Definition**: Sequence of digits with exactly one decimal point

**Range**: Implementation-defined, must support at least IEEE 754 double precision

**Examples**:
```vibe
pi 3.14159
timeout 30.5
negative -0.5
zero 0.0
```

**Note**: Scientific notation (1.23e-4) is not supported for simplicity.

### Boolean Type

**Definition**: Exactly the literals `true` or `false`

**Case Sensitivity**: Case sensitive - `True`, `TRUE`, etc. are strings

**Examples**:
```vibe
enabled true
debug false
ssl_required true
```

### String Type

Strings come in two forms: quoted and unquoted.

#### Unquoted Strings

**Valid Characters**: ASCII printable (0x21-0x7E) excluding structural characters

**Rules**:
- Cannot start with digit if entire value looks like number
- Cannot be `true` or `false`
- Cannot contain whitespace
- Cannot contain `{`, `}`, `[`, `]`, `#`

**Examples**:
```vibe
hostname server1.example.com
protocol https
email user@example.com
uuid 550e8400-e29b-41d4-a716-446655440000
```

#### Quoted Strings

**Delimiters**: Double quotes `"`

**Required When**:
- Value contains whitespace
- Value contains structural characters
- Value is purely numeric but should be treated as string
- Value contains Unicode characters
- Value equals `true` or `false` but should be string

**Examples**:
```vibe
message "Hello, World!"
path "C:\Program Files\App"
spaced "value with spaces"
unicode "Hello 世界! 🌍"
numeric_string "12345"
boolean_string "true"
```

### Array Type

**Definition**: Ordered collection of scalar values

**Syntax**: Values separated by whitespace, enclosed in `[]`

**Restrictions**: Arrays can only contain scalar values (integers, floats, booleans, strings). Nested objects or arrays are not allowed.

**Examples**:
```vibe
# Inline arrays
ports [8080 8081 8082]
hosts [web1.com web2.com web3.com]
flags [enabled debug verbose]

# Multi-line arrays
servers [
  production.example.com
  staging.example.com
  development.example.com
]

# Mixed types
mixed [42 "hello" true 3.14]
```

### Object Type

**Definition**: Unordered collection of key-value pairs

**Syntax**: Statements enclosed in `{}`

**Nesting**: Unlimited depth supported

**Examples**:
```vibe
database {
  host localhost
  port 5432
  credentials {
    username admin
    password "secret123"
  }
}
```

## Syntax Rules {#syntax-rules}

### Assignment Syntax

```vibe
key value
```

- Key must be valid identifier
- Value can be scalar, object, or array
- No assignment operator (`=` or `:`) required

### Object Declaration

```vibe
name {
  key1 value1
  key2 value2
  nested {
    inner_key inner_value
  }
}
```

- Opens new scope for nested statements
- Must be closed with matching `}`
- Can contain assignments, objects, or arrays

### Array Declaration

```vibe
# Inline
name [value1 value2 value3]

# Multi-line
name [
  value1
  value2
  value3
]
```

- Space or newline separated items
- No commas or other separators
- Must be closed with matching `]`

## Comments {#comments}

### Syntax

Comments start with `#` and continue to end of line:

```vibe
# Full line comment
key value  # Inline comment
```

### Rules

1. **Placement**: Can appear anywhere except inside quoted strings
2. **Preservation**: Not preserved in parsed data structure
3. **Escaping**: `#` inside quoted strings is literal
4. **Unicode**: Can contain any valid UTF-8 characters

### Examples

```vibe
# Application configuration
# Author: Development Team
# Last updated: 2024-01-14

app {
  name "My Application"  # Display name
  version 1.0           # Semantic version
  # debug true          # Commented out setting
}

# Database settings
database {
  host localhost        # Development server
  port 5432            # PostgreSQL default
  name "app_db"        # Name with # character in quotes
}
```

## String Literals {#string-literals}

### Escape Sequences

Quoted strings support these escape sequences:

| Escape | Character | Unicode | Description |
|--------|-----------|---------|-------------|
| `\"` | `"` | U+0022 | Double quote |
| `\\` | `\` | U+005C | Backslash |
| `\n` | | U+000A | Newline (LF) |
| `\r` | | U+000D | Carriage return (CR) |
| `\t` | | U+0009 | Horizontal tab |
| `\uXXXX` | | U+XXXX | Unicode codepoint (4 hex digits) |

### Unicode Support

- Files must be UTF-8 encoded
- Unicode characters can appear in quoted strings and comments
- `\uXXXX` escape sequences support Basic Multilingual Plane
- No support for surrogate pairs or `\UXXXXXXXX` sequences
- Identifiers are ASCII-only for performance

### Examples

```vibe
greeting "Hello, world! 🌍"
path "C:\\Users\\Name\\Documents"
escaped "She said \"Hello\""
unicode "Thumbs up: \u1F44D"
multiline "Line 1\nLine 2\nLine 3"
```

## Path Notation {#path-notation}

### Syntax

Access nested values using dot notation:

```
path = identifier { "." identifier }
```

### Examples

For this configuration:
```vibe
server {
  database {
    host localhost
    port 5432
    ssl {
      enabled true
    }
  }
}
```

Access paths:
- `server.database.host` → `"localhost"`
- `server.database.port` → `5432`
- `server.database.ssl.enabled` → `true`

### Array Access

Arrays use bracket notation:
```vibe
servers [web1.com web2.com web3.com]
```

Access: `servers[0]` → `"web1.com"`

## Error Handling {#error-handling}

### Error Categories

1. **Lexical Errors**: Invalid characters or token structure
2. **Syntax Errors**: Grammar violations
3. **Semantic Errors**: Type mismatches or invalid references

### Error Reporting

Implementations should provide:
- **Line and column numbers**
- **Descriptive error messages**
- **Context information**
- **Suggested fixes** when possible

### Common Errors

#### Unclosed Objects
```vibe
config {
  key value
# Missing closing brace
```
Error: "Unclosed object starting at line 1, column 8"

#### Invalid Escape Sequence
```vibe
path "C:\invalid\escape"
```
Error: "Invalid escape sequence '\\i' at line 1, column 11"

#### Duplicate Keys
```vibe
server {
  port 8080
  port 9090  # Duplicate key
}
```
Behavior: Implementation-defined (last value wins, error, or array conversion)

## Implementation Guidelines {#implementation-guidelines}

### Memory Management

- Use reference counting or garbage collection for values
- Implement proper cleanup for parser state
- Handle large files efficiently

### Performance Targets

Target performance on modern hardware (3GHz CPU, 8GB RAM):
- **Small files** (< 1KB): < 1ms parse time
- **Medium files** (1-100KB): < 10ms parse time  
- **Large files** (100KB-10MB): < 100ms parse time

### API Design Principles

#### Parser Interface
```c
typedef struct VibeParser VibeParser;

VibeParser* vibe_parser_new(void);
void vibe_parser_free(VibeParser* parser);
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);
VibeError vibe_get_last_error(VibeParser* parser);
```

#### Value Access Interface
```c
const char* vibe_get_string(VibeValue* value, const char* path);
int64_t vibe_get_int(VibeValue* value, const char* path);
double vibe_get_float(VibeValue* value, const char* path);
bool vibe_get_bool(VibeValue* value, const char* path);
VibeArray* vibe_get_array(VibeValue* value, const char* path);
VibeObject* vibe_get_object(VibeValue* value, const char* path);
```

### Thread Safety

- Parser instances should be thread-local
- Parsed values should be immutable and thread-safe
- Provide thread-safe access methods

### Security Considerations

#### Input Validation
- Limit maximum nesting depth (default: 100)
- Limit maximum string length (default: 1MB)
- Limit maximum file size (default: 100MB)
- Validate UTF-8 encoding

#### Attack Prevention
- **Billion Laughs**: Limit expansion depth
- **Memory Exhaustion**: Implement size limits
- **Parser Complexity**: Use iterative parsing where possible

---

<div class="spec-footer">
  <p><strong>Version:</strong> 1.0.0</p>
  <p><strong>Last Updated:</strong> January 2024</p>
  <p><strong>License:</strong> <a href="https://github.com/1ay1/vibe/blob/main/LICENSE">MIT License</a></p>
</div>

<style>
.spec-intro {
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  padding: 2rem;
  border-radius: 1rem;
  margin: 2rem 0;
}

.lead {
  font-size: 1.2rem;
  line-height: 1.6;
  margin: 0;
  color: #2c3e50;
}

.spec-footer {
  background: #f8f9fa;
  padding: 2rem;
  border-radius: 0.5rem;
  margin: 3rem 0;
  text-align: center;
  border-top: 3px solid #667eea;
}

.spec-footer p {
  margin: 0.5rem 0;
  color: #6c757d;
}

table {
  width: 100%;
  border-collapse: collapse;
  margin: 1rem 0;
  background: white;
  border-radius: 0.5rem;
  overflow: hidden;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

th, td {
  padding: 1rem;
  text-align: left;
  border-bottom: 1px solid #e1e8ed;
}

th {
  background: #f8f9fa;
  font-weight: 600;
  color: #2c3e50;
}

tr:hover {
  background: #f8f9fa;
}

code {
  background: #f8f9fa;
  padding: 0.2rem 0.4rem;
  border-radius: 0.25rem;
  font-family: 'Fira Code', Monaco, Consolas, monospace;
}

pre {
  background: #f8f9fa;
  border: 1px solid #e1e8ed;
  border-radius: 0.5rem;
  padding: 1rem;
  overflow-x: auto;
  margin: 1rem 0;
}

pre code {
  background: none;
  padding: 0;
}

h2 {
  color: #2c3e50;
  border-bottom: 2px solid #667eea;
  padding-bottom: 0.5rem;
  margin-top: 3rem;
}

h3 {
  color: #34495e;
  margin-top: 2rem;
}

h4 {
  color: #5a6c7d;
  margin-top: 1.5rem;
}
</style>