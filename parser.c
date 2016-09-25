// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stddef.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

static json_create_object_f create_object;
static json_create_array_f create_array;
static json_create_field_f create_field;
static json_alloc_string_f alloc_string;
static json_set_field_object_f set_field_object;
static json_set_field_array_f set_field_array;
static json_set_field_string_f set_field_string;
static json_set_field_number_f set_field_number;
static json_set_field_bool_f set_field_bool;
static json_set_field_null_f set_field_null;
static json_array_add_object_f array_add_object;
static json_array_add_array_f array_add_array;
static json_array_add_string_f array_add_string;
static json_array_add_number_f array_add_number;
static json_array_add_bool_f array_add_bool;
static json_array_add_null_f array_add_null;

void json_set_callbacks(
    json_create_object_f    json_create_object,
    json_create_array_f     json_create_array,
    json_create_field_f     json_create_field,
    json_alloc_string_f     json_alloc_string,
    json_set_field_object_f json_set_field_object,
    json_set_field_array_f  json_set_field_array,
    json_set_field_string_f json_set_field_string,
    json_set_field_number_f json_set_field_number,
    json_set_field_bool_f   json_set_field_bool,
    json_set_field_null_f   json_set_field_null,
    json_array_add_object_f json_array_add_object,
    json_array_add_array_f  json_array_add_array,
    json_array_add_string_f json_array_add_string,
    json_array_add_number_f json_array_add_number,
    json_array_add_bool_f   json_array_add_bool,
    json_array_add_null_f   json_array_add_null) {
    create_object    = json_create_object;
    create_array     = json_create_array;
    create_field     = json_create_field;
    alloc_string     = json_alloc_string;
    set_field_object = json_set_field_object;
    set_field_array  = json_set_field_array;
    set_field_string = json_set_field_string;
    set_field_number = json_set_field_number;
    set_field_bool   = json_set_field_bool;
    set_field_null   = json_set_field_null;
    array_add_object = json_array_add_object;
    array_add_array  = json_array_add_array;
    array_add_string = json_array_add_string;
    array_add_number = json_array_add_number;
    array_add_bool   = json_array_add_bool;
    array_add_null   = json_array_add_null;
}

static size_t json_parse_field_value(const char * json, int field);

static size_t json_parse_object(const char * json, int object) {
    const char * begin = json;
    struct jsonLexemeData data;
    enum jsonLexemeKind kind;
    char * str;
    int field;
    size_t bytes_read;
    if (JS_L_BRACE != json_next_lexeme(json, &data)) return 0;
    json += data.bytes_read;
    kind = json_next_lexeme(json, &data);
    json += data.bytes_read;
    if (JS_R_BRACE == kind) return json - begin;
    while (1) {
        if (JS_STRING != kind) return 0;
        str = alloc_string(data.measured_length);
        if (!str || !json_get_string(data.begin, str)) return 0;
        field = create_field(object, str);
        if (JS_COLON != json_next_lexeme(json, &data)) return 0;
        json += data.bytes_read;
        bytes_read = json_parse_field_value(json, field);
        if (!bytes_read) return 0;
        json += bytes_read;
        kind = json_next_lexeme(json, &data);
        json += data.bytes_read;
        if (JS_R_BRACE == kind) return json - begin;
        if (JS_COMMA != kind) return 0;
        kind = json_next_lexeme(json, &data);
        json += data.bytes_read;
    }
    return json - begin;
}

static size_t json_parse_array_value(const char * json, int array);

