//
//  diff.cpp
//  struse / samples
//
//  Created by Carl-Henrik Skårstedt on 9/19/15.
//  Copyright © 2015 Carl-Henrik Skårstedt. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS
#define STRUSE_IMPLEMENTATION
#include "struse.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
	unsigned int hash;
	unsigned int line;
} HashLookup;

typedef struct {
	int matches;
	int lineA;
	int lineB;
} LineMatches;


// allocate and read a file by name
unsigned char* ReadFile(const char *file, size_t &size) {
    if (FILE *f = fopen(file, "rb")) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (unsigned char *buffer = (unsigned char*)malloc(size)) {
			fread(buffer, size, 1, f);
			fclose(f);
			return buffer;
		}
		fclose(f);
	}
	return nullptr;
}

// get an ignore-whitespace hash for each line of a file
unsigned int* LineByLineHash(strref file, strref **aLines, unsigned int &numLines) {
    int lines = file.count_lines();
    if (lines) {
        unsigned int *hashes = new unsigned int[lines];
		strref *apLines = new strref[lines];
		for (int curr=0; curr<lines; curr++) {
			strref line = file.next_line(); // return line by line even if empty
			apLines[curr] = line;
            hashes[curr] = line.fnv1a();
		}
		*aLines = apLines;
        numLines = lines;
        return hashes;
    }
	*aLines = nullptr;
    numLines = 0;
    return nullptr;
}

int sortHashLookup(const void *A, const void *B) {
	const HashLookup *_A = (const HashLookup*)A;
	const HashLookup *_B = (const HashLookup*)B;
	return _A->hash > _B->hash ? 1 : (_A->hash==_B->hash ? 0 : -1);
}

int sortLineMatches(const void *A, const void *B) {
	const LineMatches *_A = (const LineMatches*)A;
	const LineMatches *_B = (const LineMatches*)B;
	return _A->matches < _B->matches ? 1 : (_A->matches==_B->matches ? 0 : -1);
}

int sortLineNumbers(const void *A, const void *B) {
	const LineMatches *_A = (const LineMatches*)A;
	const LineMatches *_B = (const LineMatches*)B;
	return _A->lineB > _B->lineB ? 1 : -1;
}

// multi-hit binary search
int LookupHashLookup(unsigned int value, HashLookup *lookup, int count)
{
	int first = 0;
    while (count!=first) {
        int index = (first+count)/2;
        unsigned int read = lookup[index].hash;
		if (value==read) {
			while (index && lookup[index-1].hash==value)
				index--;
            return index;
		} else if (value>read)
            first = index+1;
        else
            count = index;
    }
    return -1;	// index not found
}

void LineByLineBestMatch(LineMatches *matchBA, unsigned int numLinesA, unsigned int numLinesB,
						 unsigned int *hashesA, unsigned int *hashesB, strref *aLinesA, strref *aLinesB)
{
	// step 1: make a sorted lookup for A for easier lookup from B
	//		   (binary search)
	HashLookup *lookupA = new HashLookup[numLinesA];
	for (unsigned int l = 0; l<numLinesA; l++) {
		lookupA[l].hash = hashesA[l];
		lookupA[l].line = l;
	}
	qsort(lookupA, numLinesA, sizeof(HashLookup), sortHashLookup);

	// step 2: find sequential matches going in B that exists in A
	for (unsigned int lB = 0; lB<numLinesB; lB++) {
		unsigned int hash = hashesB[lB];
		int lAI = LookupHashLookup(hash, lookupA, numLinesA);
		if (lAI >= 0) {
			int bestLineMatch = -1;
			int numListMatch = 0;
			while ((unsigned int)lAI<numLinesA && lookupA[lAI].hash == hash) {
				unsigned int lA = lookupA[lAI].line;
				int count = 0;
				for (unsigned int lBC = lB; lBC<numLinesB && lA<numLinesA; lBC++) {
					if (hashesA[lA] != hashesB[lBC] || !aLinesA[lA].same_str_case(aLinesB[lBC]))
						break;
					count++;
					lA++;
				}
				if (count > numListMatch) {
					bestLineMatch = lookupA[lAI].line;
					numListMatch = count;
				}
				lAI++;
			}
			// empty single lines are more efficient as replacements
			matchBA[lB].lineA = (numListMatch==1 && aLinesB[lB].is_empty()) ?
				-1 : bestLineMatch;
			matchBA[lB].lineB = lB;
			matchBA[lB].matches = numListMatch;
		} else {
			// line B does not exist in A
			matchBA[lB].lineA = -1;
			matchBA[lB].lineB = lB;
			matchBA[lB].matches = 1;
		}
	}
}

