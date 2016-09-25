// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

enum jsonLexemeKind { 
    JS_ERROR,
    JS_L_BRACE, 
    JS_R_BRACE, 
    JS_L_SQUARE_BRACKET, 
    JS_R_SQUARE_BRACKET, 
    JS_COMMA, 
    JS_COLON, 
    JS_TRUE, 
    JS_FALSE, 
    JS_NULL,
    JS_STRING,
    JS_NUMBER, 
};

struct jsonLexemeData {
    size_t bytes_read;      //  - set for all
    const char * begin;     //  - set for JS_STRING and JS_NUMBER
    const char * end;       // for JS_STRING 
                            // begin points to first char inside quotes
                            // end points to ending quote
    size_t measured_length; //  - set for string
};

enum jsonLexemeKind json_next_lexeme(const char * json, struct jsonLexemeData * data);

size_t json_get_string(const char * json, char * out);
