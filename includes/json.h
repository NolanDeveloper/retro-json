#ifndef JSON_H
#define JSON_H

#include <stdlib.h>
#include <stdbool.h>

/*! \file json.h 
 * The file contains public api of retro-json library.
 */

/*!
 * Opaque structure that generalizes values of all possible kinds of json.
 */
struct jsonValue;

/*!
 * \brief Release memory held by the value.
 * \param value What to free.
 */
void json_value_free(struct jsonValue *value);

/*!
 * \brief Initializes library resources.
 * \return Success or not.
 */
bool json_init(void);

/*!
 * \brief Release library resources.
 */
void json_exit(void);

/*!
 * \brief Types of json values.
 */
enum jsonValueKind {
    JVK_STR, //!< string
    JVK_NUM, //!< number (we don't discriminate between integers and floats)
    JVK_OBJ, //!< object
    JVK_ARR, //!< array
    JVK_BOOL, //!< "true" or "false"
    JVK_NULL //!< "null"
};

/*! \name Constructors
 * \{ */

/*!
 * \brief Construct json node of type JVK_NUM.
 * \param number Value to put into that node.
 * \return Created node.
 */
struct jsonValue *json_create_number(double number);

/*!
 * \brief Construct json node of type JVK_STR.
 * \param string Value to put into that node.
 * \return Created node.
 */
struct jsonValue *json_create_string(const char *string);

/*!
 * \brief Construct json node of type JVK_OBJ.
 * \return Created node.
 */
struct jsonValue *json_create_object(void);

/*!
 * \brief Construct json node of type JVK_ARR.
 * \return Created node.
 */
struct jsonValue *json_create_array(void);

/*!
 * \brief Construct json node of type JVK_BOOL.
 * \param boolean Value to put into that node.
 * \return Created node.
 */
struct jsonValue *json_create_boolean(bool boolean);

/*!
 * \brief Construct json node of type JVK_NULL.
 * \return Created node.
 */
struct jsonValue *json_create_null(void);

/*! \} */

/*! \name Object functions
 *
 * Object is a multimap. Under the hood it's hashmap with open addressing and
 * linear probing. So expect most operations to work in O(1). However
 * performance may be not as good when there are too many values added to the
 * same key.
 *
 * \{ */

/*!
 * \brief Add field to the object.
 * \details Object is a multimap so you can add multiple values at the same key.
 * \param object Where to add.
 * \param key Name of the field.
 * \param value New value of the field.
 * \return Success or not.
 */
bool json_object_add(struct jsonValue *object, const char *key, struct jsonValue *value);

/*!
 * \brief Find the value of the field.
 * \details The function will only find the last value added to the \p key.
 * If you need other values too use json_object_lookup_next().
 * \param object Where to search.
 * \param key Name of the field.
 * \return
 * - The last added value of the field with name \p key;
 * - NULL, if there's no field with the name \p key.
 */
struct jsonValue *json_object_lookup(struct jsonValue *object, const char *key);

/*!
 * \brief Find previous values of the field.
 * \details The object is multimap. In other words it can have multiple
 * values added to the same key. Passing NULL to the \p value allows
 * to get the last value added to the \p key. Passing this value
 * to value again allows to get value that was added previously. Continue
 * doing this to get all the values at the \p key.
 * \param object Where to search.
 * \param key Name of the field.
 * \param value
 * - NULL if you only want the last value;
 * - previous value of the \p key you are interested in.
 * \return
 * - NULL if there was no value found;
 * - the value of the key that was added before \p value.
 */
struct jsonValue *json_object_lookup_next(struct jsonValue *object, const char *key, struct jsonValue *value);

/*! \}
 *
 * \name Array
 *
 * Array is implemented as a growing block of memory.
 *
 * \{
 */

/*!
 * \brief Add \p value to the end of array.
 * \param array Where to add.
 * \param value What value to add.
 * \return Success or not.
 */
bool json_array_append(struct jsonValue *array, struct jsonValue *value);

/*!
 * \brief Get number of elements in \p array.
 * \param array Json array.
 * \return Number of elements in \p array.
 */
size_t json_array_size(struct jsonValue *array);

/*!
 * \brief Get element of array at specific index.
 * \param array Json array.
 * \param index Zero based index of the wanted element.
 * \return Element of array or NULL if index is out of range.
 */
struct jsonValue *json_array_at(struct jsonValue *array, size_t index);

/*! \} */

/*!
 * \brief Parse json from string.
 * \param json UTF-8 encoded null terminated string.
 * \param all Is it required to parse whole string or unparsed trailing bytes
 * are allowed.
 * \return
 * - parsed value;
 * - NULL, if something went wrong.
 */
struct jsonValue *json_parse(const char *json, bool all);

/*! \brief Prints json value in a pretty way.
 * \details Acts like snprint i.e. passing size = 0 allows to precalculate out
 * buffer size. Then you may allocate buffer of required size and pass to the
 * function again.
 * If size != 0 output guaranteed have trailing '\0'.
 * \param out Output buffer or NULL.
 * \param size Size of out buffer. The function doesn't write more than that.
 * \param value Json value to print.
 * \returns Number of bytes (excluding terminating '\0') that would be written
 * if output buffer was of infinite size.
 */
size_t json_pretty_print(char *out, size_t size, struct jsonValue *value);

/*!
 * \brief String representation of an error.
 * \return Text describing error happend during the last call to a library
 * function in THIS THREAD or string "NULL" if the last call was successful or
 * there were no cals at all. Never returns NULL.
 */
const char *json_strerror(void);

#endif
