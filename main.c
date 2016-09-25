#include <stddef.h>
#include <stdio.h>

#include "lexer.h"

int main() {
    const char * json = "{ \"string\" : -42.e4 }";
    enum jsonLexemeKind k;
    struct jsonLexemeData lexeme_data;
    while (*json) {
        switch (next_lexeme(json, &lexeme_data)) {
        case JS_ERROR:            printf("error\n"); return 1;
        case JS_L_BRACE:          printf("{\n"); break;
        case JS_R_BRACE:          printf("}\n"); break;
        case JS_L_SQUARE_BRACKET: printf("[\n"); break;
        case JS_R_SQUARE_BRACKET: printf("]\n"); break;
        case JS_COMMA:            printf(",\n"); break;
        case JS_COLON:            printf(":\n"); break;
        case JS_TRUE:             printf("true\n"); break;
        case JS_FALSE:            printf("false\n"); break;
        case JS_NULL:             printf("null\n"); break;
        case JS_STRING:           printf("string: \"%.*s\"\n", 
                                          lexeme_data.end - lexeme_data.begin, 
                                          lexeme_data.begin); break;
        case JS_NUMBER:           printf("number: %.*s\n",
                                          lexeme_data.end - lexeme_data.begin, 
                                          lexeme_data.begin); break;
        default: return 1;
        }
        json += lexeme_data.bytes_read;
    }
    return 0;
}
