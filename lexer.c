// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "lexer.h"

static int utf8_bytes_left[256] = {
    [ 0x00 ... 0x7f ] =  0,
    [ 0x80 ... 0xbf ] = -1,
    [ 0xc0 ... 0xdf ] =  1,
    [ 0xe0 ... 0xef ] =  2,
    [ 0xf0 ... 0xf7 ] =  3,
    [ 0xf8 ... 0xff ] = -1,
};

static size_t get_utf8_code_point_length(int32_t hex) {
    if (hex <= 0x007f)
        return 1;
    else if (0x0080 <= hex && hex <= 0x07ff)
        return 2;
    return 3;
}

static size_t read_string_lexeme(const char * json, struct jsonLexemeData * data) {
    enum State { 
        S_NEXT_CHAR, S_ONE_LEFT, S_TWO_LEFT, S_THREE_LEFT, S_ESCAPE
    }; 
    int state = S_NEXT_CHAR;
    int bytes_left;
    size_t measured_length = 0;
    const char * begin = json;
    uint32_t hex = 0;
    if (*json++ != '\"') return 0;
    data->begin = json;
    while (1) {
        switch (state) {
        case S_NEXT_CHAR: 
            bytes_left = utf8_bytes_left[*(unsigned char *)json];
            if (-1 == bytes_left) return 0;
            switch (*json) {
            case '\"': 
                ++json;
                data->measured_length = measured_length + 1; 
                return json - begin;
            case '\\': state = S_ESCAPE; break;
            default:   measured_length++; state = S_NEXT_CHAR + bytes_left; break;
            }
            ++json;
            break;
        case S_THREE_LEFT: 
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
        case S_TWO_LEFT:   
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
        case S_ONE_LEFT:   
            if (*(unsigned char *)(json++) >> 6 != 0x2) return 0;
            ++measured_length;
            state = S_NEXT_CHAR;
            break;
        case S_ESCAPE:
            switch (*json) {
            case '\\': case '"': case '/': case 'b':
            case 'f':  case 'n': case 'r': case 't': 
                ++measured_length;
                ++json; 
                break;
            case 'u': 
                for (int i = 0; i < 4; ++i) {
                    char c = *++json;
                    if (!isxdigit(c)) return 0;
                    hex <<= 4;
                         if (c < 'A') hex |= c - '0';
                    else if (c < 'a') hex |= c - 'A' + 10;
                    else              hex |= c - 'a' + 10;
                }
                measured_length += get_utf8_code_point_length(hex);
                break;
            default: return 0;
            }
            state = S_NEXT_CHAR;
            break;
        default: exit(1); // unreachable statement
        }
    }
}

static size_t read_number_lexeme(const char * json, struct jsonLexemeData * data) {
    const char * begin = json;
    if ('-' == *json) ++json;
         if ('0' == *json) ++json;
    else if ('1' <= *json && *json <= '9') {
        ++json;
        while (isdigit(*json)) ++json;
    } else return 0;
    if ('.' == *json) {
        ++json;
        while (isdigit(*json)) ++json;
    }
    if ('e' != *json && 'E' != *json) {
        data->begin = begin;
        return json - begin;
    }
    ++json;
    if ('+' == *json || '-' == *json) ++json;
    while (isdigit(*json)) ++json;
    data->begin = begin;
    return json - begin;
}

static int is_prefix(const char * prefix, const char * str) {
    while (*prefix && *str && *prefix++ == *str++);
    return !!*prefix;
}

#define SINGLE_CHAR_LEXEME(c, k) \
    case c: data->bytes_read = json + 1 - begin; return k;
#define CONCRETE_WORD_LEXEME(c, w, k) \
    case (c):  \
        if (is_prefix((w), json)) return JS_ERROR; \
        data->bytes_read = json - begin + sizeof(w) - 1; \
        return (k);
