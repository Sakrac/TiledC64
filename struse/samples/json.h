#ifndef __JSON_H__
#define __JSON_H__

// Reasonably rapid parsing of JSON through callbacks
enum JSON_NODE {
	JSON_CB_OBJECT,
	JSON_CB_ARRAY,
	JSON_CB_STRING,
	JSON_CB_INT,
	JSON_CB_FLOAT,
	JSON_CB_TRUE,
	JSON_CB_FALSE,
	JSON_CB_NULL,
	JSON_CB_OBJECT_CLOSE,
	JSON_CB_ARRAY_CLOSE,
};

typedef struct {
	strref name;
	int array_index;	// objects: -1, arrays: index of current array starting at 0
} json_stack_item;

typedef bool(*JSONDataCB)(const json_stack_item* /*stack*/, int /*stack_size*/, strref /*arg*/, JSON_NODE /*type*/, void* /*user_data*/);

bool ParseJSON(strref json, JSONDataCB callback, void *user);

#endif