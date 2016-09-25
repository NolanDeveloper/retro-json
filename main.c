// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int create_object() { 
    static int n = 0;
    printf("object: %d\n", n);
    return n++;
}

int create_array() { 
    static int n = 0;
    printf("array: %d\n", n);
    return n++;
}

int create_field(int object, char * name) { 
    static int n = 0;
    printf("field(%d, %s): %d\n", object, name, n);
    return n++;
}

char * alloc_string(size_t length) {
    return malloc(length);
}

void set_field_object(int field, int object) { 
    printf("set_field_object(%d, %d)\n", field, object);
}

void set_field_array(int field, int array) {
    printf("set_field_array(%d, %d)\n", field, array);
}

void set_field_string(int field, char * string) {
    printf("set_field_string(%d, %s)\n", field, string);
}

void set_field_number(int field, double value) {
    printf("set_field_number(%d, %lf)\n", field, value);
}

void set_field_bool(int field, int value) {
    printf("set_field_bool(%d, %s)\n", field, value ? "true" : "false");
}

void set_field_null(int field) { 
    printf("set_field_null(%d)\n", field);
}

void array_add_object(int array, int object) {
    printf("array_add_object(%d, %d)\n", array, object);
}

void array_add_array(int array_container, int array_value) {
    printf("array_add_array(%d, %d)\n", array_container, array_value);
}

void array_add_string(int array, char * string) {
    printf("array_add_string(%d, %s)\n", array, string);
}

void array_add_number(int array, double value) {
    printf("array_add_number(%d, %lf)\n", array, value);
}

void array_add_bool(int array, int value) {
    printf("array_add_bool(%d, %s)\n", array, value ? "true" : "false");
}

void array_add_null(int array) {
    printf("array_add_null(%d)\n", array);
}

int main() {
    json_set_callbacks(create_object, create_array, create_field, alloc_string,
        set_field_object, set_field_array, set_field_string, set_field_number, 
        set_field_bool, set_field_null, array_add_object, array_add_array, 
        array_add_string, array_add_number, array_add_bool, array_add_null);
    const char * json = "{ \"\\u042F \\u043C\\u043E\\u0436\\u0443 \\u0457\\u0441\\u0442\" : -42E-5 }";
    if (!json_parse(json)) return 1;
    return 0;
}
