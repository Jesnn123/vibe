# VIBE Format Specification

**Version:** 1.0  
**Status:** Draft  
**Date:** 2025-01-14

## Table of Contents

1. [Overview](#overview)
2. [Design Goals](#design-goals)
3. [Grammar](#grammar)
4. [Lexical Analysis](#lexical-analysis)
5. [Data Types](#data-types)
6. [String Literals](#string-literals)
7. [Comments](#comments)
8. [Arrays](#arrays)
9. [Objects](#objects)
10. [Whitespace and Formatting](#whitespace-and-formatting)
11. [Reserved Words](#reserved-words)
12. [Path Notation](#path-notation)
13. [Duplicate Keys](#duplicate-keys)
14. [Complete Examples](#complete-examples)
15. [Parsing Algorithm](#parsing-algorithm)
16. [Error Handling](#error-handling)
17. [File Format](#file-format)
18. [Implementation Guidelines](#implementation-guidelines)
19. [Security Considerations](#security-considerations)
20. [Performance Requirements](#performance-requirements)
21. [Validation and Schema](#validation-and-schema)
22. [Comparison to Other Formats](#comparison-to-other-formats)
23. [Migration Guide](#migration-guide)
24. [Future Considerations](#future-considerations)
25. [References](#references)

## Overview

VIBE (Values In Bracket Expression) is a hierarchical configuration file format designed for human readability and fast machine parsing. It combines the visual clarity of structured formats with the simplicity of minimal syntax, making it ideal for configuration files, data serialization, and human-editable structured data.

Like its namesake, VIBE aims to create good vibes in your development workflow by eliminating the frustration of complex configuration formats. No more YAML indentation nightmares or JSON comma catastrophes - just smooth, readable config that flows naturally.

VIBE is designed to be:
- **Readable**: Structure is immediately apparent from visual inspection (good vibes at first sight)
- **Writable**: Easy to create and modify by hand (maintains the creative flow)
- **Parseable**: Single-pass parsing with deterministic grammar (keeps the parsing vibe chill)
- **Unambiguous**: Only one correct way to represent any data structure (no mixed signals)
- **Efficient**: Minimal memory overhead and fast parsing (performance vibes only)

## Design Goals

### 1. Visual Hierarchy
The structure of the data should be instantly recognizable through visual inspection alone. Nested objects use braces `{}` that create clear visual boundaries, while arrays use brackets `[]` for easy identification. It's like having your data wear its heart on its sleeve - the structure just vibes with you.

### 2. Minimal Syntax
VIBE uses exactly 5 token types, making it one of the simplest structured formats to learn and implement. No commas, semicolons, or complex punctuation rules. We keep it simple because complicated syntax kills the vibe.

### 3. Unambiguous Grammar
There is exactly one canonical way to represent any data structure in VIBE. This eliminates formatting debates and ensures consistent machine generation. No more "Is this the right way?" anxiety - VIBE's got your back.

### 4. Fast Parsing
Single-pass, O(n) parsing with a simple state machine. No backtracking, lookahead, or complex grammar rules that slow down parsing. Speed is part of the vibe - nobody likes waiting around.

### 5. Type Safety
Clear, deterministic type inference rules ensure that values are consistently interpreted across different implementations. Types just work naturally, like they're meant to be.

### 6. Human-Friendly
Optimized for human reading and writing, with meaningful error messages and forgiving whitespace rules. VIBE doesn't judge your spacing choices - it just goes with the flow.

## Grammar

### Formal Grammar Definition (EBNF)

```
config        = { statement } ;
statement     = ( assignment | object | array | comment ) [ comment ] newline ;
assignment    = identifier value ;
object        = identifier "{" { statement } "}" ;
array         = identifier "[" [ value_list ] "]" ;
value_list    = value { ( whitespace | newline ) value } ;
value         = string | number | boolean | identifier ;
comment       = "#" { any_char - newline } ;
identifier    = ( letter | "_" ) { letter | digit | "_" | "-" } ;
string        = quoted_string | unquoted_string ;
quoted_string = '"' { char | escape_sequence } '"' ;
unquoted_string = ( letter | digit | "_" | "-" | "." | "/" | ":" ) 
                  { letter | digit | "_" | "-" | "." | "/" | ":" } ;
number        = [ "-" ] ( integer | float ) ;
integer       = digit { digit } ;
float         = digit { digit } "." digit { digit } ;
boolean       = "true" | "false" ;
escape_sequence = "\" ( '"' | "\" | "n" | "t" | "r" | "u" hex_digit hex_digit hex_digit hex_digit ) ;
```

### Tokens

VIBE recognizes exactly 5 token types:

1. **IDENTIFIER** - `[a-zA-Z_][a-zA-Z0-9_-]*`
2. **STRING** - Quoted or unquoted text
3. **LEFT_BRACE** - `{`
4. **RIGHT_BRACE** - `}`
5. **LEFT_BRACKET** - `[`
6. **RIGHT_BRACKET** - `]`

### Syntax Rules

#### Assignment
```
key value
```
- Key must be a valid identifier
- Value continues until end of line or special character
- Multiple words in value require quotes

#### Object Declaration
```
name {
  statements...
}
```
- Opens a new scope with nested statements
- Can contain assignments, objects, or arrays
- Must be closed with matching `}`

#### Array Declaration
```
name [value1 value2 value3]
```
or multi-line:
```
name [
  value1
  value2
  value3
]
```
- Space or newline separated items
- No commas or other separators
- Must be closed with matching `]`

## Lexical Analysis

### Tokenization Process

The lexer processes the input character by character, maintaining state to determine token boundaries:

1. **Whitespace Handling**: Spaces, tabs, and newlines are used as separators but are not significant tokens
2. **Comment Recognition**: `#` starts a comment that continues to end of line
3. **String Recognition**: Quoted strings begin with `"` and end with matching `"`
4. **Identifier Recognition**: Alphanumeric characters, underscores, and hyphens
5. **Operator Recognition**: Braces and brackets are single-character tokens

### Character Classes

```
ALPHA     = [a-zA-Z]
DIGIT     = [0-9]
ALNUM     = ALPHA | DIGIT
IDENT_START = ALPHA | "_"
IDENT_CHAR  = ALNUM | "_" | "-"
UNQUOTED_CHAR = ALNUM | "_" | "-" | "." | "/" | ":"
WHITESPACE = " " | "\t" | "\r"
NEWLINE   = "\n"
```

### Token Examples

```
# Valid identifiers
server
database_host
ssl-enabled
_private
config2

# Valid unquoted strings
localhost
192.168.1.1
/usr/local/bin
example.com:8080

# Invalid identifiers (would be treated as strings)
2servers    # starts with digit
server.host # contains dot (would be unquoted string)
配置        # Unicode not allowed in identifiers
```

## Data Types

VIBE supports 5 fundamental data types with automatic type inference:

### Type Inference Algorithm

The type of a value is determined by examining its content:

```
function inferType(value):
    stripped = strip_whitespace(value)
    
    if stripped matches /^-?\d+$/
        return INTEGER
    else if stripped matches /^-?\d+\.\d+$/
        return FLOAT
    else if stripped == "true" or stripped == "false"
        return BOOLEAN
    else if starts_with(stripped, '"') and ends_with(stripped, '"')
        return STRING (remove quotes)
    else
        return STRING (unquoted)
```

### Integer Type

**Definition**: Sequence of digits, optionally preceded by minus sign

**Range**: Implementation-defined, but must support at least 64-bit signed integers (-2^63 to 2^63-1)

**Examples**:
```
count 42
negative -17
zero 0
large 9223372036854775807
```

**Invalid Examples**:
```
with_comma 1,000    # Comma not allowed
with_space 1 000    # Space not allowed
leading_zero 007    # Leading zeros discouraged but not invalid
```

### Float Type

**Definition**: Sequence of digits with exactly one decimal point

**Range**: Implementation-defined, but must support at least IEEE 754 double precision

**Examples**:
```
pi 3.14159
negative -0.5
zero 0.0
scientific 1.23  # Scientific notation not supported
```

**Special Values**: Implementations may support `inf`, `-inf`, and `nan` as special float values

### Boolean Type

**Definition**: Exactly the literals `true` or `false`

**Case Sensitivity**: Case sensitive - `True`, `TRUE`, `False`, `FALSE` are strings, not booleans

**Examples**:
```
enabled true
debug false
```

### String Type

Strings come in two forms: quoted and unquoted.

#### Unquoted Strings

**Valid Characters**: `[a-zA-Z0-9_-./:]`

**Rules**:
- Must contain only ASCII characters (single-byte, 0x00-0x7F)
- Cannot start with a digit if the entire value looks like a number
- Cannot be `true` or `false` (would be interpreted as boolean)
- Stops at first whitespace or special character
- Unicode characters require quoted strings

**Examples**:
```
hostname server1.example.com
path /usr/local/bin
protocol https
port_name 8080  # String, not number, because of underscore

# Unicode values must be quoted (ASCII identifiers only)
language "中文"
protocol "сервер"
port "データベース"
```

#### Quoted Strings

**Delimiters**: Double quotes `"`

**Required When**:
- Value contains whitespace
- Value contains special characters: `# { } [ ]`
- Value has leading/trailing whitespace
- Value contains Unicode (non-ASCII) characters
- Value would otherwise be ambiguous

**Examples**:
```
message "Hello, world!"
path_with_spaces "/Program Files/App"
special "value with # comment character"
empty ""
just_spaces "   "
unicode_string "你好 世界"  # Unicode requires quotes
mixed_special "配置 # with comment char"
```

### Array Type

**Definition**: Ordered collection of values of any type

**Syntax**: Values separated by whitespace, enclosed in `[]`

**Examples**:
```
numbers [1 2 3]
strings [hello world foo]
mixed [42 "hello" true 3.14]
```

### Object Type

**Definition**: Unordered collection of key-value pairs

**Syntax**: Statements enclosed in `{}`

**Examples**:
```
config {
  host localhost
  port 8080
}
```

## String Literals

### Escape Sequences

Quoted strings support the following escape sequences:

| Escape | Character | Unicode | Description |
|--------|-----------|---------|-------------|
| `\"` | `"` | U+0022 | Double quote |
| `\\` | `\` | U+005C | Backslash |
| `\n` | | U+000A | Newline (LF) |
| `\r` | | U+000D | Carriage return (CR) |
| `\t` | | U+0009 | Horizontal tab |
| `\uXXXX` | | U+XXXX | Unicode codepoint (4 hex digits) |

### Unicode Support

- Files must be valid UTF-8
- Unicode characters can appear directly in quoted strings and comments only
- `\uXXXX` escape sequences support Basic Multilingual Plane
- No support for surrogate pairs or `\UXXXXXXXX` sequences
- **Identifiers and unquoted strings are strictly ASCII-only** for simplicity and performance

**Examples**:
```
greeting "Hello, world! 🌍"
path "C:\Users\Name\Documents"
escaped "She said \"Hello\""
unicode "Emoji: \u1F44D"
multiline "Line 1\nLine 2\nLine 3"

# ASCII identifiers with Unicode quoted values
configuration "aplicación"  # ASCII identifier, quoted Unicode value
settings "сервер"
config "データベース"
language english     # ASCII identifier and unquoted ASCII value
language "español"   # ASCII identifier, quoted Unicode value
```

### String Parsing Rules

1. **Unquoted String Parsing**:
   - Starts with first non-whitespace character  
   - Continues until whitespace, special character, or end of line
   - Cannot contain `# { } [ ]`
   - ASCII-only: `[a-zA-Z0-9_-./:]`

2. **Quoted String Parsing**:
   - Starts with `"`
   - Continues until matching `"` (not escaped)
   - Must be closed on same logical line
   - Processes escape sequences

3. **Empty Strings**:
   - Only possible with quoted strings: `""`
   - Unquoted empty values are not allowed

## Comments

### Syntax

Comments start with `#` and continue to the end of the line.

```
# This is a full line comment
key value  # This is an inline comment
```

### Rules

1. **Placement**: Comments can appear anywhere except inside quoted strings
2. **Preservation**: Comments are not preserved in the parsed data structure
3. **Escaping**: `#` inside quoted strings is literal, not a comment start
4. **Unicode**: Comments can contain any valid UTF-8 characters

### Examples

```
# Application configuration file
# Author: John Doe
# Date: 2025-01-14

app {
  name "My App"  # Application display name
  version 1.0    # Semantic version
  # debug true   # Commented out debug flag
}

database {
  # Database connection settings
  host localhost      # Development database
  port 5432          # PostgreSQL default port
  name "app_db"      # Database name with # character
}
```

### Comment Best Practices

1. Use comments to explain non-obvious configuration choices
2. Include units for numeric values when relevant  
3. Document valid value ranges or formats
4. Explain the purpose of complex nested structures
5. Feel free to add encouraging comments - good vibes are contagious!

```
timeout 30  # seconds - just enough time to grab a coffee ☕
retries 3   # third time's the charm!
```

## Arrays

### Syntax Variants

#### Inline Arrays
```
ports [8080 8081 8082]
hosts [server1 server2 server3]
```

#### Multi-line Arrays
```
servers [
  prod.example.com
  staging.example.com
  dev.example.com
]
```

#### Mixed Format (Not Recommended)
```
mixed [item1 item2
  item3 item4]
```

### Array Content Rules

1. **Type Mixing**: Arrays can contain values of different types
2. **Nesting**: Arrays cannot directly contain other arrays or objects
3. **Empty Arrays**: `[]` represents an empty array
4. **Whitespace**: Items separated by any amount of whitespace
5. **Comments**: Comments can appear between array items

### Examples

```
# Homogeneous arrays
numbers [1 2 3 4 5]
strings [apple orange banana]
booleans [true false true]

# Heterogeneous arrays
mixed [42 "hello" true 3.14]

# Multi-line with comments
servers [
  # Production servers
  prod1.example.com
  prod2.example.com
  
  # Staging server
  staging.example.com
]

# Empty array
empty_list []
```

### Array Access

Arrays are typically accessed by index in implementations:
- `servers[0]` → `"prod1.example.com"`
- `numbers[2]` → `3`

## Objects

### Syntax

Objects are defined using braces and can contain any number of statements:

```
object_name {
  key1 value1
  key2 value2
  nested_object {
    nested_key nested_value
  }
}
```

### Nesting Rules

1. **Unlimited Depth**: Objects can be nested to arbitrary depth (subject to implementation limits)
2. **Mixed Content**: Objects can contain assignments, nested objects, and arrays
3. **Empty Objects**: `{}` represents an empty object
4. **Scope**: Each object creates a new scope for key names

### Examples

```
# Simple object
server {
  host localhost
  port 8080
}

# Nested objects
application {
  name "My App"
  
  database {
    host db.example.com
    port 5432
    
    connection_pool {
      min_size 5
      max_size 20
      timeout 30.0
    }
  }
  
  cache {
    type redis
    host cache.example.com
    
    settings {
      max_memory 1gb
      eviction_policy lru
    }
  }
}

# Object with arrays
web_server {
  hosts [
    web1.example.com
    web2.example.com
  ]
  
  ssl {
    enabled true
    protocols [TLSv1.2 TLSv1.3]
  }
}
```

### Object Access

Objects are typically accessed using dot notation:
- `server.host` → `"localhost"`
- `application.database.port` → `5432`
- `web_server.ssl.protocols[0]` → `"TLSv1.2"`

## Whitespace and Formatting

### Whitespace Rules

1. **Insignificant Indentation**: Indentation is purely visual and not syntactically meaningful
2. **Flexible Spacing**: Any amount of whitespace can separate tokens
3. **Line Breaks**: Statements are separated by newlines
4. **Visual Alignment**: Encouraged for readability but not required

### Recommended Style

```
# Good: Consistent 2-space indentation
server {
  host localhost
  port 8080
  
  ssl {
    enabled true
    cert_path /etc/ssl/cert.pem
  }
}

# Also valid: No indentation
server {
host localhost
port 8080
ssl {
enabled true
cert_path /etc/ssl/cert.pem
}
}

# Also valid: Tab indentation
server {
	host localhost
	port 8080
}
```

### Formatting Guidelines

1. **Indentation**: Use 2 or 4 spaces consistently
2. **Blank Lines**: Use blank lines to separate logical sections
3. **Alignment**: Align values for related keys when it improves readability
4. **Array Formatting**: Use multi-line format for arrays with more than 3-4 items

## Reserved Words

**None.** VIBE has no reserved words. Any valid identifier can be used as a key name. We don't believe in gatekeeping - all words are welcome in the VIBE family.

```
# All of these are valid key names
true true
false false
null null
if conditional
for loop_config
class object_type
vibe excellent  # meta!

# Unicode content must be in quoted strings
chinese_name "配置"
russian_name "сервер"
spanish_name "configuración"
```

This design choice prioritizes flexibility and eliminates the need to escape or quote common configuration key names. Because reserving words is just not the vibe.

## Path Notation

For programmatic access to nested values, implementations should support dot notation:

### Syntax

```
object.property
object.nested_object.property
object.array[index]
object.nested_object.array[index].property
```

### Examples

Given this VIBE configuration:
```
app {
  name "My Application"
  version 1.0
  
  database {
    hosts [db1.example.com db2.example.com]
    port 5432
  }
  
  features [auth api cache]
}
```

Path access:
- `app.name` → `"My Application"`
- `app.version` → `1.0`
- `app.database.port` → `5432`
- `app.database.hosts[0]` → `"db1.example.com"`
- `app.features[1]` → `"api"`

### Implementation Notes

1. **Case Sensitivity**: Paths are case-sensitive
2. **Array Bounds**: Out-of-bounds array access should return null/undefined
3. **Missing Keys**: Access to non-existent keys should return null/undefined
4. **Type Safety**: Implementations may provide typed access methods

## Duplicate Keys

### Behavior Options

Implementations must define their behavior for duplicate keys:

#### Option 1: Last Value Wins (Recommended)
```
port 8080
host localhost
port 9000  # This value is used: 9000
```
Sometimes you change your mind - that's totally valid! VIBE just goes with your latest decision.

#### Option 2: Error on Duplicate
Parsers reject files with duplicate keys in the same scope. For those who like their configs strict and unambiguous.

#### Option 3: Array Conversion
Some implementations may convert duplicate keys into arrays. Why choose when you can have both? (Though this might confuse the vibe a bit.)

### Scope Rules

Duplicate keys are only considered within the same scope:

```
server {
  port 8080  # Different scope
}

client {
  port 3000  # Different scope - this is allowed
}
```

## Complete Examples

### Web Application Configuration

```
# Web Application Configuration
# Environment: Production

application {
  name "E-commerce API"
  version 2.1.4
  environment production
  debug false
  
  # Server configuration
  server {
    host 0.0.0.0
    port 8080
    
    ssl {
      enabled true
      cert_file /etc/ssl/certs/api.crt
      key_file /etc/ssl/private/api.key
      protocols [TLSv1.2 TLSv1.3]
    }
    
    timeouts {
      read 30
      write 30
      idle 120
      shutdown 10
    }
  }
  
  # Database configuration
  database {
    primary {
      driver postgresql
      host db-primary.internal
      port 5432
      database ecommerce_prod
      username api_user
      password_file /etc/secrets/db_password
      
      pool {
        min_connections 10
        max_connections 50
        idle_timeout 300
        max_lifetime 3600
      }
    }
    
    replicas [
      db-replica1.internal:5432
      db-replica2.internal:5432
      db-replica3.internal:5432
    ]
    
    migrations {
      auto_migrate false
      directory /app/migrations
    }
  }
  
  # Cache configuration
  cache {
    type redis
    
    primary {
      host cache-primary.internal
      port 6379
      database 0
      max_connections 20
    }
    
    cluster [
      cache1.internal:6379
      cache2.internal:6379
      cache3.internal:6379
    ]
    
    settings {
      default_ttl 3600
      max_memory 2gb
      eviction_policy allkeys-lru
    }
  }
  
  # Logging configuration
  logging {
    level info
    format json
    
    outputs [
      stdout
      /var/log/app/application.log
      /var/log/app/errors.log
    ]
    
    loggers {
      database {
        level debug
        output /var/log/app/database.log
      }
      
      security {
        level warn
        output /var/log/app/security.log
      }
    }
  }
  
  # Feature flags
  features {
    payment_v2 true
    recommendation_engine true
    beta_checkout false
    advanced_search true
  }
  
  # External services
  services {
    payment_gateway {
      url https://api.payments.example.com
      api_key_file /etc/secrets/payment_api_key
      timeout 10
      retry_attempts 3
    }
    
    email {
      provider sendgrid
      api_key_file /etc/secrets/sendgrid_api_key
      from_address "noreply@mystore.com"
      
      templates {
        welcome_email template_123
        order_confirmation template_456
        password_reset template_789
      }
    }
    
    analytics {
      provider google_analytics
      tracking_id "GA-XXXXXXXX-X"
      
      events [
        page_view
        purchase
        signup
        cart_abandonment
      ]
    }
  }
  
  # Security settings
  security {
    cors {
      enabled true
      allowed_origins [
        https://mystore.com
        https://admin.mystore.com
      ]
      allowed_methods [GET POST PUT DELETE OPTIONS]
      max_age 86400
    }
    
    rate_limiting {
      enabled true
      requests_per_minute 60
      burst_size 10
      
      endpoints {
        "/api/auth/login" {
          requests_per_minute 5
          burst_size 2
        }
        
        "/api/orders" {
          requests_per_minute 30
          burst_size 5
        }
      }
    }
    
    jwt {
      secret_file /etc/secrets/jwt_secret
      expiry 3600
      refresh_expiry 604800
      issuer "mystore-api"
    }
  }
  
  # Monitoring and health checks
  monitoring {
    health_check {
      enabled true
      path /health
      interval 30
      timeout 5
    }
    
    metrics {
      enabled true
      provider prometheus
      path /metrics
      
      custom_metrics [
        order_processing_time
        cart_conversion_rate
        api_response_time
      ]
    }
    
    alerts {
      error_rate_threshold 5.0
      response_time_threshold 500
      
      notifications {
        email admin@mystore.com
        slack "#alerts"
        pagerduty true
      }
    }
  }
}
```

### Development Environment Override

```
# Development overrides
application {
  environment development
  debug true
  
  server {
    port 3000
    
    ssl {
      enabled false
    }
  }
  
  database {
    primary {
      host localhost
      database ecommerce_dev
      username dev_user
      password "dev_password_123"
    }
    
    replicas []  # No replicas in development
  }
  
  cache {
    type memory  # In-memory cache for development
  }
  
  logging {
    level debug
    format text
    outputs [stdout]
  }
  
  features {
    payment_v2 true
    beta_checkout true  # Enable beta features in dev
  }
}
```

## Parsing Algorithm

### State Machine

The VIBE parser is implemented as a state machine with the following states:

```
States:
- ROOT: Expecting top-level statements
- OBJECT: Inside an object, expecting statements
- ARRAY: Inside an array, expecting values
- VALUE: Reading a value
- COMMENT: Reading a comment
```

### Parsing Process

```
1. Initialize parser state to ROOT
2. Read next token
3. Based on current state and token type:
   - ROOT state:
     - IDENTIFIER: Read as key, transition to VALUE
     - IDENTIFIER + '{': Create object, push OBJECT state
     - IDENTIFIER + '[': Create array, push ARRAY state
     - '#': Transition to COMMENT state
     - EOF: End parsing
   
   - OBJECT state:
     - IDENTIFIER: Read as key, transition to VALUE
     - IDENTIFIER + '{': Create nested object, push OBJECT state
     - IDENTIFIER + '[': Create array, push ARRAY state
     - '}': Pop state, return to previous
     - '#': Transition to COMMENT state
   
   - ARRAY state:
     - STRING/NUMBER/BOOLEAN/IDENTIFIER: Add to array
     - ']': Pop state, return to previous
     - '#': Transition to COMMENT state
   
   - VALUE state:
     - STRING/NUMBER/BOOLEAN/IDENTIFIER: Set value, return to previous state
   
   - COMMENT state:
     - NEWLINE: Return to previous state
     - Any other: Continue reading comment
```

### Parser Implementation Example (Pseudocode)

```c
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

typedef struct {
    ParseState state;
    int depth;
    VibeValue* current_object;
    char* current_key;
} Parser;

VibeValue* parse_vibe(char* input) {
    Parser parser = {0};
    parser.state = ROOT;
    parser.current_object = create_object();
    
    Lexer lexer = init_lexer(input);
    Token token;
    
    while ((token = next_token(&lexer)).type != EOF_TOKEN) {
        switch (parser.state) {
            case ROOT:
            case OBJECT:
                if (token.type == IDENTIFIER) {
                    parser.current_key = token.value;
                    Token next = peek_token(&lexer);
                    
                    if (next.type == LEFT_BRACE) {
                        consume_token(&lexer); // consume '{'
                        push_object(&parser, token.value);
                    } else if (next.type == LEFT_BRACKET) {
                        consume_token(&lexer); // consume '['
                        push_array(&parser, token.value);
                    } else {
                        parser.state = VALUE;
                    }
                } else if (token.type == RIGHT_BRACE) {
                    pop_state(&parser);
                }
                break;
                
            case ARRAY:
                if (token.type == RIGHT_BRACKET) {
                    pop_state(&parser);
                } else {
                    add_array_value(&parser, parse_value(token));
                }
                break;
                
            case VALUE:
                set_object_value(&parser, parser.current_key, parse_value(token));
                parser.state = (parser.depth > 0) ? OBJECT : ROOT;
                break;
        }
    }
    
    return parser.current_object;
}
```

## Error Handling

### Error Categories

1. **Lexical Errors**: Invalid character sequences
2. **Syntax Errors**: Invalid grammar
3. **Semantic Errors**: Valid syntax but invalid meaning
4. **Type Errors**: Type inference failures

### Error Reporting

Parsers should provide detailed error information:

```
Error: Unexpected token '}' on line 15, column 8
  ssl {
      ^
Expected identifier or closing brace
```

### Common Errors and Messages

#### Unclosed Objects
```
Error: Unclosed object on line 12
  server {
         ^
Expected '}' before end of file
(Looks like this object needs some closure 😉)
```

#### Invalid Escape Sequence
```
Error: Invalid escape sequence '\x' on line 8, column 15
  path "C:\Users\name"
              ^
Valid escape sequences: \" \\ \n \t \r \uXXXX
(That backslash is trying to escape reality, but VIBE keeps it real!)
```

#### Duplicate Keys (if implementation rejects)
```
Error: Duplicate key 'port' on line 16
  port 9000
  ^
Previously defined on line 14
(Déjà vu! This key is having an identity crisis.)
```

#### Invalid Array Syntax
```
Error: Expected ']' on line 10, column 25
  servers [web1.com web2.com
                            ^
Arrays must be closed with ']'
(This array is feeling a bit... open-ended. Let's give it some closure!)
```

### Recovery Strategies

1. **Panic Mode**: Skip tokens until a synchronization point (e.g., newline, brace)
2. **Error Production**: Define grammar rules for common errors
3. **Insertion**: Insert missing tokens when obvious
4. **Deletion**: Skip unexpected tokens

## File Format

### File Extension
**Recommended**: `.vibe`

Alternative extensions: `.vb`, `.config`, `.conf`

### MIME Type
**Proposed**: `application/vibe`  
**Alternative**: `text/vibe`

### Character Encoding
**Required**: UTF-8 without BOM

**Byte Order Mark**: Not permitted. Files must not start with UTF-8 BOM (EF BB BF).

### Line Endings
**Supported**: 
- Unix (LF): `\n`
- Windows (CRLF): `\r\n`
- Legacy Mac (CR): `\r`

**Recommended**: Unix line endings for cross-platform compatibility.

### File Size Limits
**Recommended Limits**:
- Maximum file size: 10 MB
- Maximum line length: 1000 characters
- Maximum nesting depth: 64 levels
- Maximum identifier length: 255 characters
- Maximum string length: 1 MB

## Implementation Guidelines

### Memory Management

1. **Streaming Parser**: Support parsing files larger than available memory
2. **Memory Efficiency**: Parsed structure should not exceed 2x file size
3. **Reference Counting**: Implement proper cleanup for nested structures
4. **String Interning**: Consider interning common string values

### Performance Targets

- **Parse Speed**: > 100 MB/s on modern hardware (3+ GHz CPU)
- **Memory Usage**: < 2x input file size
- **Latency**: < 1ms for files under 1KB
- **Scalability**: Linear time complexity O(n) with input size

### API Design Principles

#### Parser Interface
```c
// C API example
typedef struct VibeParser VibeParser;

VibeParser* vibe_parser_new();
void vibe_parser_free(VibeParser* parser);
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);
VibeError vibe_get_last_error(VibeParser* parser);
```

#### Value Access Interface
```c
// Type-safe accessors
const char* vibe_get_string(VibeValue* value, const char* path);
int64_t vibe_get_int(VibeValue* value, const char* path);
double vibe_get_float(VibeValue* value, const char* path);
bool vibe_get_bool(VibeValue* value, const char* path);
VibeArray* vibe_get_array(VibeValue* value, const char* path);
VibeObject* vibe_get_object(VibeValue* value, const char* path);
```

### Thread Safety

Implementations should specify thread safety guarantees:

1. **Parser Objects**: Not thread-safe (one parser per thread)
2. **Parsed Values**: Immutable and thread-safe for reading
3. **Concurrent Parsing**: Multiple parsers can operate concurrently

### Testing Requirements

Implementations should include:

1. **Unit Tests**: Individual component testing
2. **Integration Tests**: End-to-end parsing tests
3. **Performance Tests**: Benchmarks with various file sizes
4. **Fuzz Tests**: Random input testing for robustness
5. **Compatibility Tests**: Cross-implementation compatibility

### Test Suite Examples

```
# Basic parsing tests
test_simple_assignment()
test_nested_objects()
test_arrays()
test_comments()
test_string_escaping()

# Error handling tests
test_syntax_errors()
test_unclosed_braces()
test_invalid_escape_sequences()

# Edge cases
test_empty_file()
test_large_files()
test_deep_nesting()
test_unicode_content()

# Performance tests
benchmark_parsing_speed()
benchmark_memory_usage()
profile_large_file_parsing()
```

## Security Considerations

### Input Validation

1. **File Size Limits**: Prevent memory exhaustion attacks
2. **Nesting Depth**: Prevent stack overflow with deep structures
3. **String Length**: Limit individual string sizes
4. **Identifier Length**: Limit key name lengths

### Recommended Security Limits

```
MAX_FILE_SIZE = 10 MB
MAX_NESTING_DEPTH = 64
MAX_STRING_LENGTH = 1 MB
MAX_IDENTIFIER_LENGTH = 255 characters
MAX_ARRAY_SIZE = 10,000 elements
MAX_OBJECT_KEYS = 10,000 keys
```

### Attack Vectors

#### Billion Laughs Attack
Prevent exponential memory growth through nested structures:
```
# Potential attack - deeply nested objects
level1 {
  level2 {
    level3 {
      # ... continues for many levels
    }
  }
}
```

**Mitigation**: Enforce maximum nesting depth

#### Memory Exhaustion
Large arrays or strings can consume excessive memory:
```
# Potential attack - huge array
huge_array [item1 item2 ... (millions of items)]
```

**Mitigation**: Enforce size limits and streaming parsing

#### Parser Complexity Attacks
Certain input patterns may cause quadratic parsing time:
```
# Potential attack - many duplicate keys
key value
key value
# ... repeated thousands of times
```

**Mitigation**: Implement linear-time duplicate detection

### Safe Parsing Practices

1. **Validate Input**: Check file size before parsing
2. **Resource Limits**: Set timeouts and memory limits
3. **Sanitize Output**: Validate parsed values before use
4. **Error Handling**: Don't expose internal parser state in errors
5. **Logging**: Log parsing attempts for security monitoring

## Performance Requirements

### Benchmarking Standards

#### Test Files
1. **Small**: 1KB configuration file
2. **Medium**: 100KB structured data
3. **Large**: 10MB complex configuration
4. **Deep**: Maximum nesting depth file
5. **Wide**: Many top-level keys

#### Performance Metrics
- **Parse Time**: Wall clock time to parse file
- **Memory Usage**: Peak memory consumption during parsing
- **Memory Efficiency**: Ratio of parsed structure size to file size
- **Throughput**: MB/s processing rate

#### Target Performance (on 3GHz CPU, 8GB RAM)

| File Size | Parse Time | Memory Usage | Throughput |
|-----------|------------|--------------|------------|
| 1KB | < 0.1ms | < 10KB | > 10 MB/s |
| 100KB | < 10ms | < 1MB | > 100 MB/s |
| 10MB | < 1s | < 50MB | > 100 MB/s |

### Optimization Strategies

1. **Single-Pass Parsing**: No backtracking or multiple passes
2. **String Interning**: Reuse common strings
3. **Memory Pooling**: Reduce allocation overhead
4. **SIMD Instructions**: Use vectorized operations for scanning
5. **Branch Prediction**: Optimize hot parsing paths

## Validation and Schema

While VIBE itself has no built-in schema validation, implementations may provide schema languages for validation.

### Proposed Schema Syntax

```vibe
# Example schema definition
schema ApplicationConfig {
  app {
    name: string
    version: string
    debug: boolean?  # Optional field
  }
  
  server {
    host: string
    port: integer(1..65535)  # Range constraint
    
    ssl?: {  # Optional object
      enabled: boolean
      cert: string
    }
  }
  
  features: [string]  # Array of strings
}
```

### Validation Rules

1. **Type Checking**: Ensure values match expected types
2. **Required Fields**: Validate presence of mandatory keys
3. **Range Constraints**: Check numeric values are within bounds
4. **Format Validation**: Validate strings match patterns (e.g., URLs, emails)
5. **Cross-Field Validation**: Validate relationships between fields

## Comparison to Other Formats

### Feature Comparison

| Feature | VIBE | JSON | YAML | TOML | XML |
|---------|------|------|------|------|-----|
| Human Readable | ✓ | ✗ | ✓ | ✓ | ✗ |
| Minimal Syntax | ✓ | ✗ | ✓ | ✓ | ✗ |
| Visual Hierarchy | ✓ | ✓ | ✗ | ✗ | ✓ |
| Fast Parsing | ✓ | ✓ | ✗ | ✓ | ✗ |
| No Indentation Rules | ✓ | ✓ | ✗ | ✓ | ✓ |
| Type Inference | ✓ | ✗ | ✓ | ✓ | ✗ |
| Comments | ✓ | ✗ | ✓ | ✓ | ✓ |
| Unambiguous | ✓ | ✓ | ✗ | ✓ | ✓ |
| Single Pass Parse | ✓ | ✓ | ✗ | ✓ | ✗ |

### When to Use VIBE

**Choose VIBE when**:
- Configuration files need to be human-readable and editable
- Fast parsing is important
- Visual structure clarity is valued
- Simple syntax is preferred
- Comments are needed

**Choose JSON when**:
- Web APIs and data interchange
- Strict typing is not needed
- Minimal parser complexity is required
- Maximum compatibility is needed

**Choose YAML when**:
- Complex data structures with references
- Multi-document files are needed
- Existing YAML ecosystem is required

**Choose TOML when**:
- Simple configuration files
- Strong typing is important
- INI-like format is preferred

## Migration Guide

### From JSON

JSON structures map directly to VIBE:

**JSON**:
```json
{
  "server": {
    "host": "localhost",
    "port": 8080,
    "ssl": {
      "enabled": true
    }
  },
  "features": ["auth", "api"]
}
```

**VIBE**:
```vibe
server {
  host localhost
  port 8080
  ssl {
    enabled true
  }
}
features [auth api]
```

### From YAML

YAML structures translate with some changes:

**YAML**:
```yaml
server:
  host: localhost
  port: 8080
  ssl:
    enabled: true
features:
  - auth
  - api
```

**VIBE**:
```vibe
server {
  host localhost
  port 8080
  ssl {
    enabled true
  }
}
features [auth api]
```

### From TOML

TOML sections become VIBE objects:

**TOML**:
```toml
[server]
host = "localhost"
port = 8080

[server.ssl]
enabled = true

features = ["auth", "api"]
```

**VIBE**:
```vibe
server {
  host localhost
  port 8080
  ssl {
    enabled true
  }
}
features [auth api]
```

### Migration Tools

Implementations should provide conversion utilities:

```bash
# Command-line converters
vibe-convert --from json --to vibe config.json config.vibe
vibe-convert --from yaml --to vibe config.yaml config.vibe
vibe-convert --from toml --to vibe config.toml config.vibe

# Validation
vibe-validate config.vibe
vibe-format config.vibe  # Pretty-print formatter
```

## Future Considerations

The future is looking bright for VIBE! We're constantly vibing with new ideas while keeping the core philosophy intact.

### Version 2.0 Potential Features

#### Multi-line Strings
```vibe
description """
This is a multi-line string
that spans several lines
and preserves formatting.
"""
```

#### Include Directives
```vibe
include "database.vibe"
include "logging.vibe"
```

#### Variable Substitution
```vibe
env development
database_host ${env}.db.example.com
```

#### Schema Validation
```vibe
schema ConfigSchema {
  server {
    host: string
    port: integer(1..65535)
  }
}
```

#### Binary Format
A binary representation for faster parsing and smaller size:
- Magic number: `VIBE` (0x56494245)
- Version byte
- Length-prefixed strings
- Type markers for values

### Experimental Features

#### Extended Types
- Date/time literals: `2025-01-14T10:30:00Z`
- Duration literals: `30s`, `5m`, `2h`
- Size literals: `10MB`, `1GB`

#### Conditional Blocks
```vibe
if environment == "production" {
  debug false
  log_level error
}
```

#### Templates
```vibe
template DatabaseConfig {
  host ${db_host}
  port ${db_port}
  name ${db_name}
}

primary_db: DatabaseConfig {
  db_host prod-db.example.com
  db_port 5432
  db_name app_prod
}
```

Templates would let you copy that vibe across multiple configurations!

## References

### Standards and Specifications
- [JSON Specification (RFC 7159)](https://tools.ietf.org/html/rfc7159)
- [YAML Specification](https://yaml.org/spec/)
- [TOML Specification](https://toml.io/en/v1.0.0)
- [XML Specification](https://www.w3.org/TR/xml/)

### Related Work
- [JSON5](https://json5.org/) - JSON for humans
- [HJSON](https://hjson.github.io/) - Human JSON
- [CSON](https://github.com/bevry/cson) - CoffeeScript Object Notation
- [SDLang](https://sdlang.org/) - Simple Declarative Language

### Implementation References
- [Parsing Techniques](https://dickgrune.com/Books/PTAPG_2nd_Edition/) - Grune & Jacobs
- [Crafting Interpreters](https://craftinginterpreters.com/) - Robert Nystrom
- [ANTLR](https://www.antlr.org/) - Parser generator

---

**VIBE Format Specification v1.0**  
*Pass the vibe check* ✓

Remember: Configuration doesn't have to be complicated. Sometimes the best solution is the one that just feels right. 

Keep calm and VIBE on! 🌊

This specification is released under the MIT License (because sharing good vibes should be free).