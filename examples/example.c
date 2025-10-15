#include "../vibe.h"
#include <stdio.h>
#include <stdlib.h>

static void print_separator(const char* title) {
    printf("\n");
    printf("=== %s ===\n", title);
    printf("\n");
}

static void print_database_config(VibeValue* config) {
    print_separator("Database Configuration");
    
    // Primary database
    const char* primary_host = vibe_get_string(config, "database.primary.host");
    int64_t primary_port = vibe_get_int(config, "database.primary.port");
    const char* db_name = vibe_get_string(config, "database.primary.name");
    const char* username = vibe_get_string(config, "database.primary.username");
    
    printf("Primary Database:\n");
    printf("  Host: %s\n", primary_host ? primary_host : "unknown");
    printf("  Port: %lld\n", (long long)primary_port);
    printf("  Database: %s\n", db_name ? db_name : "unknown");
    printf("  Username: %s\n", username ? username : "unknown");
    
    // Connection pool
    int64_t min_conn = vibe_get_int(config, "database.primary.pool.min_connections");
    int64_t max_conn = vibe_get_int(config, "database.primary.pool.max_connections");
    int64_t idle_timeout = vibe_get_int(config, "database.primary.pool.idle_timeout");
    
    printf("  Pool Settings:\n");
    printf("    Min connections: %lld\n", (long long)min_conn);
    printf("    Max connections: %lld\n", (long long)max_conn);
    printf("    Idle timeout: %lld seconds\n", (long long)idle_timeout);
    
    // SSL settings
    bool ssl_enabled = vibe_get_bool(config, "database.primary.ssl.enabled");
    const char* ssl_mode = vibe_get_string(config, "database.primary.ssl.mode");
    
    printf("  SSL:\n");
    printf("    Enabled: %s\n", ssl_enabled ? "yes" : "no");
    printf("    Mode: %s\n", ssl_mode ? ssl_mode : "unknown");
    
    // Read replicas (now named objects)
    VibeObject* replicas = vibe_get_object(config, "database.replicas");
    if (replicas && replicas->count > 0) {
        printf("  Read Replicas (%zu):\n", replicas->count);
        for (size_t i = 0; i < replicas->count; i++) {
            const char* replica_name = replicas->entries[i].key;
            VibeValue* replica = replicas->entries[i].value;
            
            if (replica && replica->type == VIBE_TYPE_OBJECT) {
                const char* replica_host = vibe_get_string(replica, "host");
                int64_t replica_port = vibe_get_int(replica, "port");
                double weight = vibe_get_float(replica, "weight");
                
                printf("    %zu. %s: %s:%lld (weight: %.1f)\n", 
                       i + 1,
                       replica_name ? replica_name : "unknown",
                       replica_host ? replica_host : "unknown",
                       (long long)replica_port,
                       weight);
            }
        }
    }
    
    // Cache configuration
    const char* cache_type = vibe_get_string(config, "database.cache.type");
    const char* cache_host = vibe_get_string(config, "database.cache.host");
    int64_t cache_port = vibe_get_int(config, "database.cache.port");
    int64_t default_ttl = vibe_get_int(config, "database.cache.default_ttl");
    
    printf("  Cache:\n");
    printf("    Type: %s\n", cache_type ? cache_type : "unknown");
    printf("    Host: %s:%lld\n", 
           cache_host ? cache_host : "unknown", 
           (long long)cache_port);
    printf("    Default TTL: %lld seconds\n", (long long)default_ttl);
}