void FindMatchingLines(LineMatches *matchBA, strref textA, unsigned int numLinesB, unsigned int *hashesB, strref *aLinesB)
{
	unsigned int numHashA;
	strref *aLinesA;

	// step 1: convert original text to lines and hashes
	unsigned int *hashesA = LineByLineHash(textA, &aLinesA, numHashA);

	// Find best matches for each line of B in A (number of sequential matching lines)
	LineByLineBestMatch(matchBA, numHashA, numLinesB, hashesA, hashesB, aLinesA, aLinesB);

	// Done with lines of Text A
	delete[] aLinesA;
	delete[] hashesA;
}

int FilterDuplicates(int numChanges, LineMatches *matchBA)
{
	for (int l = 0; l<numChanges; l++) {
		if (matchBA[l].lineA<0)
			continue;			// not a match, don't compare
		// sort to pull the largest to the top (this should be fairly unchanged after the first filters)
		qsort(matchBA+l, numChanges-l, sizeof(LineMatches), sortLineMatches);

		// find duplicate lines...
		int first = matchBA[l].lineB;
		int last = first + matchBA[l].matches;

		int k = l+1;
		while (k<numChanges) {
			// set of lines entirely within current set?
			int k1 = matchBA[k].lineB;
			int ke = k1 + matchBA[k].matches;
			if (k1>=first && ke<=last) {
				memmove(&matchBA[k], &matchBA[k+1], sizeof(LineMatches) * (numChanges-k-1));
				numChanges--;
			} else {
				if (k1 <= first && ke > first) {
					matchBA[k].matches = first - k1;
				} else if (k1>=first && k1<last) {
					int remove = last-k1;
					matchBA[k].lineB += remove;
					matchBA[k].lineA += remove;
					matchBA[k].matches -= remove;
				}
				if (matchBA[k].lineB == 61) {
					printf("yep!\n");
				}
				k++;
			}
		}
	}
	return numChanges;
}

// For sequences of added lines collapse them into repeats
int MergeAddedSequentialLines(int numChanges, LineMatches *matchBA)
{
	for (int l = 0; l<numChanges; l++) {
		if (matchBA[l].lineA<0) {
			int count = 0;
			for (int l2 = l; l2<numChanges && matchBA[l2].lineA<0; l2++)
				count++;
			matchBA[l].matches = count;
			if (count>1)
				memmove(&matchBA[l+1], &matchBA[l+count], sizeof(LineMatches) * (numChanges-l-count));
			numChanges -= count-1;
		}
	}
	return numChanges;
}

// hash, independent of line endings
unsigned int CustomFileHash(strref text) {
	unsigned int hash = strref().fnv1a();
	strref le("\n");
	while (text) {
		hash = text.next_line().fnv1a(hash);
		hash = le.fnv1a(hash);
	}
	return hash;
}

// find a minimal set of section copies and new added lines to reconstruct B from A
strovl CompareLines(strref textA, strref textB)
{
	// step 1: convert new text (B) to lines and hashes
	unsigned int numLinesB;
	strref *aLinesB;
	unsigned int *hashesB = LineByLineHash(textB, &aLinesB, numLinesB);

	// step 2: find matching sequences in the original text (A)
	LineMatches *matchBA = new LineMatches[numLinesB];
	FindMatchingLines(matchBA, textA, numLinesB, hashesB, aLinesB);
	
	// step 3: keep the best matches, filter out duplicates
	int numChanges = FilterDuplicates(numLinesB, matchBA);
	
	// step 4: sort matches in order of occurrence to retrieve patch order
	qsort(matchBA, numChanges, sizeof(LineMatches), sortLineNumbers);

	// step 5: merge sequential added lines
	numChanges = MergeAddedSequentialLines(numChanges, matchBA);
	assert(numChanges <= (int)numLinesB);

	// step 6: estimate file size of output data
	size_t approx_size = 10;	// include room for original hash
	for (int l = 0; l<numChanges; l++) {
		if (matchBA[l].lineA < 0) {
			approx_size += 16;
			for (int n = 0; n<matchBA[l].matches; n++)
				approx_size += aLinesB[matchBA[l].lineB+n].get_len() + 2;
		} else
			approx_size += 32;
	}

	// step 7: generate the patch output
	strovl out;
	char *buf = (char*)malloc(approx_size);
	if (buf != nullptr) {
		out = strovl(buf, strl_t(approx_size));

		out.sprintf("%08x\n", CustomFileHash(textA));
		for (int l=0; l<numChanges; l++) {
			if (matchBA[l].lineA < 0) {
				out.sprintf_append("+%d\n", matchBA[l].matches);
				for (int n = 0; n<matchBA[l].matches; n++) {
					out.append(aLinesB[matchBA[l].lineB + n]);
					out.append('\n');
				}
			} else
				out.sprintf_append("<%d,%d\n", matchBA[l].lineA, matchBA[l].matches);
		}
	}
	
	// Done with lines of Text B
	delete[] aLinesB;
	delete[] hashesB;

	return out;
}

