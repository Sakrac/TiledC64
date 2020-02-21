#include "struse.h"
#include "json.h"

#define JSON_STACK_SIZE 128		// max hierarchical depth of a json file

#define PHASH(keyword, hash) (hash)
#define JSON_KEYWORD_NULL PHASH("null", 0x77074ba4)
#define JSON_KEYWORD_TRUE PHASH("true", 0x4db211e5)
#define JSON_KEYWORD_FALSE PHASH("false", 0x0b069958)

bool ParseJSON(strref json, JSONDataCB callback, void *user_data)
{
	json_stack_item stack[JSON_STACK_SIZE];
	bool assign = false;
	int sp = JSON_STACK_SIZE-1;
	stack[sp].name.clear();
	stack[sp].array_index = -1;
	json = json.after('{');	// skip first open bracket since that simply defines the scope
	json.skip_whitespace();
	while (json) {
		char c = json.get_first();
		if ((c=='}' || c==']') && sp==(JSON_STACK_SIZE-1))
			break;	// closed the JSON block, exit
		switch (c) {
			case '{': // curly brackets begin objects
				callback(stack + sp, JSON_STACK_SIZE - sp, strref(), JSON_CB_OBJECT, user_data);
				assign = false;
				--sp;
				stack[sp].name.clear();
				stack[sp].array_index = -1;
				++json;
				break;
			case '[': // straight brackets begin arrays
				callback(stack + sp, JSON_STACK_SIZE - sp, strref(), JSON_CB_ARRAY, user_data);
				--sp;
				stack[sp].name.clear();
				stack[sp].array_index = 0;
				assign = true;
				++json;
				break;
			case '}': // just handle minor mistakes
			case ']': // close object or array
				if (sp<JSON_STACK_SIZE && stack[sp].array_index >= 0) {	// this was an array
					sp++;
					callback(stack + sp, JSON_STACK_SIZE - sp, strref(), JSON_CB_ARRAY_CLOSE, user_data);
				} else {	// this was an object
					if (sp<(JSON_STACK_SIZE-1)) {
						sp++;
						callback(stack + sp, JSON_STACK_SIZE - sp, strref(), JSON_CB_OBJECT_CLOSE, user_data);
					}
				}
				if (sp<JSON_STACK_SIZE && stack[sp].array_index >= 0)
					stack[sp].array_index++;
				assign = false;
				++json;
				break;
			case ':': // colon indicates an assignment
				assign = true;
				++json;
				break;
			case ',': // JSON requires a comma separator but it doesn't matter to parsing
				++json;
				break;
			default: { // anything else is either a name or a value
				strref tag;
				bool quoted = false;
				bool is_null = false;
				if (c=='"') {
					tag = json.between('"', '"');
					quoted = true;
				} else {
					tag = json.get_valid_json_string();
					is_null = tag.fnv1a() == JSON_KEYWORD_NULL;
				}

				if (!assign && !is_null && stack[sp].array_index<0) {	// array and null values are not objects
					stack[sp].name = tag;
					stack[sp].array_index = -1; // a '[' will change this to an array type if detected after ':'
					json += tag.get_len() + (quoted ? 2:0);
					assign = true;
				} else {
					JSON_NODE type = JSON_CB_STRING;
					if (is_null)
						type = JSON_CB_NULL;
					else if (!quoted && tag.fnv1a() == JSON_KEYWORD_TRUE)
						type = JSON_CB_TRUE;
					else if (!quoted && tag.fnv1a() == JSON_KEYWORD_FALSE)
						type = JSON_CB_FALSE;
					else if (!quoted && tag.is_number())
						type = JSON_CB_INT;
					else if (!quoted && tag.is_float_number())
						type = JSON_CB_FLOAT;
					int skip = (int)(tag.get_len() + (quoted ? 2 : 0));
					if (!skip)
						return false;
					if (type==JSON_CB_NULL || type==JSON_CB_TRUE || type==JSON_CB_FALSE)
						tag.clear();
					callback(stack + sp, JSON_STACK_SIZE - sp, tag, type, user_data);
					json += skip;
					if (stack[sp].array_index>=0)
						stack[sp].array_index++;
					assign = false;
				}
				break;
			}
		}
		json.skip_whitespace();
	}
	return true;
}