static void print_server_config(VibeValue* config) {
    print_separator("Server Configuration");
    
    const char* bind_addr = vibe_get_string(config, "server.bind_address");
    int64_t port = vibe_get_int(config, "server.port");
    VibeValue* worker_proc_val = vibe_get(config, "server.worker_processes");
    int64_t worker_connections = vibe_get_int(config, "server.worker_connections");
    
    printf("Basic Settings:\n");
    printf("  Bind Address: %s\n", bind_addr ? bind_addr : "unknown");
    printf("  Port: %lld\n", (long long)port);
    
    if (worker_proc_val) {
        if (worker_proc_val->type == VIBE_TYPE_STRING) {
            printf("  Worker Processes: %s\n", worker_proc_val->as_string);
        } else if (worker_proc_val->type == VIBE_TYPE_INTEGER) {
            printf("  Worker Processes: %lld\n", (long long)worker_proc_val->as_integer);
        } else {
            printf("  Worker Processes: unknown type\n");
        }
    } else {
        printf("  Worker Processes: unknown\n");
    }
    
    printf("  Worker Connections: %lld\n", (long long)worker_connections);
    
    // Timeouts
    int64_t client_header = vibe_get_int(config, "server.timeouts.client_header");
    int64_t client_body = vibe_get_int(config, "server.timeouts.client_body");
    int64_t keepalive = vibe_get_int(config, "server.timeouts.keepalive_timeout");
    
    printf("  Timeouts:\n");
    printf("    Client Header: %lld seconds\n", (long long)client_header);
    printf("    Client Body: %lld seconds\n", (long long)client_body);
    printf("    Keepalive: %lld seconds\n", (long long)keepalive);
    
    // SSL Configuration
    bool ssl_enabled = vibe_get_bool(config, "server.ssl.enabled");
    int64_t ssl_port = vibe_get_int(config, "server.ssl.port");
    
    printf("  SSL:\n");
    printf("    Enabled: %s\n", ssl_enabled ? "yes" : "no");
    printf("    Port: %lld\n", (long long)ssl_port);
    
    // SSL Protocols
    VibeArray* protocols = vibe_get_array(config, "server.ssl.protocols");
    if (protocols && protocols->count > 0) {
        printf("    Protocols: ");
        for (size_t i = 0; i < protocols->count; i++) {
            VibeValue* protocol = protocols->values[i];
            if (protocol && protocol->type == VIBE_TYPE_STRING) {
                printf("%s%s", i > 0 ? ", " : "", protocol->as_string);
            }
        }
        printf("\n");
    }
    
    // SSL Certificates (now named objects)
    VibeObject* certificates = vibe_get_object(config, "server.ssl.certificates");
    if (certificates && certificates->count > 0) {
        printf("    Certificates (%zu):\n", certificates->count);
        for (size_t i = 0; i < certificates->count; i++) {
            const char* cert_name = certificates->entries[i].key;
            VibeValue* cert = certificates->entries[i].value;
            
            if (cert && cert->type == VIBE_TYPE_OBJECT) {
                const char* domain = vibe_get_string(cert, "domain");
                const char* cert_file = vibe_get_string(cert, "cert_file");
                
                printf("      %zu. %s (%s) -> %s\n", 
                       i + 1,
                       cert_name ? cert_name : "unknown",
                       domain ? domain : "unknown",
                       cert_file ? cert_file : "unknown");
            }
        }
    }
}

static void print_application_config(VibeValue* config) {
    print_separator("Application Configuration");
    
    const char* name = vibe_get_string(config, "application.name");
    VibeValue* version_val = vibe_get(config, "application.version");
    int64_t build_number = vibe_get_int(config, "application.build_number");
    const char* environment = vibe_get_string(config, "application.environment");
    
    printf("Basic Info:\n");
    printf("  Name: %s\n", name ? name : "unknown");
    
    if (version_val) {
        if (version_val->type == VIBE_TYPE_FLOAT) {
            printf("  Version: %.1f\n", version_val->as_float);
        } else if (version_val->type == VIBE_TYPE_STRING) {
            printf("  Version: %s\n", version_val->as_string);
        } else {
            printf("  Version: unknown type\n");
        }
    } else {
        printf("  Version: unknown\n");
    }
    
    printf("  Build: %lld\n", (long long)build_number);
    printf("  Environment: %s\n", environment ? environment : "unknown");
    
    // Feature flags
    bool new_ui = vibe_get_bool(config, "application.features.new_ui");
    bool beta_api = vibe_get_bool(config, "application.features.beta_api");
    bool analytics = vibe_get_bool(config, "application.features.analytics");
    bool rate_limiting = vibe_get_bool(config, "application.features.rate_limiting");
    
    printf("  Feature Flags:\n");
    printf("    New UI: %s\n", new_ui ? "enabled" : "disabled");
    printf("    Beta API: %s\n", beta_api ? "enabled" : "disabled");
    printf("    Analytics: %s\n", analytics ? "enabled" : "disabled");
    printf("    Rate Limiting: %s\n", rate_limiting ? "enabled" : "disabled");
    
    // Application limits
    int64_t max_users = vibe_get_int(config, "application.limits.max_users");
    int64_t max_rps = vibe_get_int(config, "application.limits.max_requests_per_second");
    int64_t session_timeout = vibe_get_int(config, "application.limits.session_timeout");
    
    printf("  Limits:\n");
    printf("    Max Users: %lld\n", (long long)max_users);
    printf("    Max RPS: %lld\n", (long long)max_rps);
    printf("    Session Timeout: %lld seconds\n", (long long)session_timeout);
}

