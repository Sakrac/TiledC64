// TiledC64.cpp : Defines the entry point for the console application.
//
#include <vector>
#define STRUSE_IMPLEMENTATION
#include "struse.h"
#include "samples\xml.h"
#include "MapRead.h"
#include "ArtRead.h"
#include "BuildChars.h"
#include "MetaTiles.h"
#include "MapWrite.h"
#include "MergeMap.h"
#include "LoadImg.h"

// Fixes to do:
//	* char flip rotation considered for matching chars (bitmap)
//	* meta tile rotation depends on rotation of chars+color+char rotations
//	* take tile properties into account for rotation matching
//	* enumerate tile flips for Bits layers
//	* add char rotations to tile meta maps

// Added:
//	* properties per tile in tiled
//	 - Rot, FlipX, FlipY, FlipEnum - can disable tile flips/rotations if they don't matter for that tile



// c:\code\4k2\assets\Castle4x4.tmx -palette=c:\code\TiledC64\pixcen_default.png

#define MAX_ARGS 32

char* GetSwitch(const char* match, char** swtc, int swtn)
{
	int l = (int)strlen(match);
	while (swtn) {
		if (_strnicmp(match, *swtc, l) == 0) {
			if ((*swtc)[l] == '=') return *swtc + l + 1;
			else return *swtc;
		}
		++swtc;
		--swtn;
	}
	return 0;
}


int main(int argc, char* argv[])
{
	const char* args[MAX_ARGS];
	char* swtc[MAX_ARGS];
	int argn = 0;
	int swtn = 0;
	for (int a = 0; a < argc; ++a) {
		if (argv[a][0] == '-') { swtc[swtn++] = argv[a] + 1; } else { args[argn++] = argv[a]; }
	}

	// check for custom palette
	if (const char* paletteFile = GetSwitch("palette", swtc, swtn)) {
		if (!LoadPalette(paletteFile)) {
			printf("Could not open palette image \"%s\"\n", paletteFile);
			return 1;
		}
	}

	// should maps be merged?
	if (GetSwitch("merge", swtc, swtn)) {
		int metaX = 2, metaY = 2;
		if (argn < 3) {
			printf("merging maps requires a source list and a destination file\n");
			return 1;
		}
		if (!MergeMaps(args[1], args[2])) {
			printf("Error merging maps\n");
			return 1;
		}
		return 0;
	}


	// load map file (first arg)
	if (argn < 2) {
		printf("missing map name\n");
		return 1;
	}
	TileMap map = {};
	int error = 0;

	// previewing an image file?
	if (GetSwitch("img", swtc, swtn)) {
		const char* colors = GetSwitch("col", swtc, swtn);
		const char* meta = GetSwitch("meta", swtc, swtn);
		const char* screenBits = GetSwitch("screenBits", swtc, swtn);
		const char* colorBits = GetSwitch("colorBits", swtc, swtn);
		const char* metaMapBits = GetSwitch("metaMapBits", swtc, swtn);
		const char* lookupBits = GetSwitch("metaLookupBits", swtc, swtn);
		const char* type = GetSwitch("type", swtc, swtn);
		const char* exp = GetSwitch("export", swtc, swtn);
		const char* stats = GetSwitch("stats", swtc, swtn);
		if (!LoadImgMap(map, args[1], colors, meta, screenBits, colorBits, metaMapBits, lookupBits, type, exp, stats)) {
			error++;
		}
	} else {
		if (!LoadMap(args[1], map)) {
			printf("failed to load map or tileset xml\n");
			++error;
		}
		if (!LoadTileSetImages(map, args[1])) {
			printf("failed to load tileset images\n");
			++error;
		}
	}

	if (!error) {
		// convert layers first pass
		//	- any simple type will first make char/screen/color arrays
		for (int l = 0, n = (int)map.layers.size(); l < n; ++l) {
			switch (map.layers[l].type) {
				case TileLayer::Text:
					if (!ConvertTextLayer(map, l)) { error++; }
					break;
				case TileLayer::TextMC:
					if (!ConvertTextMCLayer(map, l)) { error++; }
					break;
				case TileLayer::ECBM:
					if (!ConvertECBMLayer(map, l)) { error++; }
					break;
				case TileLayer::Bitmap:
					if (!ConvertBitmapLayer(map, l)) { error++; }
					break;
				case TileLayer::CRAM:	// CRAM is the same as bits but will just go to color instead of map data
				case TileLayer::Bits:	// Bits can be either exported as is or merged into other layer's screen data
					// TODO: Write tileset into bits of map data
					ConvertDataLayer(map, l);
					break;
				default:
					break;
			}
		}

		// Merge chars from layers that merge into others
		for (int l = 0, n = (int)map.layers.size(); l < n; ++l) {
			if (map.layers[l].mergeChars) {
				for (int l2 = 0; l2 < n; ++l2) {
					if (map.layers[l].mergeChars.same_str(map.layers[l2].name)) {
						if (!MergeChars(map, map.layers[l], map.layers[l2])) {
							printf("failed to merge chars from layer " STRREF_FMT " to layer " STRREF_FMT "\n",
								   STRREF_ARG(map.layers[l].name), STRREF_ARG(map.layers[l2].name));
							++error;
						}
						break;
					}
				}
			}
		}


		// Copy layer map data around before generating meta tiles!
		for (int l = 0, n = (int)map.layers.size(); l < n; ++l) {
			if (map.layers[l].target) {
				for (int l2 = 0; l2 < n; ++l2) {
					if (map.layers[l].target.same_str(map.layers[l2].name)) {
						if (!MergeLayerData(map, map.layers[l], map.layers[l2])) {
							printf("failed to merge data from layer " STRREF_FMT " to layer " STRREF_FMT "\n",
								   STRREF_ARG(map.layers[l].name), STRREF_ARG(map.layers[l2].name));
							++error;
						}
						break;
					}
				}
			}
		}

		// Apply meta tiling as needed
		for (int l = 0, n = (int)map.layers.size(); l < n; ++l) {
			MakeMetaLayer(map, map.layers[l]);
		}

		// If flip bits are requested they should be applied to the meta data, not the individual chars.
		for (int l = 0, n = (int)map.layers.size(); l < n; ++l) {
			if (map.layers[l].flipTarget) {
				strref flipLayer = map.layers[l].flipTarget.before_or_full(',');
				flipLayer.trim_whitespace();
				for (int l2 = 0; l2 < n; ++l2) {
					if (flipLayer.same_str(map.layers[l2].name)) {
						if (!MergeFlipData(map, map.layers[l], map.layers[l2])) {
							printf("Failed to merge layer " STRREF_FMT " into layer " STRREF_FMT "\n",
								   STRREF_ARG(map.layers[l].name), STRREF_ARG(map.layers[l2].name));
							++error;
						}
						break;
					}
				}
			}
		}

		// summarize the result
		WriteMapStats(map, args[1]);

		// attenot to write output
		WriteMapData(map, args[1]);
	}

	return error;
}

