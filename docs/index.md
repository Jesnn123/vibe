---
layout: default
title: VIBE - Visual, Intuitive, Basic, Expressive
description: A human-friendly configuration language that just works
---

<div class="hero">
  <div class="hero-content">
    <h1 class="hero-title">
      <span class="vibe-logo">VIBE</span>
      <span class="hero-subtitle">Configuration Language</span>
    </h1>
    <p class="hero-description">
      Visual • Intuitive • Basic • Expressive
    </p>
    <p class="hero-tagline">
      The configuration language that feels natural to write and delightful to read
    </p>
    <div class="hero-buttons">
      <a href="#quick-start" class="btn btn-primary">Get Started</a>
      <a href="#examples" class="btn btn-secondary">See Examples</a>
      <a href="https://github.com/1ay1/vibe" class="btn btn-outline">GitHub</a>
    </div>
  </div>
</div>

## Why VIBE? ✨

<div class="features-grid">
  <div class="feature">
    <div class="feature-icon">🎯</div>
    <h3>No Comma Chaos</h3>
    <p>Forget about missing commas and trailing syntax. VIBE uses natural whitespace separation.</p>
  </div>
  
  <div class="feature">
    <div class="feature-icon">🧠</div>
    <h3>Smart Type Detection</h3>
    <p>Automatically infers types from values. Numbers look like numbers, strings look like strings.</p>
  </div>
  
  <div class="feature">
    <div class="feature-icon">💬</div>
    <h3>Comments Everywhere</h3>
    <p>Add comments anywhere with `#`. Document your configuration inline.</p>
  </div>
  
  <div class="feature">
    <div class="feature-icon">🌍</div>
    <h3>Unicode Ready</h3>
    <p>Full UTF-8 support in quoted strings. International configuration made easy.</p>
  </div>
  
  <div class="feature">
    <div class="feature-icon">⚡</div>
    <h3>Lightning Fast</h3>
    <p>Simple grammar designed for speed. Parses large configs in milliseconds.</p>
  </div>
  
  <div class="feature">
    <div class="feature-icon">🎨</div>
    <h3>Beautiful Syntax</h3>
    <p>Clean, readable format that looks great in any editor with proper highlighting.</p>
  </div>
</div>

## Quick Start {#quick-start}

### Installation

```bash
# Clone the repository
git clone https://github.com/1ay1/vibe.git
cd vibe

# Build the library
make

# Or use the development environment
cd dev_env
./setup.sh
make build
```

### Your First VIBE File

Create a file called `config.vibe`:

```vibe
# My first VIBE configuration
app {
  name "My Awesome App"
  version 1.0
  debug true
  
  database {
    host localhost
    port 5432
    pool_size 10
  }
  
  servers [web1.com web2.com api.com]
}
```

### Parse It

```c
#include "vibe.h"

int main() {
    VibeParser* parser = vibe_parser_new();
    VibeValue* config = vibe_parse_file(parser, "config.vibe");
    
    // Access values easily
    const char* app_name = vibe_get_string(config, "app.name");
    int64_t db_port = vibe_get_int(config, "app.database.port");
    
    printf("App: %s, DB Port: %ld\n", app_name, db_port);
    
    vibe_value_free(config);
    vibe_parser_free(parser);
    return 0;
}
```

## Examples {#examples}

### Web Server Configuration

```vibe
# Web server configuration
server {
  host 0.0.0.0
  port 8080
  workers 4
  
  ssl {
    enabled true
    cert_path "/etc/ssl/server.crt"
    key_path "/etc/ssl/server.key"
    protocols [TLSv1.2 TLSv1.3]
  }
  
  routes {
    api {
      path "/api/v1/*"
      timeout 30.0
      rate_limit 1000
    }
    
    static {
      path "/static/*"
      directory "/var/www/static"
      cache_ttl 3600
    }
  }
}

# Logging configuration
logging {
  level info
  output "/var/log/server.log"
  rotate_size "100MB"
  max_files 10
}
```

### Database Configuration

