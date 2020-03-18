// after processing 8x8 pixel tiles, make a meta lookup and meta map
#include <vector>
#include "struse\struse.h"
#include "MapRead.h"


// store meta tile indices & color smartly...
// can color be packed?
// combined charset meta tiles?

// 00 10 20
// 01 11 21
// c c c c c c

// draw can be slow - multiply by w*h is ok
// same meta tile / different colors? -> look up color first, first byte is meta tile map index?

// meta tiles should just be a packed array of the data already in screen/color at this stage - no reordering logic
// 1 byte index -> 1 byte index to chars, 1 byte index to color?

void MakeMetaLayer(TileMap& map, TileLayer& layer)
{
	if (layer.metaX > 1 || layer.metaY > 1) {
		size_t charsPerTile = (size_t)layer.metaX * (size_t)layer.metaY;
		size_t width = layer.width;
		size_t height = layer.height;
		size_t metaX = layer.metaX;
		size_t metaY = layer.metaY;
		size_t metaSize = metaX * metaY;
		size_t metaWidth = width / metaX;
		size_t metaHeight = height / metaY;
		size_t maxTiles = metaWidth * metaHeight;
		uint8_t* metaTileIndex = layer.screen ? (uint8_t*)calloc(1, maxTiles * charsPerTile) : nullptr;
		uint8_t* metaTileColor = layer.color ? (uint8_t*)calloc(1, maxTiles * charsPerTile) : nullptr;
		uint8_t* metaLookupMap = (layer.metaLookupBits && layer.screen && layer.color) ? (uint8_t*)calloc(1, maxTiles) : nullptr;
		uint8_t* metaLookupColor = (layer.screen && layer.color) ? (uint8_t*)calloc(1, maxTiles) : nullptr;
		uint8_t* metaMap = (uint8_t*)calloc(1, maxTiles), *metaOut = metaMap;

		layer.metaLookupMap = metaLookupMap;
		layer.metaLookupColor = metaLookupColor;
		layer.metaTileIndex = metaTileIndex;
		layer.metaTileColor = metaTileColor;
		layer.metaMap = metaMap;

		uint32_t numMeta = 0;
		uint32_t numIndex = 0;
		uint32_t numColor = 0;

		for (size_t y = 0; y < metaHeight; ++y) {
			for (size_t x = 0; x < metaWidth; ++x) {
				size_t tl = y * metaY * width + x * metaX;
				uint8_t f = 0;// layer.flips ? layer.flips[tl] : 0;
				// copy map into a next potential tile to compare with previous ones
				uint8_t* nextTiles = metaTileIndex + numIndex * metaSize;
				uint8_t* nextColor = metaTileColor + numColor * metaSize;
				for (size_t my = 0; my < metaY; ++my) {
					for (size_t mx = 0; mx < metaX; ++mx) {
						size_t xp = mx;
						size_t yp = my;
						if (f == RotateCCW) {
							xp = metaY - 1 - my;
							yp = mx;
						} else if (f == RotateCW) {
							xp = my;
							yp = metaX - 1 - mx;
						} else {
							if (f & FlipX) { xp = metaX - 1 - mx; }
							if (f & FlipY) { yp = metaY - 1 - my; }
						}
						size_t slot = tl + xp + yp * width;
						if (metaTileIndex) { nextTiles[mx + my * metaX] = layer.screen[slot]; }
						if (metaTileColor) { nextColor[mx + my * metaX] = layer.color[slot]; }
					}
				}
				size_t matchIndex = 0;
				size_t matchColor = 0;
				if (!metaLookupMap) {
					// both color and tiles must match
					if (metaTileIndex) {
						for (; matchIndex < numIndex; ++matchIndex) {
							bool match = true;
							uint8_t* checkTiles = metaTileIndex + matchIndex * metaSize;
							uint8_t* checkColor = metaTileColor ? (metaTileColor + matchIndex * metaSize) : nullptr;
							for (size_t chk = 0; chk < metaSize; ++chk) {
								if (checkTiles[chk] != nextTiles[chk] || (checkColor && (checkColor[chk] != nextColor[chk]))) {
									match = false;
									break;
								}
							}
							if (match) { break; }
						}
						if (matchIndex >= numIndex) { ++numIndex; ++numColor; }
						matchColor = matchIndex;
					}
				} else {
					// match with existing index meta tles
					if (metaTileIndex) {
						for (; matchIndex < numIndex; ++matchIndex) {
							bool match = true;
							uint8_t* checkTiles = metaTileIndex + matchIndex * metaSize;
							for (size_t chk = 0; chk < metaSize; ++chk) {
								if (checkTiles[chk] != nextTiles[chk]) {
									match = false;
									break;
								}
							}
							if (match) { break; }
						}
						if (matchIndex >= numIndex) { ++numIndex; }
					}
					// match with existing color meta tles
					if (metaTileColor) {
						for (; matchColor < numColor; ++matchColor) {
							bool match = true;
							uint8_t* checkColor = metaTileColor + matchColor * metaSize;
							for (size_t chk = 0; chk < metaSize; ++chk) {
								if (checkColor[chk] != nextColor[chk]) {
									match = false;
									break;
								}
							}
							if (match) { break; }
						}
						if (matchColor >= numColor) { ++numColor; }
					}
				}
				// match a color/index combo?
				size_t matchMeta = matchIndex;
				if (metaLookupMap && metaLookupColor) {
					matchMeta = 0;
					for (; matchMeta < numMeta; ++matchMeta) {
						if (metaLookupMap[matchMeta] == (uint8_t)matchIndex && metaLookupColor[matchMeta] == (uint8_t)matchColor) {
							break;
						}
					}
					if (matchMeta >= numMeta) {
						metaLookupMap[matchMeta] = (uint8_t)matchIndex;
						metaLookupColor[matchMeta] = (uint8_t)matchColor;
						++numMeta;
					}
				} else if (metaTileIndex) {
					matchMeta = matchIndex;
				} else if (metaTileColor) {
					matchMeta = matchColor;
				}
				if (metaOut) { *metaOut++ = (uint8_t)matchMeta; }
			}
		}
		layer.numMeta = numMeta;
		layer.numMetaIndex = numIndex;
		layer.numMetaColor = numColor;
	}

}
