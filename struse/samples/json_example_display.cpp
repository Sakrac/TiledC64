//
// SAMPLE USE OF STRREF JSON CALLBACK PARSER
//

#define _CRT_SECURE_NO_WARNINGS
#define STRUSE_IMPLEMENTATION
#include "struse.h"
#include "json.h"

const char json_tests[] = {
	"{"
	" \"numbers\" : [ 0, 1.25, 2, 3, 4 ]"
	" \"object\" : {"
	"  \"array_of_arrays\" : [ [ \"abc\", true, false, null, 1, 1.5 ], [ { \"name\" : \"N/A\" } ] ]"
	" }"
	"}"
};

static const char _spacing[] = "                                                                ";

bool json_test_print(const json_stack_item *stack, int stack_size, strref arg, JSON_NODE type, void *user)
{
	bool array = stack[0].array_index >= 0;
	strref name = stack->name;
	switch (type) {
		case JSON_CB_OBJECT:
			if (array)
				printf("%.*s{ // array index %d\n", stack_size, _spacing, stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = {\n", stack_size, _spacing, STRREF_ARG(name));
			break;

		case JSON_CB_ARRAY:
			if (array)
				printf("%.*s[ // array index %d\n", stack_size, _spacing, stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = [\n", stack_size, _spacing, STRREF_ARG(name));
			break;

		case JSON_CB_OBJECT_CLOSE:
			printf("%.*s}\n", stack_size, _spacing);
			break;

		case JSON_CB_ARRAY_CLOSE:
			printf("%.*s]\n", stack_size, _spacing);
			break;

		case JSON_CB_STRING:
			if (array)
				printf("%.*s\"" STRREF_FMT "\" // array index %d\n", stack_size, _spacing, STRREF_ARG(arg), stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = \"" STRREF_FMT "\"\n", stack_size, _spacing, STRREF_ARG(name), STRREF_ARG(arg));
			break;


		case JSON_CB_TRUE:
			if (array)
				printf("%.*strue // array index %d\n", stack_size, _spacing, stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = true\n", stack_size, _spacing, STRREF_ARG(name));
			break;
		case JSON_CB_FALSE:
			if (array)
				printf("%.*sfalse // array index %d\n", stack_size, _spacing, stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = false\n", stack_size, _spacing, STRREF_ARG(name));
			break;
		case JSON_CB_FLOAT:
		case JSON_CB_INT:
			if (array)
				printf("%.*s" STRREF_FMT " // array index %d\n", stack_size, _spacing, STRREF_ARG(arg), stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = " STRREF_FMT "\n", stack_size, _spacing, STRREF_ARG(name), STRREF_ARG(arg));
			break;

		case JSON_CB_NULL:
			if (array)
				printf("%.*snull // array index %d\n", stack_size, _spacing, stack->array_index);
			else
				printf("%.*s\"" STRREF_FMT "\" = null\n", stack_size, _spacing, STRREF_ARG(name));
			break;


		default:
			return false;
	}

	return true;
}

int main(int argc, char **argv) {
	if (argc > 1) {
		if (FILE *f = fopen(argv[1], "rb")) {
			fseek(f, 0, SEEK_END);
			size_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			if (char *buffer = (char*)malloc(size)) {
				fread(buffer, size, 1, f);
				fclose(f);
				ParseJSON(strref(buffer, size), json_test_print, nullptr);
				free(buffer);
			} else
				fclose(f);
		}
	} else {
		if (!ParseJSON(strref(json_tests, sizeof(json_tests)), json_test_print, nullptr))
			return 1;
	}

	return 0;
}
