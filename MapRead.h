#pragma once
#include <inttypes.h>

struct TileSet {
	void* tileSetFile;
	uint8_t* imageData;
	uint8_t* imagePalettized;
	strref xmlName;		// from map file
	strref imageName;
	uint32_t first;			// from map file
	uint32_t count;
	uint32_t columns;
	uint32_t width, height;	// in tiles
	uint32_t tile_wid, tile_hgt;	// in tiles
	uint32_t img_wid, img_hgt;
};

// shift index 29 bits right to get rotation bits
enum TileRotations {
	Normal,
	RotateCW = 3,
	RotateCCW = 5,
	FlipY = 2,
	FlipX = 4
};

struct TileLayer {
	enum Type {
		// graphics types
		Text = 0,	// default mode
		TextMC,
		ECBM,
		Bitmap,
		BitmapMC,
		Bits,		// tile index => bit data with this shift

		// data types
		CRAM, // store tile index in upper 4 bits of CRAM
	};

	enum FlipType {
		NoFlip = 0,
		TileFlipX = 1,
		TileFlipY = 2,
		TileRot = 4,
		ToCRAM = 8
	};
	uint32_t *map;			// in 32 bit indices in ranges for each tileset
	uint8_t *flips;		// keep track of the flip bits from the map
	uint8_t *chars;
	uint8_t *screen;
	uint8_t *color;
	uint8_t *metaTileIndex;
	uint8_t *metaTileColor;
	uint8_t *metaLookupMap;
	uint8_t *metaLookupColor;
	uint8_t *metaMap;
	uint32_t numChars;		// number of unique chars encountered
	strref name;		// layer name if relevant at smoe point
	strref target;		// if merging data into another tile buffer
	strref flipTarget;	// layer flip/rotation bits copied here
	strref exportFile;	// filename without extension to export binary data to (chr, scr, col, mtm, mtt, bin)
	strref mergeChars;	// merge chars from this layer into named layer
	Type type;
	uint32_t bitShift;	// for Bits/CRAM type
	uint32_t bitLast;	// for Bits/CRAM type, this is the last bit the tile can index
	uint32_t flipBitShift;	// for copying tile flip bits into another data layer
	uint32_t flipType;	// what flip bits to include, and whether to put flip bits into color map (no meta!)
	uint32_t id;
	uint32_t screenBits, colorBits;
	uint32_t metaMapBits, metaLookupBits;
	uint32_t width, height;	// in tiles
	uint32_t metaX, metaY;
	uint32_t numMeta, numMetaIndex, numMetaColor;
	uint8_t bg[4];		// 4 for ECBM, 1 for bitmap, 3 for multicolor text
};

struct TileMap {
	void* tileMapFile;
	std::vector<TileSet> tileSets;
	std::vector<TileLayer> layers;
	strref stats;
	uint32_t width, height;	// in tiles
	uint32_t tile_wid, tile_hgt;	// in pixels
	uint32_t currTileSet;
	uint32_t currLayer;
};

char* LoadText(const char* filename, size_t& size);
bool LoadMap(const char *mapFilename, TileMap &map);