```vibe
# Multi-environment database config
environments {
  development {
    database {
      type postgresql
      host localhost
      port 5432
      name myapp_dev
      username dev_user
      password "dev_secret"
      
      connection_pool {
        min_connections 2
        max_connections 10
        idle_timeout 300
      }
    }
  }
  
  production {
    database {
      type postgresql
      host db-cluster.internal
      port 5432
      name myapp_prod
      username app_user
      # Password from environment variable
      
      connection_pool {
        min_connections 10
        max_connections 50
        idle_timeout 600
      }
      
      backup {
        enabled true
        schedule "0 2 * * *"  # Daily at 2 AM
        retention_days 30
      }
    }
  }
}
```

### Microservices Configuration

```vibe
# Microservices architecture
services {
  auth_service {
    image "myapp/auth:latest"
    replicas 3
    ports [3001 3002 3003]
    
    environment {
      JWT_SECRET "super-secret-key"
      DATABASE_URL "postgres://auth:pass@db:5432/auth"
      REDIS_URL "redis://cache:6379"
    }
    
    health_check {
      path "/health"
      interval 30
      timeout 5
      retries 3
    }
  }
  
  api_service {
    image "myapp/api:latest"
    replicas 5
    ports [4001 4002 4003 4004 4005]
    
    dependencies [auth_service database]
    
    resources {
      cpu "500m"
      memory "512Mi"
      disk "1Gi"
    }
  }
  
  worker_service {
    image "myapp/worker:latest"
    replicas 2
    
    queues [emails notifications reports]
    
    scaling {
      min_replicas 1
      max_replicas 10
      target_queue_length 100
    }
  }
}

# Load balancer configuration
load_balancer {
  algorithm round_robin
  health_checks true
  
  # Use object for complex backend configs instead of array
  backends {
    api1 {
      host api1.internal
      port 4000
      weight 1
    }
    api2 {
      host api2.internal
      port 4000
      weight 1
    }
    api3 {
      host api3.internal
      port 4000
      weight 2
    }
  }
  
  ssl_termination true
  compression true
}
```

## Language Reference

### Data Types

VIBE automatically detects data types:

<div class="type-examples">
  <div class="type-example">
    <h4>Strings</h4>
    <pre><code># Unquoted (simple values)
host localhost
protocol https

# Quoted (with spaces/special chars)
message "Hello, World! 🌍"
path "C:\Program Files\App"</code></pre>
  </div>
  
  <div class="type-example">
    <h4>Numbers</h4>
    <pre><code># Integers
port 8080
count 42
negative -17

# Floats
pi 3.14159
timeout 30.5
ratio -0.25</code></pre>
  </div>
  
  <div class="type-example">
    <h4>Booleans</h4>
    <pre><code>enabled true
debug false
ssl_required true</code></pre>
  </div>
  
  <div class="type-example">
    <h4>Arrays</h4>
    <pre><code># Arrays contain only scalar values
ports [8080 8081 8082]
hosts [web1.com web2.com]
flags [enabled debug verbose]

# Multi-line arrays
servers [
  production.example.com
  staging.example.com
  development.example.com
]

# Mixed scalar types
mixed [42 "hello" true 3.14]</code></pre>
  </div>
  
  <div class="type-example">
    <h4>Objects</h4>
    <pre><code># Use objects for complex nested data
database {
  primary {
    host db1.example.com
    port 5432
    ssl_enabled true
  }
  
  replica {
    host db2.example.com
    port 5432
    ssl_enabled false
  }
}

# For multiple similar items, use named objects
# NOT arrays with nested objects
servers {
  web1 {
    host web1.example.com
    port 8080
  }
  web2 {
    host web2.example.com
    port 8080
  }
}</code></pre>
  </div>
  
  <div class="type-example">
    <h4>Comments</h4>
    <pre><code># Full line comment
key value  # Inline comment

# Multi-line explanation
# This is a complex configuration
# that requires detailed explanation
complex_setting 42</code></pre>
  </div>
</div>

### Grammar Overview

```ebnf
config        = { statement } ;
statement     = ( assignment | object | array | comment ) ;
assignment    = identifier scalar_value ;
object        = identifier "{" { statement } "}" ;
array         = identifier "[" [ value_list ] "]" ;
scalar_value  = string | number | boolean ;
```

## API Reference

### C Library

```c
// Parser lifecycle
VibeParser* vibe_parser_new(void);
void vibe_parser_free(VibeParser* parser);

// Parsing
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);

// Value access
const char* vibe_get_string(VibeValue* value, const char* path);
int64_t vibe_get_int(VibeValue* value, const char* path);
double vibe_get_float(VibeValue* value, const char* path);
bool vibe_get_bool(VibeValue* value, const char* path);

// Memory management
void vibe_value_free(VibeValue* value);
```

