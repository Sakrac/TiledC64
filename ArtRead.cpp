#include <vector>
#include "struse\struse.h"
#include "MapRead.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "ArtRead.h"

static uint8_t sPalette[16][3] = {
	{ 0, 0, 0 },		// #000000
	{ 255, 255, 255 },	// #FFFFFF
	{ 137, 64, 54 },	// #880000
	{ 122, 191, 199 },	// #AAFFEE
	{ 138, 70, 174 },	// #CC44CC
	{ 104, 169, 65 },	// #00CC55
	{ 62, 49, 162 },	// #0000AA
	{ 208, 220, 113 },	// #EEEE77
	{ 144, 95, 37 },	// #DD8855
	{ 92, 71, 0 },		// #664400
	{ 187, 119, 109 },	// #FF7777
	{ 85, 85, 85 },		// #555555
	{ 128, 128, 128 },	// #808080
	{ 172, 234, 136 },	// #AAFF66
	{ 124, 112, 218 },	// #0088FF
	{ 171, 171, 171 }	// #ABABAB
};

// if a color is not part of a char scan for the best replacement
int GetClosestColor(uint8_t color, uint8_t* colors, int numColors)
{
	int bestIndex = 0;
	int minDiff = 0x7fffffff;
	int r0 = sPalette[color][0];
	int g0 = sPalette[color][1];
	int b0 = sPalette[color][2];
	for (int c = 0; c < numColors; ++c) {
		int dr = sPalette[colors[c]][0] - r0;
		int dg = sPalette[colors[c]][1] - g0;
		int db = sPalette[colors[c]][2] - b0;
		int diff = dr * dr + dg * dg + db * db;
		if (diff < minDiff) {
			bestIndex = c;
			minDiff = diff;
		}
	}
	return bestIndex;
}

uint8_t* PalettizeImage(uint8_t* src, size_t pixels)
{
	uint8_t *dst = (uint8_t*)calloc(1, pixels), *out=dst;
	if (out) {
		for (size_t p = 0; p < pixels; ++p) {
			int r = *src++;
			int g = *src++;
			int b = *src++;
			int a = *src++;
			if (a < 128) { *out++ = 15; }
			else {
				int minDiff = 0x7fffffff;
				size_t best = 0;
				for (size_t c = 0; c < 16; ++c) {
					int dr = sPalette[c][0] - r;
					int dg = sPalette[c][1] - g;
					int db = sPalette[c][2] - b;
					int diff = dr * dr + dg * dg + db * db;
					if (diff < minDiff) {
						best = c;
						minDiff = diff;
					}
				}
				*out++ = (uint8_t)best;
			}
		}
	}
	return dst;
}

bool LoadTileSetImages(TileMap& map, const char* mapFilename)
{
	strref mapFilestr(mapFilename);
	strref path = mapFilestr.get_substr(0, mapFilestr.find_last('/', '\\') + 1);

	for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
		if (tileset->imageName) {
			strown<256> file(path);
			file.append(tileset->imageName);
			int x,y,n;
			uint8_t *data = stbi_load(file.c_str(), &x, &y, &n, 0);
			if (data) {
				if (n != 4) {	// just convert all images to rgb+alpha
					uint8_t *exp = (uint8_t*)malloc(x*y * 4);
					memset(exp, 0xff, x*y * 4);
					uint8_t *out = exp, *in=data;
					for (size_t p = 0, np = x*y; p < np; ++p) {
						for (size_t c = 0; c < (size_t)n; ++c) { *out++ = *in++; }
						out += 4 - n;
					}
					stbi_image_free(data);
					data = exp;
				}
				tileset->imageData = data;
				tileset->img_wid = x;
				tileset->img_hgt = y;
				tileset->imagePalettized = PalettizeImage(data, x * y);
			}
		} else { return false; }
	}
	return true;
}

bool LoadPalette(const char *paletteFilename)
{
	int x, y, n;
	uint8_t *data = stbi_load(paletteFilename, &x, &y, &n, 0);
	if (data) {
		uint8_t *scan = data;
		size_t colIdx = 0;
		size_t pixels = x * y, curr = 1;
		for (size_t copy = 0; copy < 3; ++copy) { sPalette[0][copy] = scan[copy]; }
		while(colIdx < 15 && curr < pixels) {
			size_t comp = 0;
			for (; comp < 3; ++comp) {
				if (sPalette[colIdx][comp] != scan[comp]) { break; }
			}
			if (comp < 3) {
				++colIdx;
				for (size_t copy = 0; copy < 3; ++copy) { sPalette[colIdx][copy] = scan[copy]; }
			}
			scan += n;
			curr++;
		}
		stbi_image_free(data);
		return true;
	}
	return false;
}