size_t PatchFileSize(strref *aLines, int orig_lines, strref patch, bool &error)
{
	// estimate patch size
	size_t size = 0;
	error = false;
	while (patch) {
		strref line = patch.next_line();
		if (line.get_first()=='+') {
			++line;
			int64_t num_lines = line.atoi();
			for (int l = 0; l<num_lines; l++)
				size += patch.next_line().get_len() + 2;
		} else if (line.get_first()=='<') {
			++line;
			int first = line.atoi_skip();
			if (line.get_first()!=',') {
				error = true;
				break;
			}
			++line;
			int64_t num_lines = line.atoi();
			if ((first+num_lines) > orig_lines) {
				error = true;
				break;
			}
			for (int l = 0; l<num_lines; l++)
				size += aLines[l+first].get_len() + 2;
		} else {
			error = true;
			break;
		}
	}
	return size;
}

strovl PatchFile(strref orig, strref patch)
{
	// check that the hash matches
	uint64_t hash = patch.next_line().ahextoui();
	if (hash != CustomFileHash(orig))
		return strovl();

	int orig_lines = orig.count_lines();
	strref *aLines = new strref[orig_lines];
	if (!aLines)
		return strovl();

	int n = 0;
	while (orig && n<orig_lines)
		aLines[n++] = orig.next_line();
	

	// estimate patch size
	bool error = false;
	size_t size = PatchFileSize(aLines, orig_lines, patch, error);

	strovl out;
	if (!error && size) {
		// if size eval was successful, get an output buffer and apply patch
		char *buf = (char*)malloc(size);
		out = strovl(buf, strl_t(size));
		while (patch) {
			strref line = patch.next_line();
			if (line.get_first()=='+') {
				++line;
				int64_t num_lines = line.atoi();
				for (int l = 0; l<num_lines; l++) {
					out.append(patch.next_line());
					out.append('\n');
				}
			} else if (line.get_first()=='<') {
				++line;
				int first = line.atoi_skip();
				++line;
				int64_t num_lines = line.atoi();
				for (int l = 0; l<num_lines; l++) {
					out.append(aLines[l+first]);
					out.append('\n');
				}
			}
		}
	}
	delete[] aLines;

	return out;
}

// Diff two files and return a patch
strovl DiffFiles(const char *fileA, const char *fileB) {
    size_t sizeA, sizeB;
    unsigned char *bufA = ReadFile(fileA, sizeA);
    if (!bufA)
        return strovl();
    unsigned char *bufB = ReadFile(fileB, sizeB);
    if (!bufB) {
        free(bufA);
        return strovl();
    }

	strovl patch = CompareLines(strref((char*)bufA, (strl_t)sizeA),
								strref((char*)bufB, (strl_t)sizeB));
	
	free(bufB);
	free(bufA);
	return patch;
}

// Apply a patch to a file and return the new file
strovl PatchFile(const char *orig_file, const char *patch_file) {
	size_t sizeA, sizeB;
	unsigned char *bufA = ReadFile(orig_file, sizeA);
	if (!bufA)
		return strovl();
	unsigned char *bufB = ReadFile(patch_file, sizeB);
	if (!bufB) {
		free(bufA);
		return strovl();
	}
	
	// Rebuild the patched file
	strovl patched = PatchFile(strref((char*)bufA, (strl_t)sizeA),
							   strref((char*)bufB, (strl_t)sizeB));
	
	free(bufB);
	free(bufA);
	
	return patched;
}

int main(int argc, char **argv) {
	
	// help?
	if (argc<=1) {
		puts("Usage:\n"
			 "diff [-d] [-p] original_file update_file/patch_file output_file\n"
			 "-d: diff, output_file is a patch.\n"
			 "-p: patch, apply a patch to a file to generate an update.\n"
			 "if output file omitted, print output to stdout.\n");
		return 0;
	}
	
	// Parse some command line arguments
	bool diff = true;
	const char* orig_file=nullptr, *second_file=nullptr, *diff_out=nullptr;
	for (int a=1; a<argc; a++) {
		strref arg(argv[a]);
		if (arg.get_first()=='-') {
			++arg;
			switch (strref::tolower(arg.get_first())) {
				case 'd':
					diff = true;
					break;
				case 'p':
					diff = false;
					break;
				default:
					printf("unknown option " STRREF_FMT "\n", STRREF_ARG(arg));
					return 1;
			}
		} else if (!orig_file)
			orig_file = arg.get();
		else if (!second_file)
			second_file = arg.get();
		else if (!diff_out)
			diff_out = arg.get();
	}
	
	// diff or patch
	if (second_file) {
		strovl result;
		if (diff)
			result = DiffFiles(orig_file, second_file);
		else
			result = PatchFile(orig_file, second_file);
		
		if (result) {
			if (diff_out) {
				if (FILE *f = fopen(diff_out, "w")) {
					fwrite(result.charstr(), result.len(), 1, f);
					fclose(f);
				}
			} else
				puts(result.c_str());
			free(result.charstr());
		}
	}
	return 0;
}