### Path Notation

Access nested values using dot notation:

```c
// For this VIBE config:
// server {
//   database {
//     host localhost
//     port 5432
//   }
// }

const char* host = vibe_get_string(config, "server.database.host");
int64_t port = vibe_get_int(config, "server.database.port");
```

## Comparison

<div class="comparison-table">
  <table>
    <thead>
      <tr>
        <th>Feature</th>
        <th>VIBE</th>
        <th>JSON</th>
        <th>YAML</th>
        <th>TOML</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Comments</td>
        <td>✅ Everywhere</td>
        <td>❌ None</td>
        <td>✅ Yes</td>
        <td>✅ Yes</td>
      </tr>
      <tr>
        <td>Trailing Commas</td>
        <td>✅ No commas needed</td>
        <td>❌ Required, error-prone</td>
        <td>✅ Not needed</td>
        <td>✅ Not needed</td>
      </tr>
      <tr>
        <td>Multiline Strings</td>
        <td>🔄 Planned</td>
        <td>❌ Escape sequences only</td>
        <td>✅ Multiple ways</td>
        <td>✅ Yes</td>
      </tr>
      <tr>
        <td>Unicode</td>
        <td>✅ Full UTF-8</td>
        <td>✅ Yes</td>
        <td>✅ Yes</td>
        <td>✅ Yes</td>
      </tr>
      <tr>
        <td>Nesting</td>
        <td>✅ Unlimited</td>
        <td>✅ Unlimited</td>
        <td>✅ Unlimited</td>
        <td>🔶 Limited</td>
      </tr>
      <tr>
        <td>Parsing Speed</td>
        <td>✅ Very Fast</td>
        <td>✅ Fast</td>
        <td>🔶 Moderate</td>
        <td>✅ Fast</td>
      </tr>
      <tr>
        <td>Human Readability</td>
        <td>✅ Excellent</td>
        <td>🔶 Good</td>
        <td>✅ Excellent</td>
        <td>✅ Very Good</td>
      </tr>
    </tbody>
  </table>
</div>

## Getting Involved

### Contributing

We welcome contributions! Here's how to get started:

1. **Fork the repository** on GitHub
2. **Clone your fork** locally
3. **Create a feature branch** (`git checkout -b feature/amazing-feature`)
4. **Make your changes** and add tests
5. **Run the test suite** (`make test`)
6. **Commit your changes** (`git commit -m 'Add amazing feature'`)
7. **Push to your branch** (`git push origin feature/amazing-feature`)
8. **Open a Pull Request**

### Development Setup

```bash
# Clone and setup development environment
git clone https://github.com/1ay1/vibe.git
cd vibe/dev_env
./setup.sh

# Available development commands
make help        # Show all available targets
make build       # Build the library
make test        # Run tests
make debug       # Build debug version
make docs        # Generate documentation
```

### Reporting Issues