extern enum jsonLexemeKind json_next_lexeme(
        const char * json, struct jsonLexemeData * data) {
    const char * begin = json;
    size_t t;
    while (isspace(*json)) ++json;
    switch (*json) {
    case '\0': return JS_ERROR;
    SINGLE_CHAR_LEXEME('{', JS_L_BRACE)
    SINGLE_CHAR_LEXEME('}', JS_R_BRACE)
    SINGLE_CHAR_LEXEME('[', JS_L_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(']', JS_R_SQUARE_BRACKET)
    SINGLE_CHAR_LEXEME(',', JS_COMMA)
    SINGLE_CHAR_LEXEME(':', JS_COLON)
    CONCRETE_WORD_LEXEME('t', "true",  JS_TRUE)
    CONCRETE_WORD_LEXEME('f', "false", JS_FALSE)
    CONCRETE_WORD_LEXEME('n', "null",  JS_NULL)
    case '\"': 
        t = read_string_lexeme(json, data); 
        if (!t) return JS_ERROR;
        json += t;
        data->bytes_read = json - begin;
        return JS_STRING;
    default:
        t = read_number_lexeme(json, data); 
        if (!t) return JS_ERROR;
        json += t;
        data->bytes_read = json - begin;
        return JS_NUMBER;
    }
}
#undef SINGLE_CHAR_LEXEME
#undef CONCRETE_WORD_LEXEME

static size_t put_utf8_code_point(int32_t hex, char * out) {
    if (hex <= 0x007f) {
        *out = hex;
        return 1;
    } else if (0x0080 <= hex && hex <= 0x07ff) {
        *out++ = 0xc0 | hex >> 6;
        *out   = 0x80 | (hex & 0x3f);
        return 2;
    } else {
        *out++ = 0xe0 | hex >> 12;
        *out++ = 0x80 | ((hex >> 6) & 0x3f);
        *out   = 0x80 | (hex & 0x3f);
        return 3;
    } 
}

extern size_t json_get_string(const char * json, char * out) {
    enum State { 
        S_NEXT_CHAR, S_ONE_LEFT, S_TWO_LEFT, S_THREE_LEFT, S_ESCAPE
    }; 
    const char * begin = json;
    int state = S_NEXT_CHAR;
    int bytes_left;
    int32_t hex;
    while (1) {
        switch (state) {
        case S_NEXT_CHAR: 
            bytes_left = utf8_bytes_left[*(unsigned char *)json];
            if (-1 == bytes_left) return 0;
            switch (*json) {
            case '\"': *out = '\0'; return json + 1 - begin;
            case '\\': state = S_ESCAPE; json++; break;
            default:   
                state = S_NEXT_CHAR + bytes_left; 
                *out++ = *json++;
                break;
            }
            break;
        case S_THREE_LEFT: 
            if (*(unsigned char *)(json) >> 6 != 0x2) return 0;
            *out++ = *json++;
        case S_TWO_LEFT:   
            if (*(unsigned char *)(json) >> 6 != 0x2) return 0;
            *out++ = *json++;
        case S_ONE_LEFT:   
            if (*(unsigned char *)(json) >> 6 != 0x2) return 0;
            *out++ = *json++;
            state = S_NEXT_CHAR;
            break;
        case S_ESCAPE:
            state = S_NEXT_CHAR;
            switch (*json++) {
            case '\\': *out++ = '\\'; break;
            case  '"': *out++ =  '"'; break;
            case  '/': *out++ =  '/'; break;
            case  'b': *out++ = '\b'; break;
            case  'f': *out++ = '\f'; break;
            case  'n': *out++ = '\n'; break;
            case  'r': *out++ = '\r'; break;
            case  't': *out++ = '\t'; break;
            case  'u':
                hex = 0;
                for (int i = 0; i < 4; ++i) {
                    char c = *json++;
                    if (!isxdigit(c)) return 0;
                    hex <<= 4;
                         if (c < 'A') hex |= c - '0';
                    else if (c < 'a') hex |= c - 'A' + 10;
                    else              hex |= c - 'a' + 10;
                }
                out += put_utf8_code_point(hex, out);
                break;
            default: return 0;
            }
            break;
        default: exit(1); // unreachable statement
        }
    }
}
