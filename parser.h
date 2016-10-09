/*
 * author: Nolan <sullen.goose@gmail.com>
 * Copy if you can.
 */

struct jsonString;
struct jsonObject;
struct jsonArray;
struct jsonValue;
struct jsonDocument;

enum jsonValueKind { JVK_STR, JVK_NUM, JVK_OBJ, JVK_ARR, JVK_BOOL, JVK_NULL, JVK_ERROR };

/* String */
size_t json_string_length(struct jsonString * string);
const char * json_string_data(struct jsonString * string);

/* Object */
size_t json_object_size(struct jsonObject * object);
struct jsonValue * json_object_at(struct jsonObject * object, const char * key);
void json_object_for_each(struct jsonObject * object,
        void (*action)(const char *, struct jsonValue *, void *), void * user_data);

/* Array */
size_t json_array_size(struct jsonArray * array);
void json_array_for_each(struct jsonArray * array,
        void (*action)(size_t, struct jsonValue *, void *), void * user_data);

/* Value */
enum jsonValueKind json_value_kind(struct jsonValue * value);
struct jsonString * json_value_string(struct jsonValue * value);
double json_value_number(struct jsonValue * value);
struct jsonObject * json_value_object(struct jsonValue * value);
struct jsonArray * json_value_array(struct jsonValue * value);
int json_value_bool(struct jsonValue * value);

/* Document */
struct jsonDocument * json_parse(const char * json);
struct jsonValue * json_document_value(struct jsonDocument * document);
void json_document_free(struct jsonDocument * document);

