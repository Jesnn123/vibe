#ifndef VIBE_H
#define VIBE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* VIBE Parser - Simple C Implementation */

/* Value types */
typedef enum {
    VIBE_TYPE_NULL = 0,
    VIBE_TYPE_INTEGER,
    VIBE_TYPE_FLOAT,
    VIBE_TYPE_BOOLEAN,
    VIBE_TYPE_STRING,
    VIBE_TYPE_ARRAY,
    VIBE_TYPE_OBJECT
} VibeType;

/* Forward declarations */
typedef struct VibeValue VibeValue;
typedef struct VibeObject VibeObject;
typedef struct VibeArray VibeArray;

/* Object entry (key-value pair) */
typedef struct {
    char* key;
    VibeValue* value;
} VibeObjectEntry;

/* Object structure */
struct VibeObject {
    VibeObjectEntry* entries;
    size_t count;
    size_t capacity;
};

/* Array structure */
struct VibeArray {
    VibeValue** values;
    size_t count;
    size_t capacity;
};

/* Value structure */
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

/* Error structure */
typedef struct {
    bool has_error;
    char* message;
    int line;
    int column;
} VibeError;

/* Parser structure */
typedef struct {
    const char* input;
    size_t pos;
    size_t length;
    int line;
    int column;
    VibeError error;
} VibeParser;

/* API Functions */

/* Parser lifecycle */
VibeParser* vibe_parser_new(void);
void vibe_parser_free(VibeParser* parser);

/* Parsing functions */
VibeValue* vibe_parse_string(VibeParser* parser, const char* input);
VibeValue* vibe_parse_file(VibeParser* parser, const char* filename);

/* Value creation */
VibeValue* vibe_value_new_integer(int64_t value);
VibeValue* vibe_value_new_float(double value);
VibeValue* vibe_value_new_boolean(bool value);
VibeValue* vibe_value_new_string(const char* value);
VibeValue* vibe_value_new_array(void);
VibeValue* vibe_value_new_object(void);

/* Value access */
VibeValue* vibe_get(VibeValue* root, const char* path);
const char* vibe_get_string(VibeValue* value, const char* path);
int64_t vibe_get_int(VibeValue* value, const char* path);
double vibe_get_float(VibeValue* value, const char* path);
bool vibe_get_bool(VibeValue* value, const char* path);
VibeArray* vibe_get_array(VibeValue* value, const char* path);
VibeObject* vibe_get_object(VibeValue* value, const char* path);

/* Object operations */
void vibe_object_set(VibeObject* obj, const char* key, VibeValue* value);
VibeValue* vibe_object_get(VibeObject* obj, const char* key);

/* Array operations */
void vibe_array_push(VibeArray* arr, VibeValue* value);
VibeValue* vibe_array_get(VibeArray* arr, size_t index);

/* Cleanup */
void vibe_value_free(VibeValue* value);

/* Error handling */
VibeError vibe_get_last_error(VibeParser* parser);
void vibe_error_free(VibeError* error);

/* Debug helpers */
void vibe_value_print(VibeValue* value, int indent);

#endif /* VIBE_H */
