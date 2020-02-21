//
//  prehash.cpp
//  
//
//  Created by Carl-Henrik Skårstedt on 9/14/15.
//
//
//  Parse a file for instances of strings to hash (prefixed by
//  PHASH).

#define _CRT_SECURE_NO_WARNINGS
#define STRUSE_IMPLEMENTATION
#include "struse.h"

#define PHASH(str, hash) (hash)

#define PHASH_SANDWICH PHASH("Sandwich");
#define PHASH_SALAD PHASH("Salad");

// maximum length (keyword must be same line)
#define PHASH_MAX_LENGTH 1024

// maximum additional margin
#define PHASH_MAX_MARGIN 128*1024

#define PHASH_EXTEND_CHARACTERS 12

// trivial implementation to highlight how to use
// wildcard search combined with exchange.
bool prehash_search_and_replace(const char *file) {
    if (FILE *f = fopen(file, "rb")) {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        size_t buffer_size = size + PHASH_MAX_MARGIN;
        if (void *buffer = malloc(buffer_size)) {
            fread(buffer, size, 1, f);
            fclose(f);
            
            // create an overlay string to start scanning
            strovl overlay((char*)buffer, (strl_t)buffer_size, (strl_t)size);
            
            // calculate hash of entire file
            unsigned int original_hash = overlay.fnv1a();
            
            // find all instances of PHASH token
            // extra quotes inserted to allow running tool over this file
            strref pattern("P""HASH(*{ \t}\"*@\"*{!\n\r/})");
            strref match;
            strown<PHASH_MAX_LENGTH> replace;
            while ((match = overlay.wildcard_after(pattern, match))) {
                // get the keyword to hash
                strref keyword = match.between('"', '"');
                
                // generate a replacement to the original string
                replace.sprintf("PHASH(\"" STRREF_FMT "\", 0x%08x)", STRREF_ARG(keyword), keyword.fnv1a());
                
                // exchange the match with the evaluated string
                overlay.exchange(match, replace.get_strref());
                
                // if about to run out of space, early out.
                if (overlay.len()>(overlay.cap()-PHASH_EXTEND_CHARACTERS))
                    break;
            }
            
            // check if there was a change to the data
            if (size != overlay.len() || original_hash != overlay.fnv1a()) {
                // write back if possible
                if ((f = fopen(file, "wb"))) {
                    fwrite(overlay.get(), overlay.len(), 1, f);
                    fclose(f);
				} else {
					free(buffer);
					return false;
				}
            }
			free(buffer);
            return true;
        } else
            fclose(f);
    }
    return false;
}

// prehash with a faster approach of scanning from one memory
// location and appending to another
bool prehash_dual_buffer(const char *file) {
	if (FILE *f = fopen(file, "rb")) {
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *original = malloc(size)) {
			fread(original, size, 1, f);
			fclose(f);

			strref origref((const char*)original, (strl_t)size);

			size_t buffer_size = size + PHASH_MAX_MARGIN;
			if (void *buffer = malloc(buffer_size)) {
				// create an overlay string as an output buffer
				strovl overlay((char*)buffer, (strl_t)buffer_size);

				// find all instances of PHASH token
				// extra quotes inserted to allow running tool over this file
				strref pattern("P""HASH(*{ \t}\"*@\"*{!\n\r/})");
				strl_t prevPos = 0;
				strref match;
				strown<PHASH_MAX_LENGTH> replace;
				while ((match = origref.wildcard_after(pattern, match))) {
					// copy the data from the original up to the point of the match
					overlay.append(origref.get_substr(prevPos, origref.substr_offs(match)-prevPos));

					// update the previous position to right after the match
					prevPos = origref.substr_end_offs(match);

					// get the keyword to hash
					strref keyword = match.between('"', '"');

					// generate a replacement to the original string
					replace.sprintf("PHASH(\"" STRREF_FMT "\", 0x%08x)", STRREF_ARG(keyword), keyword.fnv1a());

					// append the replacement
					overlay.append(replace.get_strref());
				}
				overlay.append(origref.get_substr(prevPos, origref.get_len()-prevPos));

				// check if there was a change to the data
				if (origref.get_len() != overlay.get_len() || origref.fnv1a() != overlay.fnv1a()) {
					// done with the original file
					free(original);

					// write back if possible
					if ((f = fopen(file, "wb"))) {
						fwrite(overlay.get(), overlay.len(), 1, f);
						fclose(f);
					} else {
						free(buffer);
						return false;
					}
				}
				free(buffer);
				return true;
			}
		} else
			fclose(f);
	}
	return false;
}

int main(int argc, char **argv) {
    const char *file = "../prehash.cpp";
    if (argc>1)
        file = argv[1];

	if (!prehash_search_and_replace(file)) {
        printf("Failed to prehash \"%s\"\n", file);
        return 1;
    }
    return 0;
}