static size_t json_parse_array(const char * json, int array) {
    const char * begin = json;
    struct jsonLexemeData data;
    enum jsonLexemeKind kind;
    size_t bytes_read;
    if (JS_L_SQUARE_BRACKET != json_next_lexeme(json, &data)) return 0;
    json += data.bytes_read;
    kind = json_next_lexeme(json, &data);
    json += data.bytes_read;
    if (JS_R_SQUARE_BRACKET == kind) return json - begin;
    while (1) {
        bytes_read = json_parse_array_value(json, array);
        if (!bytes_read) return 0;
        json += bytes_read;
        kind = json_next_lexeme(json, &data);
        json += data.bytes_read;
        if (JS_R_SQUARE_BRACKET == kind) return json - begin;
        if (JS_COMMA != kind) return 0;
    }
    return json - begin;
}

static size_t json_parse_field_value(const char * json, int field) {
    struct jsonLexemeData data;
    int t;
    char * str;
    size_t bytes_read;
    switch (json_next_lexeme(json, &data)) {
    case JS_L_BRACE: 
        t = create_object();
        bytes_read = json_parse_object(json, t);
        if (!bytes_read) return 0;
        set_field_object(field, t);
        return bytes_read;
    case JS_L_SQUARE_BRACKET: 
        t = create_array();
        bytes_read = json_parse_array(json, t);
        if (!bytes_read) return 0;
        set_field_object(field, t);
        return bytes_read;
    case JS_TRUE: 
        set_field_bool(field, 1);
        return data.bytes_read;
    case JS_FALSE: 
        set_field_bool(field, 0);
        return data.bytes_read;
    case JS_NULL:
        set_field_null(field);
        return data.bytes_read;
    case JS_STRING:
        str = alloc_string(data.measured_length);
        if (!str || !json_get_string(data.begin, str)) return 0;
        set_field_string(field, str);
        return data.bytes_read;
    case JS_NUMBER: 
        set_field_number(field, strtod(data.begin, NULL));
        return data.bytes_read;
    default: return 0;
    }
}

static size_t json_parse_array_value(const char * json, int array) {
    struct jsonLexemeData data;
    int t;
    char * str;
    size_t bytes_read;
    switch (json_next_lexeme(json, &data)) {
    case JS_L_BRACE: 
        t = create_object();
        bytes_read = json_parse_object(json, t);
        if (!bytes_read) return 0;
        array_add_object(array, t);
        return bytes_read;
    case JS_L_SQUARE_BRACKET: 
        t = create_array();
        bytes_read = json_parse_array(json, t);
        if (!bytes_read) return 0;
        array_add_object(array, t);
        return bytes_read;
    case JS_TRUE: 
        array_add_bool(array, 1);
        return data.bytes_read;
    case JS_FALSE: 
        array_add_bool(array, 0);
        return data.bytes_read;
    case JS_NULL:
        array_add_null(array);
        return data.bytes_read;
    case JS_STRING:
        str = alloc_string(data.measured_length);
        if (!str || !json_get_string(data.begin, str)) return 0;
        array_add_string(array, str);
        return data.bytes_read;
    case JS_NUMBER: 
        array_add_number(array, strtod(data.begin, NULL));
        return data.bytes_read;
    default: return 0;
    }
}

extern size_t json_parse(const char * json) {
    struct jsonLexemeData data;
    char * str;
    double d;
    size_t t;
    switch (json_next_lexeme(json, &data)) {
    case JS_L_BRACE: 
        return (t = json_parse_object(json, create_object())) ? t + data.bytes_read : 0;
    case JS_L_SQUARE_BRACKET: 
        return (t = json_parse_array(json, create_array())) ? t + data.bytes_read : 0;
    case JS_TRUE:
        set_field_bool(-1, 1);
        break;
    case JS_FALSE:
        set_field_bool(-1, 0);
        break;
    case JS_NULL:
        set_field_null(-1);
        break;
    case JS_STRING:
        str = alloc_string(data.measured_length);
        if (!str || !json_get_string(data.begin, str)) return 0;
        set_field_string(-1, str);
        break;
    case JS_NUMBER:
        d = strtod(data.begin, NULL);
        set_field_number(-1, d);
        break;
    default: return 0;
    }
    return data.bytes_read;
}
