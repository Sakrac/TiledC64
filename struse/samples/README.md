# STRUSE SAMPLES

These examples are intended to be read by humans and if desired copied directly from to help visualize the features and functionality of struse.h. These examples are intended to correctly deal with the format or purpose they were built for. If they don't let me know.

* [Basic](#basic)
* [Prehash](#prehash)
* [XML](#xml)
* [JSON](#json)
* [Diff](#diff)

### <a name="basic"></a>Basic sample

Files in project:

* samples/basic.cpp
* struse.h

The basic sample project is copied from struse.h test code (working on including better tests) that does a variety of simple operations on **strref** and **strown** types. The following areas are covered:

**strref samples**

strref is a const string pointer and a string length.

1. Extracting directory, filename and extension from a path
2. fnv1a hash from a string
3. Rolling hash find / basic string find
4. Wildcard matching examples

**strown samples**

strown is a self-contained string builder that mirrors most of strref

1. Insert and Remove
2. Uppercase / lowercase
3. Format strings with {} notation
4. built-in C style printf

### <a name="prehash"></a>Prehash sample

Files in project:

* samples/prehash.cpp
* struse.h

The intention of Prehash is to be a pre-build step for C++ files that references string hashes, but doesn't the strings. Prehash will find instances of PHASH("<keyword>") and replace it with PHASH("<keyword>", fnv1a(<keyword>)). If Prehash didn't do any changes it also wouldn't save the result to avoid unnecessary compiling.

There are two different implementations in prehash.cpp, one that highlights pairing *wildcard* searching with *exchange* to do the operation in a single buffer. This requires more shuffling than an implementation that separates string parsing and string output into two separate buffers so the second implementation does that.

**prehash\_search\_and\_replace**

1. Read in a file as a *strovl*
2. Wildcard search for **PHASH(\*{ \t}\"*@\"\*{!\n\r/})**
3. Within result, find quoted text and create a replacement *strown* version
4. Exchange the match with the replacement
5. Detect changes by comparing size and hash and potentially save file

**prehash\_dual\_buffer**

1. Read in a file as a *strref*, allocate an output buffer as a *strovl*
2. Wildcard search for **PHASH(\*{ \t}\"*@\"\*{!\n\r/})**
3. Append text before match to output and create a replacement *strown* for the match
4. Append replacement
5. Append remainder of text to output
6. Detect changes by comparing size and hash and potentially save file


### <a name="xml"></a>XML parser sample

Files in project:

* samples/xml.cpp
* samples/xml.h
* samples/xml_example.cpp
* struse.h

samples/xml.cpp is a general no-alloc XML parser using callbacks to tell the caller about the data. samples/xml_example.cpp is an implementation of such a callback and a string block representing an XML file to parse. Feel free to use xml.cpp directly as is if you need a small XML parser.

**ParseXML**

1. Iteratively search for XML tags (blocks surrounded by < and >).
2. Determine XML tag type (open, close, self-closing or text between two tags)
3. Keep an internal stack of hierarchic XML tags to detect errors (and advanced callback features)
4. Perform the callback with the tag or inbetween text.

**XMLDataCB** (this is the prototype for the user callback)

Parameters: user data, tag *strref*, stack *strref* array, stack depth, tag/text type

1. If inbetween text, handle based on application (tip: parent tag is the first element on the tag stack)
2. If open or self-closing tag, perform action and iterate over attributes if desired
3. If close or self-closing tag, handle that
4. If any error was detected, return false to stop further processing otherwise return true

**XMLFirstAttribute**

1. Returns the first attribute of a tag+attribute string (passed in to (XMLDataCB) as "*strref* tag")

**XMLNextAttribute**

1. Given a string of a number of attributes, move string forward to the next attibute and return that

**XMLAttributeName**

1. Given a string of a number of attributes, return the *strref* name of the first one

**XMLAttributeValue**

1. Given a string of a number of attributes, return the *strref* value without the quotes of the first one

**XMLFindAttr**

1. If only interested in a single attribute out of many this searches for a name and returns the *strref* value without quotes.

*xml_example.cpp*

Parses the following xml data in **XMLExample_Callback**:

    <?xml version="1.0"?>
    <root>
      <sprite type="billboard">
        <color red="32" blue='192' green="255"/>
        <bitmap>textures/yourface.dds</bitmap>
        <doublesided/>
        <size width='128' height='128'/>
      </sprite>
      <sprite type="static">
        <color red="32" green="64" blue='255'/>
        <bitmap>textures/splash.dds</bitmap>
        <size width='32' height='32'/>
      </sprite>
    </root>

### <a name="json"></a>JSON parser sample

Files in project:

* samples/json.cpp
* samples/json.h
* samples/json_example.cpp
* struse.h

samples/json.cpp is a general no-alloc JSON parser using callbacks to tell the caller about the data.

**ParseJSON**

Process JSON data using callbacks to build data. Call with a strref containing a JSON file, the callback function and an optional user data pointer.

**JSONDataCB**

This is the callback prototype.

* **stack**: An array where the first element is the current object or array. Each element has two fields, 'name' is the name of the object or empty if an array element, array_index is the index of this value in an array.
* **stack_size**: Number of levels of JSON objects/arrays
* **arg**: Current argument (valid for string, int, float. true/false/null have empty arg).
* **type**: Type of this JSON call
* **user_data**: Pointer passed in to the parser.

JSON Callback types:

	JSON_CB_OBJECT
	JSON_CB_ARRAY
	JSON_CB_STRING
	JSON_CB_INT
	JSON_CB_FLOAT
	JSON_CB_TRUE
	JSON_CB_FALSE
	JSON_CB_NULL
	JSON_CB_OBJECT_CLOSE
	JSON_CB_ARRAY_CLOSE

See [json.org](http://www.json.org/) for format details.


### <a name="diff"></a>Diff / Patch text file sample

Files in project:

* samples/diff.cpp
* struse.h

Diff is a naive implementation of a text file compare and patch. It will export a file that looks like a visual diff between two files which can be used to re-create the updated file from the original and the patch output.

The implementation is simplistic and not a replacement for a true diff, it is a fun little sample using a number of features from struse.h.
 
