#include "vibe.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    const char* filename = "config.vibe";
    
    if (argc > 1) {
        filename = argv[1];
    }
    
    printf("VIBE Parser Example\n");
    printf("===================\n\n");
    
    /* Create parser */
    VibeParser* parser = vibe_parser_new();
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        return 1;
    }
    
    /* Parse file */
    printf("Parsing file: %s\n\n", filename);
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
    
    printf("Parse successful!\n\n");
    
    /* Print entire structure */
    printf("Configuration structure:\n");
    vibe_value_print(config, 0);
    printf("\n\n");
    
    /* Example: Access specific values */
    printf("Accessing specific values:\n");
    printf("--------------------------\n");
    
    /* Try to access application.name */
    const char* app_name = vibe_get_string(config, "application.name");
    if (app_name) {
        printf("application.name = \"%s\"\n", app_name);
    }
    
    /* Try to access application.version */
    VibeValue* version = vibe_get(config, "application.version");
    if (version) {
        if (version->type == VIBE_TYPE_FLOAT) {
            printf("application.version = %g\n", version->as_float);
        } else if (version->type == VIBE_TYPE_STRING) {
            printf("application.version = \"%s\"\n", version->as_string);
        }
    }
    
    /* Try to access application.debug */
    VibeValue* debug = vibe_get(config, "application.debug");
    if (debug && debug->type == VIBE_TYPE_BOOLEAN) {
        printf("application.debug = %s\n", debug->as_boolean ? "true" : "false");
    }
    
    /* Try to access server.port */
    VibeValue* port = vibe_get(config, "server.port");
    if (port && port->type == VIBE_TYPE_INTEGER) {
        printf("server.port = %lld\n", (long long)port->as_integer);
    }
    
    /* Try to access server.host */
    const char* host = vibe_get_string(config, "server.host");
    if (host) {
        printf("server.host = \"%s\"\n", host);
    }
    
    /* Try to access an array */
    VibeArray* servers = vibe_get_array(config, "servers");
    if (servers) {
        printf("servers = [");
        for (size_t i = 0; i < servers->count; i++) {
            VibeValue* val = servers->values[i];
            if (val->type == VIBE_TYPE_STRING) {
                printf("%s\"%s\"", i > 0 ? ", " : "", val->as_string);
            }
        }
        printf("]\n");
    }
    
    printf("\n");
    
    /* Cleanup */
    vibe_value_free(config);
    vibe_parser_free(parser);
    
    printf("Done!\n");
    return 0;
}