Found a bug or have a feature request? Please [open an issue](https://github.com/1ay1/vibe/issues) with:

- **Clear description** of the problem or feature
- **Minimal example** that demonstrates the issue
- **Expected vs actual behavior**
- **System information** (OS, compiler version, etc.)

## License

VIBE is released under the [MIT License](https://github.com/1ay1/vibe/blob/main/LICENSE).

---

<div class="footer-cta">
  <h2>Ready to try VIBE?</h2>
  <p>Join the community of developers who are building better configurations</p>
  <div class="footer-buttons">
    <a href="https://github.com/1ay1/vibe" class="btn btn-primary">Star on GitHub</a>
    <a href="#quick-start" class="btn btn-secondary">Get Started</a>
  </div>
</div>

<style>
/* Hero Section */
.hero {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 4rem 0;
  text-align: center;
  margin: -2rem -2rem 3rem -2rem;
}

.hero-content {
  max-width: 800px;
  margin: 0 auto;
  padding: 0 2rem;
}

.vibe-logo {
  font-size: 4rem;
  font-weight: 900;
  letter-spacing: -0.05em;
  background: linear-gradient(45deg, #ff6b6b, #4ecdc4, #45b7d1, #96ceb4);
  background-size: 400% 400%;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  animation: gradient 4s ease infinite;
}

@keyframes gradient {
  0% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
  100% { background-position: 0% 50%; }
}

.hero-subtitle {
  display: block;
  font-size: 1.5rem;
  font-weight: 300;
  margin-top: 0.5rem;
  opacity: 0.9;
}

.hero-description {
  font-size: 1.3rem;
  margin: 1rem 0;
  opacity: 0.95;
}

.hero-tagline {
  font-size: 1.1rem;
  margin: 1.5rem 0 2rem 0;
  opacity: 0.9;
}

.hero-buttons {
  display: flex;
  gap: 1rem;
  justify-content: center;
  flex-wrap: wrap;
}

/* Buttons */
.btn {
  display: inline-block;
  padding: 0.75rem 1.5rem;
  border-radius: 0.5rem;
  text-decoration: none;
  font-weight: 600;
  transition: all 0.3s ease;
  border: none;
  cursor: pointer;
}

.btn-primary {
  background: #ff6b6b;
  color: white;
}

.btn-primary:hover {
  background: #ff5252;
  transform: translateY(-2px);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.2);
  color: white;
  backdrop-filter: blur(10px);
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.3);
  transform: translateY(-2px);
}

.btn-outline {
  background: transparent;
  color: white;
  border: 2px solid rgba(255, 255, 255, 0.3);
}

.btn-outline:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: rgba(255, 255, 255, 0.5);
  transform: translateY(-2px);
}

/* Features Grid */
.features-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 2rem;
  margin: 3rem 0;
}

.feature {
  padding: 2rem;
  border-radius: 1rem;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  text-align: center;
  transition: transform 0.3s ease;
}

.feature:hover {
  transform: translateY(-5px);
}

.feature-icon {
  font-size: 3rem;
  margin-bottom: 1rem;
}

.feature h3 {
  color: #2c3e50;
  margin-bottom: 1rem;
}

.feature p {
  color: #5a6c7d;
  line-height: 1.6;
}

/* Type Examples */
.type-examples {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 1.5rem;
  margin: 2rem 0;
}

.type-example {
  border: 1px solid #e1e8ed;
  border-radius: 0.5rem;
  overflow: hidden;
}

.type-example h4 {
  background: #f8f9fa;
  margin: 0;
  padding: 1rem;
  border-bottom: 1px solid #e1e8ed;
  color: #2c3e50;
}

.type-example pre {
  margin: 0;
  padding: 1rem;
  background: #fff;
}

.type-example code {
  font-size: 0.9rem;
  line-height: 1.4;
}

/* Comparison Table */
.comparison-table {
  margin: 2rem 0;
  overflow-x: auto;
}

.comparison-table table {
  width: 100%;
  border-collapse: collapse;
  background: white;
  border-radius: 0.5rem;
  overflow: hidden;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.comparison-table th,
.comparison-table td {
  padding: 1rem;
  text-align: left;
  border-bottom: 1px solid #e1e8ed;
}

.comparison-table th {
  background: #f8f9fa;
  font-weight: 600;
  color: #2c3e50;
}

.comparison-table tr:hover {
  background: #f8f9fa;
}

/* Footer CTA */
.footer-cta {
  text-align: center;
  padding: 3rem 0;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: 1rem;
  margin: 3rem 0;
}

.footer-cta h2 {
  margin-bottom: 1rem;
}

.footer-cta p {
  margin-bottom: 2rem;
  opacity: 0.9;
}

.footer-buttons {
  display: flex;
  gap: 1rem;
  justify-content: center;
  flex-wrap: wrap;
}

/* Code Blocks */
pre {
  background: #f8f9fa;
  border: 1px solid #e1e8ed;
  border-radius: 0.5rem;
  padding: 1rem;
  overflow-x: auto;
}

code {
  font-family: 'Fira Code', 'Monaco', 'Consolas', monospace;
  font-size: 0.9em;
}

/* Responsive Design */
@media (max-width: 768px) {
  .hero {
    padding: 2rem 0;
  }
  
  .vibe-logo {
    font-size: 3rem;
  }
  
  .hero-buttons {
    flex-direction: column;
    align-items: center;
  }
  
  .btn {
    width: 200px;
  }
  
  .features-grid {
    grid-template-columns: 1fr;
  }
  
  .type-examples {
    grid-template-columns: 1fr;
  }
}
</style>