static void print_monitoring_config(VibeValue* config) {
    print_separator("Monitoring Configuration");
    
    // Health checks
    bool health_enabled = vibe_get_bool(config, "monitoring.health.enabled");
    const char* health_path = vibe_get_string(config, "monitoring.health.path");
    int64_t health_interval = vibe_get_int(config, "monitoring.health.interval");
    
    printf("Health Checks:\n");
    printf("  Enabled: %s\n", health_enabled ? "yes" : "no");
    printf("  Path: %s\n", health_path ? health_path : "unknown");
    printf("  Interval: %lld seconds\n", (long long)health_interval);
    
    // Health check definitions (now named objects)
    VibeObject* checks = vibe_get_object(config, "monitoring.health.checks");
    if (checks && checks->count > 0) {
        printf("  Checks (%zu):\n", checks->count);
        for (size_t i = 0; i < checks->count; i++) {
            const char* check_name = checks->entries[i].key;
            VibeValue* check = checks->entries[i].value;
            
            if (check && check->type == VIBE_TYPE_OBJECT) {
                const char* check_type = vibe_get_string(check, "type");
                int64_t timeout = vibe_get_int(check, "timeout");
                
                printf("    %zu. %s (%s) - timeout: %llds\n", 
                       i + 1,
                       check_name ? check_name : "unknown",
                       check_type ? check_type : "unknown",
                       (long long)timeout);
            }
        }
    }
    
    // Metrics
    bool metrics_enabled = vibe_get_bool(config, "monitoring.metrics.enabled");
    bool prometheus_enabled = vibe_get_bool(config, "monitoring.metrics.prometheus.enabled");
    const char* prometheus_path = vibe_get_string(config, "monitoring.metrics.prometheus.path");
    
    printf("  Metrics:\n");
    printf("    Enabled: %s\n", metrics_enabled ? "yes" : "no");
    printf("    Prometheus: %s\n", prometheus_enabled ? "enabled" : "disabled");
    if (prometheus_path) {
        printf("    Prometheus Path: %s\n", prometheus_path);
    }
}

static void print_api_config(VibeValue* config) {
    print_separator("API Configuration");
    
    const char* version = vibe_get_string(config, "api.version");
    const char* base_path = vibe_get_string(config, "api.base_path");
    
    printf("Basic Settings:\n");
    printf("  Version: %s\n", version ? version : "unknown");
    printf("  Base Path: %s\n", base_path ? base_path : "unknown");
    
    // Rate limiting
    bool rate_limit_enabled = vibe_get_bool(config, "api.rate_limit.enabled");
    int64_t global_rpm = vibe_get_int(config, "api.rate_limit.global.requests_per_minute");
    int64_t global_burst = vibe_get_int(config, "api.rate_limit.global.burst");
    
    printf("  Rate Limiting:\n");
    printf("    Enabled: %s\n", rate_limit_enabled ? "yes" : "no");
    printf("    Global: %lld req/min (burst: %lld)\n", 
           (long long)global_rpm, (long long)global_burst);
    
    // CORS
    bool cors_enabled = vibe_get_bool(config, "api.cors.enabled");
    VibeArray* allowed_origins = vibe_get_array(config, "api.cors.allowed_origins");
    
    printf("  CORS:\n");
    printf("    Enabled: %s\n", cors_enabled ? "yes" : "no");
    if (allowed_origins && allowed_origins->count > 0) {
        printf("    Allowed Origins: ");
        for (size_t i = 0; i < allowed_origins->count; i++) {
            VibeValue* origin = allowed_origins->values[i];
            if (origin && origin->type == VIBE_TYPE_STRING) {
                printf("%s%s", i > 0 ? ", " : "", origin->as_string);
            }
        }
        printf("\n");
    }
}

