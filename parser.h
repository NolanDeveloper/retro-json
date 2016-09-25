// author: Nolan <sullen.goose@gmail.com>
// Copy if you can.

typedef int    (*json_create_object_f)();
typedef int    (*json_create_array_f)();
typedef int    (*json_create_field_f)(int object, char * name);
typedef char * (*json_alloc_string_f)(size_t length);
typedef void   (*json_set_field_object_f)(int field, int object);
typedef void   (*json_set_field_array_f)(int field, int array);
typedef void   (*json_set_field_string_f)(int field, char * string);
typedef void   (*json_set_field_number_f)(int field, double value);
typedef void   (*json_set_field_bool_f)(int field, int value);
typedef void   (*json_set_field_null_f)(int field);
typedef void   (*json_array_add_object_f)(int array, int object);
typedef void   (*json_array_add_array_f)(int array_container, int array_value);
typedef void   (*json_array_add_string_f)(int array, char * string);
typedef void   (*json_array_add_number_f)(int array, double value);
typedef void   (*json_array_add_bool_f)(int array, int value);
typedef void   (*json_array_add_null_f)(int array);

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
    json_array_add_null_f   json_array_add_null);

size_t json_parse(const char * json);
