#ifndef JSON_H
#define JSON_H

#include <stdlib.h>
#include <stdbool.h>

/*!
 * \brief Initializes library resources.
 *
 * \return
 * - True in case of success;
 * - False in case of failure.
 */
bool json_init(void);

/*!
 * \brief Release library resources.
 */
void json_exit(void);

struct jsonValue *json_create_number(double number);

struct jsonValue *json_create_string(const char *string);

struct jsonValue *json_create_object(void);

struct jsonValue *json_create_array(void);

struct jsonValue *json_create_boolean(int boolean);

struct jsonValue *json_create_null(void);

bool json_object_add(struct jsonValue *object, const char *key, struct jsonValue *value);

struct jsonValue *json_object_lookup(struct jsonValue *object, const char *key);

struct jsonValue *json_object_lookup_next(struct jsonValue *object, const char *key, struct jsonValue *value);

bool json_array_append(struct jsonValue *array, struct jsonValue *value);

size_t json_array_size(struct jsonValue *array);

struct jsonValue *json_array_at(struct jsonValue *array, size_t index);

void json_value_free(struct jsonValue *value);

struct jsonValue *json_parse(const char *json);

/*! \brief Prints json value in a pretty way.
 *
 * \details Acts like snprint i.e. passing size = 0 allows to precalculate out
 * buffer size. Then you may allocate buffer of required size and pass to the
 * function again.
 *
 * If size != 0 output guaranteed have trailing '\0'.
 *
 * \param out Output buffer or NULL.
 * \param size Size of out buffer. The function doesn't write more than that.
 * \param value Json value to print.
 * \returns Number of bytes (excluding terminating '\0') that would be written
 * if output buffer was of infinite size.
 */
size_t json_pretty_print(char *out, size_t size, struct jsonValue *value);

const char *json_strerror(void);

#endif