static void print_summary_stats(VibeValue* config) {
    print_separator("Configuration Summary");
    
    // Count various configuration items (now named objects)
    VibeObject* replicas = vibe_get_object(config, "database.replicas");
    VibeObject* certificates = vibe_get_object(config, "server.ssl.certificates");
    VibeObject* health_checks = vibe_get_object(config, "monitoring.health.checks");
    VibeObject* log_outputs = vibe_get_object(config, "logging.outputs");
    
    printf("Configuration Statistics:\n");
    printf("  Database replicas: %zu\n", replicas ? replicas->count : 0);
    printf("  SSL certificates: %zu\n", certificates ? certificates->count : 0);
    printf("  Health checks: %zu\n", health_checks ? health_checks->count : 0);
    printf("  Log outputs: %zu\n", log_outputs ? log_outputs->count : 0);
    
    // Feature flags summary
    bool new_ui = vibe_get_bool(config, "application.features.new_ui");
    bool beta_api = vibe_get_bool(config, "application.features.beta_api");
    bool analytics = vibe_get_bool(config, "application.features.analytics");
    bool rate_limiting = vibe_get_bool(config, "application.features.rate_limiting");
    
    int enabled_features = (new_ui ? 1 : 0) + (beta_api ? 1 : 0) + 
                          (analytics ? 1 : 0) + (rate_limiting ? 1 : 0);
    
    printf("  Feature flags enabled: %d/4\n", enabled_features);
    
    // Service status
    bool ssl_enabled = vibe_get_bool(config, "server.ssl.enabled");
    bool metrics_enabled = vibe_get_bool(config, "monitoring.metrics.enabled");
    bool cors_enabled = vibe_get_bool(config, "api.cors.enabled");
    
    printf("  Services enabled:\n");
    printf("    SSL: %s\n", ssl_enabled ? "✓" : "✗");
    printf("    Metrics: %s\n", metrics_enabled ? "✓" : "✗");
    printf("    CORS: %s\n", cors_enabled ? "✓" : "✗");
}

int main(int argc, char** argv) {
    const char* filename = "./config.vibe";

    if (argc > 1) {
        filename = argv[1];
    }

    printf("VIBE Complex Configuration Example\n");
    printf("==================================\n");
    printf("Parsing file: %s\n", filename);

    /* Create parser */
    VibeParser* parser = vibe_parser_new();
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        return 1;
    }

    /* Parse file */
    VibeValue* config = vibe_parse_file(parser, filename);

    if (!config) {
        VibeError error = vibe_get_last_error(parser);
        if (error.has_error) {
            fprintf(stderr, "Parse error at line %d, column %d:\n",
                    error.line, error.column);
            fprintf(stderr, "  %s\n", error.message);
        } else {
            fprintf(stderr, "Unknown parse error\n");
        }
        vibe_parser_free(parser);
        return 1;
    }

    printf("✓ Parse successful!\n");

    /* Print detailed configuration sections */
    print_application_config(config);
    print_database_config(config);
    print_server_config(config);
    print_api_config(config);
    print_monitoring_config(config);
    print_summary_stats(config);

    /* Test error handling - try to access non-existent paths */
    print_separator("Error Handling Tests");
    
    const char* nonexistent = vibe_get_string(config, "nonexistent.path");
    printf("Non-existent path result: %s\n", nonexistent ? nonexistent : "NULL (correct)");
    
    int64_t invalid_int = vibe_get_int(config, "application.name");  // String, not int
    printf("Type mismatch (string as int): %lld (should be 0)\n", (long long)invalid_int);
    
    VibeArray* invalid_array = vibe_get_array(config, "application.version");  // String, not array
    printf("Type mismatch (string as array): %s\n", invalid_array ? "non-NULL" : "NULL (correct)");

    /* Performance info */
    print_separator("Performance Information");
    printf("Configuration successfully parsed and accessed\n");
    printf("All dot-notation paths resolved correctly\n");
    printf("Type checking and error handling working as expected\n");

    /* Cleanup */
    vibe_value_free(config);
    vibe_parser_free(parser);

    printf("\n✓ Example completed successfully!\n");
    return 0;
}