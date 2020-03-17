// convert 
#include <vector>
#include "struse\struse.h"
#include "MapRead.h"
#include "ArtRead.h"
#include <inttypes.h>
#include "BuildChars.h"

// convert from Tiled's flip/rotation order to a more organized bit field
// 0-no, 1-cw+x, 2-y, 3-cw+x+y, 4-x, 5-cw, 6-x+y, 7-cw+y 
static TileLayer::FlipType TiledFlipFlop[] = {
	TileLayer::NoFlip,
	(TileLayer::FlipType)(TileLayer::TileRot | TileLayer::TileFlipX),
	TileLayer::TileFlipY,
	(TileLayer::FlipType)(TileLayer::TileRot | TileLayer::TileFlipX | TileLayer::TileFlipY),
	TileLayer::TileFlipX,
	TileLayer::TileRot,
	(TileLayer::FlipType)(TileLayer::TileFlipX | TileLayer::TileFlipY),
	(TileLayer::FlipType)(TileLayer::TileRot | TileLayer::TileFlipY)
};

// counts how many unique tile indices are used in this layer, the number of binary unique tiles are less or equal
int CountRawTiles(const TileMap& map, int layerIndex)
{
	if (!map.layers[layerIndex].map) { return 0; }
	size_t last = 0;
	for (std::vector<TileSet>::const_iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
		if (tileset->first > last) { last = tileset->first + tileset->count; }
	}
	if (last > 0) {
		const TileLayer &layer = map.layers[layerIndex];
		uint32_t* used = (uint32_t*)calloc(1, ((last + 31) / 32) * sizeof(uint32_t));
		for (size_t i = 0, n = layer.width * layer.height; i < n; ++i) {
			size_t tile = layer.map[i] & 0xfffffff;	// top nibble is flip x/y & rotate bits
			if (tile < last) {
				used[tile >> 5] |= uint32_t(1) << (tile & 0x1f);
			}
		}
		int unique = 0;
		for (size_t t = 0; t < last; ++t) {
			if (used[t >> 5] & (uint32_t(1) << (t & 0x1f))) { unique++; }
		}
		free(used);
		return unique;
	}
	return 0;
}

// shared system for adding tiles for different modes?
//	size_t numChars: number of unique chars encountered so far
//	uint64_t *chars: chars encountered so far. first is 0
int FindCharInSet(uint64_t curr, const size_t numChars, const uint64_t* chars)
{
	for (size_t c = 0; c < numChars; ++c) {
		if (curr == chars[c]) { return (int)c; }
	}
	return -1;
}

// check against various flips and rotations of existing chars
uint64_t FlipChar(uint64_t o, bool flipX, bool flipY, bool rot)
{
	if (rot) {
		// a rotated character is 90 degrees clockwise so to match an existing
		// character rotate this character counter clockwise
		// 70 71 72 73 74 75 76 77
		// 60 61 ...
		// 07 0f 17 1f 27 2f 37 3f
		// 06 0e ...
		uint64_t e = 0;
		for (size_t y = 0; y < 8; ++y) for (size_t x = 0; x < 8; ++x) {
			if (o & (1ULL << ((y << 3) | x))) { e |= 1ULL << (((x^7) << 3) | y); }
		}
	}
	if (flipX) {
		uint64_t h = ((o & 0xf0f0f0f0f0f0f0f0ULL) >> 4) | ((o & 0x0f0f0f0f0f0f0f0fULL) << 4);
		uint64_t q = ((h & 0xccccccccccccccccULL) >> 2) | ((h & 0x3333333333333333ULL) << 2);
		uint64_t e = ((q & 0xaaaaaaaaaaaaaaaaULL) >> 1) | ((h & 0x5555555555555555ULL) << 1);
		o = e;
	}
	if (flipY) {
		uint64_t h = ((o & 0xffffffff00000000ULL) >> 32) | ((o & 0x00000000ffffffffULL) << 32);
		uint64_t q = ((h & 0xffff0000ffff0000ULL) >> 16) | ((h & 0x0000ffff0000ffffULL) << 16);
		uint64_t e = ((q & 0xff00ff00ff00ff00ULL) >> 8) | ((e & 0x00ff00ff00ffff00ULL) << 8);
		o = e;
	}
	return o;
}

int FindCharInSet(uint64_t chr, const uint64_t* set, int setSize)
{
	for (int i = 0; i < setSize; ++i) {
		if (set[i] == chr) { return i; }
	}
	return -1;
}

int FindMatchChar(uint32_t* permutation, uint64_t charBits, const int numChars, const uint64_t* chars, bool canFlipX, bool canFlipY, bool canRot, bool canInv)
{
	int match = -1;
	for (size_t p = 0; p < 16 && match < 0; ++p) {
		bool inv = !!(p & 1);
		bool flipX = !!(p & 2);
		bool flipY = !!(p & 4);
		bool rot = !!(p & 8);
		bool valid = true;
		if (p == 0 || (flipX && canFlipX) || (flipY && canFlipY) || (rot && canRot) && (inv && canInv)) {
			uint64_t charPermute = FlipChar(charBits, flipX, flipY, rot);
			if (inv) { charPermute = ~charPermute; }
			match = FindCharInSet(charPermute, chars, numChars);
			if (match >= 0) {
				uint32_t pm = (inv ? TileLayer::Inverted : 0) |
					(rot ? TileLayer::TileRot : 0) |
					(flipX ? TileLayer::TileFlipX : 0) |
					(flipY ? TileLayer::TileFlipY : 0);
				if (permutation) { *permutation = pm; }
				return match;
			}
		}
	}
	return -1;
}

// count color usage in one 8x8
void CharColorCounts(uint32_t* counts, uint8_t *tile, size_t width)
{
	memset(counts, 0, sizeof(uint32_t) * 16);
	for (size_t y = 0; y < 8; ++y) {
		for (size_t x = 0; x < 8; ++x) {
			uint8_t c = tile[x];
			++counts[c & 0xf];
		}
		tile += width;
	} 
}

uint8_t Pick2Colors(uint32_t* counts, uint8_t* second)
{
	uint8_t c0 = 0, c1 = 0;
	uint32_t num0 = 0, num1 = 0;
	for (size_t col = 0; col < 16; ++col) {
		uint32_t n = counts[col];
		if (n == 0x40) { // only one color
			if (second) { *second = (uint8_t)col; }
			return (uint8_t)col;
		}
		if (n > num0) {
			if (n > num1) {
				num0 = num1;
				c0 = c1;
				num1 = n;
				c1 = (uint8_t)col;
			} else {
				num0 = n;
				c0 = (uint8_t)col;
			}
		}
	}
	if (second) { *second = c1; }
	return c0;
}

uint8_t PickCharColor(uint8_t bg, uint8_t* tile, size_t width)
{
	uint32_t counts[16] = {};
	CharColorCounts(counts, tile, width);
	uint8_t	c = 0;
	uint32_t num = 0;
	for (size_t col = 0; col < 16; ++col) {
		uint32_t n = counts[col];
		if ((uint8_t)col != bg && n > num) {
			c = (uint8_t)col;
			num = n;
		}
	}
	return c;
}

uint8_t Pick2ColorsFromTile(uint8_t* second, uint8_t *tile, size_t width)
{
	uint32_t counts[16] = {};
	CharColorCounts(counts, tile, width);
	return Pick2Colors(counts, second);
}

uint64_t GetHiresChar(uint8_t *tile, uint8_t colors[2], size_t width)
{
	if (colors[0] == colors[1]) { return 0; }
	uint64_t bits = 0;	// return 64 bits
	uint8_t* bytes = (uint8_t*)&bits; // but assign in bytes so endian is irrelevant
	for (size_t y = 0; y < 8; ++y) {
		uint8_t byte = 0;
		for (size_t x = 0; x < 8; ++x) {
			uint8_t c = tile[x];
			byte <<= 1;
			if (c == colors[1]) {
				byte |= 1;
			} else if (c != colors[0]) {
				if (GetClosestColor(c, colors, 2)) { byte |= 1; }
			}
		}
		bytes[y] = byte;
		tile += width;
	}
	return bits;
}

// check if char should be stored as a hires char
bool IsCharMCHires(uint8_t* tile, uint8_t bg)
{
	uint8_t fg = 0xff;
	bool doublewide = true;
	for (size_t y = 0; y < 8; ++y) {
		for (size_t x = 0; x < 8; x+=2) {
			uint8_t c = tile[x];
			if (c != bg) {
				if (c >= 8) { return false; }
				else if (fg == 0xff) { fg = c; }
				if (tile[x + 1] == bg) { doublewide = false; }
				else if (tile[x + 1] != fg) { return false; }
			} else if (tile[x + 1] != bg) {
				if (tile[x + 1] >= 8) { return false; }
				if (fg == 0xff) { fg = tile[x + 1]; doublewide = false; }
			}
		}
	}
	return !doublewide;
}

uint8_t* GetTileAddress(int x, int y, uint8_t *image, uint32_t width)
{
	return image + y * 8 * width + x * 8;
}

// returns -1 if not a background color
int ECBMBackIndex(const TileLayer &layer, uint8_t color)
{
	for (int bg = 0; bg < 4; ++bg) {
		if (color == layer.bg[bg]) { return bg; }
	}
	return -1;
}


// returns number of unique chars
bool ConvertTextLayer(TileMap& map, int layerIndex)
{
	int maxChars = CountRawTiles(map, layerIndex);

	// big buffer - may have > 256 characters in a set
	uint64_t* chars = (uint64_t*)calloc(1, maxChars * sizeof(uint64_t));
	int numChars = 1;	// keep 0 clear

	// map columns and rows doesn't matter, iterate all
	TileLayer& layer = map.layers[layerIndex];
	size_t layerSize = (size_t)layer.width * (size_t)layer.height;
	uint32_t* tiles = layer.map;
	uint8_t* screen = (uint8_t*)calloc(1, layerSize);
	uint8_t* color = (uint8_t*)calloc(1, layerSize);
	for (size_t slot = 0; slot < layerSize; ++slot) {
		uint32_t index = *tiles++;
		uint32_t flip = index >> 29;
		index &= 0xfffffff;
		if (index) {	// empty char, store with black color & index 0
			// find the corresopnding tileset
			for (size_t t = 0, nt = map.tileSets.size(); t < nt; ++t) {
				const TileSet& tileSet = map.tileSets[t];
				if (index >= tileSet.first && index <= (tileSet.first + tileSet.count)) {
					if (tileSet.imagePalettized) {
						// find 2 colors in a tile and convert the current tile to bits
						index -= tileSet.first;
						uint8_t* tileArt = GetTileAddress(index % tileSet.width, index / tileSet.width, tileSet.imagePalettized, tileSet.img_wid);
						uint8_t c[2] = { layer.bg[0], PickCharColor(layer.bg[0], tileArt, tileSet.img_wid) };
						uint64_t charBits = GetHiresChar(tileArt, c, tileSet.img_wid);

						// is there a match?
						int match = FindCharInSet(charBits, chars, numChars);
						if (match < 0) {
							match = numChars++;
							chars[match] = charBits;
						}
						screen[slot] = (uint8_t)match;
						color[slot] = c[1];
					}
					break;
				}
			}
		}
	}

	// store the result in the layer
	layer.chars = (uint8_t*)chars;
	layer.screen = screen;
	layer.color = color;
	layer.numChars = numChars;
	return true;
}

// returns number of unique chars
bool ConvertTextMCLayer(TileMap& map, int layerIndex)
{
	int maxChars = CountRawTiles(map, layerIndex);

	// big buffer - may have > 256 characters in a set
	uint64_t* chars = (uint64_t*)calloc(1, maxChars * sizeof(uint64_t));
	int numChars = 1;	// keep 0 clear

	// map columns and rows doesn't matter, iterate all
	TileLayer& layer = map.layers[layerIndex];
	size_t layerSize = layer.width * layer.height;
	uint32_t* tiles = layer.map;
	uint8_t* screen = (uint8_t*)calloc(1, layerSize);
	uint8_t* color = (uint8_t*)calloc(1, layerSize);
	for (size_t slot = 0; slot < layerSize; ++slot) {
		uint32_t index = *tiles++;
		uint32_t flip = index >> 28;
		index &= 0xfffffff;
		if (index) {	// empty char, store with black color & index 0
			// find the corresopnding tileset
			for (size_t t = 0, nt = map.tileSets.size(); t < nt; ++t) {
				const TileSet& tileSet = map.tileSets[t];
				if (index >= tileSet.first && index <= (tileSet.first + tileSet.count)) {
					if (tileSet.imagePalettized) {
						// find 2 colors in a tile and convert the current tile to bits
						index -= tileSet.first;
						uint8_t* tileArt = GetTileAddress(index % tileSet.width, index / tileSet.width, tileSet.imagePalettized, tileSet.img_wid);
						uint32_t charBits = 0;
						uint8_t charColor = 0;
						if (IsCharMCHires(tileArt, layer.bg[0])) {
							charColor = PickCharColor(layer.bg[0], tileArt, tileSet.img_wid);
							uint8_t c[2] = { layer.bg[0], charColor };
							uint64_t charBits = GetHiresChar(tileArt, c, tileSet.img_wid);
						} else {

						}

						// is there a match?
						int match = FindCharInSet(charBits, chars, numChars);
						if (match < 0) {
							match = numChars++;
							chars[match] = charBits;
						}
						screen[slot] = (uint8_t)match;
						color[slot] = charColor;
					}
					break;
				}
			}
		}
	}

	// store the result in the layer
	layer.chars = (uint8_t*)chars;
	layer.screen = screen;
	layer.color = color;
	layer.numChars = numChars;
	return true;
}


// returns number of unique chars
bool ConvertECBMLayer(TileMap& map, int layerIndex)
{
	int maxChars = CountRawTiles(map, layerIndex);

	// true if both colors in char are background colors and can be inverted to match another.
	char* canInvertChar = (char*)calloc(1, maxChars);

	// big buffer - may have > 256 characters in a set
	uint64_t* chars = (uint64_t*)calloc(1, maxChars * sizeof(uint64_t));
	int numChars = 1;	// keep 0 clear

	// map columns and rows doesn't matter, iterate all
	TileLayer& layer = map.layers[layerIndex];
	size_t layerSize = layer.width * (layer.height-(layer.metaY ? layer.height%layer.metaY : 0));
	uint32_t* tiles = layer.map;
	uint8_t* screen = (uint8_t*)calloc(1, layerSize);
	uint8_t* color = (uint8_t*)calloc(1, layerSize);
	uint8_t* flips = (uint8_t*)calloc(1, layerSize);
	for (size_t slot = 0; slot < layerSize; ++slot) {
		uint32_t index = *tiles++;
		uint32_t flip = index >> 29; // flip/rot are top 3 bits
		index &= 0xfffffff;
		flips[slot] = flip;	// even if empty track the flip
		if (index) {	// empty char, store with black color & index 0
			// find the corresopnding tileset
			for (size_t t = 0, nt = map.tileSets.size(); t < nt; ++t) {
				const TileSet& tileSet = map.tileSets[t];
				if (index >= tileSet.first && index <= (tileSet.first + tileSet.count)) {
					if (tileSet.imagePalettized) {
						// find 2 colors in a tile and convert the current tile to bits
						index -= tileSet.first;
						uint8_t* tileArt = GetTileAddress(index % tileSet.width, index / tileSet.width, tileSet.imagePalettized, tileSet.img_wid);
						uint8_t c[2];
						c[0] = Pick2ColorsFromTile(&c[1], tileArt, tileSet.img_wid);
						int b[2] = { ECBMBackIndex(layer, c[0]), ECBMBackIndex(layer, c[1]) };	// background oclor index
						if ((b[0] < 0 && b[1] >= 0) || (b[0] >= 0 && b[1] >= 0 && b[1] < b[0])) {
							int bt = b[0]; b[0] = b[1]; b[1] = bt;
							uint8_t ct = c[0]; c[0] = c[1]; c[1] = ct;
						}
						bool canInvert = b[0] >= 0 && b[1] >= 0;

						uint64_t charBits = (c[0] == c[1] && b[0] >= 0) ? 0 : GetHiresChar(tileArt, c, tileSet.img_wid);

						// is there a match?
						int match = FindCharInSet(charBits, chars, numChars);
						if (match < 0) {
							match = FindCharInSet(charBits ^ 0xffffffffffffffffULL, chars, numChars);
							// if no match check for an inverted version
							if (match >= 0) {
								if (canInvert) {
									int bt = b[0]; b[0] = b[1]; b[1] = bt;
									uint8_t ct = c[0]; c[0] = c[1]; c[1] = ct;
								} else if (canInvertChar[match]) {
									// invert character in the set, update screen & color data!
									canInvertChar[match] = false;
									chars[match] = charBits;
									for (size_t i = 0; i < slot; ++i) {
										if ((screen[i] & 0x3f) == (uint8_t)match) {
											uint8_t bs = ECBMBackIndex(layer, color[i] & 0xff);
											color[i] = layer.bg[screen[i] >> 6];
											screen[i] = (screen[i] & 0x3f) | (bs << 6);
										}
									}
								} else {
									match = -1;
								}
							}
							if (match < 0) { // new char!
								match = numChars++;
								chars[match] = charBits;
								canInvertChar[match] = canInvert;
							}
						}
						screen[slot] = (((uint8_t)match) & 0x3f) | (uint8_t)(b[0] << 6);
						color[slot] = c[1];
					}
					break;
				}
			}
		}
	}

	// store the result in the layer
	layer.chars = (uint8_t*)chars;
	layer.screen = screen;
	layer.color = color;
	layer.numChars = numChars;
	return true;
}

// returns number of unique chars
bool ConvertBitmapLayer(TileMap& map, int layerIndex)
{
	int maxChars = CountRawTiles(map, layerIndex);

	// big buffer - may have > 256 characters in a set
	uint64_t* chars = (uint64_t*)calloc(1, maxChars * sizeof(uint64_t));
	int numChars = 1;	// keep 0 clear

	// map columns and rows doesn't matter, iterate all
	TileLayer& layer = map.layers[layerIndex];
	size_t layerSize = layer.width * layer.height;
	uint32_t* tiles = layer.map;
	uint8_t* screen = (uint8_t*)calloc(1, layerSize);
	uint8_t* color = (uint8_t*)calloc(1, layerSize);
	uint8_t* flips = (uint8_t*)calloc(1, layerSize);
	for (size_t slot = 0; slot < layerSize; ++slot) {
		uint32_t index = *tiles++;
		uint32_t flip = (uint32_t)TiledFlipFlop[index >> 29];
		index &= 0xfffffff;
		flips[slot] = flip;	// even if empty track the flip
		if (index) {	// empty char, store with black color & index 0
			// find the corresopnding tileset
			for (size_t t = 0, nt = map.tileSets.size(); t < nt; ++t) {
				const TileSet& tileSet = map.tileSets[t];
				if (index >= tileSet.first && index <= (tileSet.first + tileSet.count)) {
					if (tileSet.imagePalettized) {
						// find 2 colors in a tile and convert the current tile to bits
						index -= tileSet.first;
						uint8_t* tileArt = GetTileAddress(index % tileSet.width, index / tileSet.width, tileSet.imagePalettized, tileSet.img_wid);
						uint8_t c[2];
						c[0] = Pick2ColorsFromTile(&c[1], tileArt, tileSet.img_wid);
						uint64_t charBits = GetHiresChar(tileArt, c, tileSet.img_wid);

						// is there a match?
						uint32_t pm = 0;
						int match = FindMatchChar(&pm, charBits, numChars, chars,
												  !!(layer.flipType & TileLayer::TileFlipX),
												  !!(layer.flipType & TileLayer::TileFlipY),
												  !!(layer.flipType & TileLayer::TileRot),
												  true);
						if (match >= 0) {
							// apply flip, rot & invert colors if a matching permutation was found
							if (pm & TileLayer::Inverted) {
								uint8_t ct = c[0]; c[0] = c[1]; c[1] = ct;
							}
							if (pm & TileLayer::TileFlipX) { flips[slot] ^= TileLayer::TileFlipX; }
							if (pm & TileLayer::TileFlipY) { flips[slot] ^= TileLayer::TileFlipY; }
							if (pm & TileLayer::TileRot) { flips[slot] ^= TileLayer::TileRot; }
						}

						if (match < 0) { // new char!
							match = numChars++;
							chars[match] = charBits;
						}
						screen[slot] = match;
						color[slot] = (c[1]<<4)|c[0];
					}
					break;
				}
			}
		}
	}

	// store the result in the layer
	layer.chars = (uint8_t*)chars;
	layer.screen = screen;
	layer.color = color;
	layer.numChars = numChars;
	layer.flips = flips;
	return true;
}

void ConvertDataLayer(TileMap& map, int layerIndex)
{
	TileLayer& layer = map.layers[layerIndex];
	size_t layerSize = (size_t)layer.width * (size_t)layer.height;
	uint32_t* tiles = layer.map;
	uint8_t* screen = (uint8_t*)calloc(1, layerSize);	// data layer -> screen data
	for (size_t slot = 0; slot < layerSize; ++slot) {
		uint32_t index = *tiles++;
		uint32_t flip = index >> 29;
		index &= 0xfffffff;
		if (index) {	// empty char, store with black color & index 0
						// find the corresopnding tileset
			for (size_t t = 0, nt = map.tileSets.size(); t < nt; ++t) {
				const TileSet& tileSet = map.tileSets[t];
				if (index >= tileSet.first && index < (tileSet.first + tileSet.count)) {
					if (tileSet.imagePalettized) {
						index -= tileSet.first;

						uint8_t dataIndex = tileSet.tileIndex[index];
						uint8_t properties = tileSet.tileProperties[index];
						if (properties & TileSet::EnumFlip) {
							properties &= ~TileSet::EnumFlip;
							uint8_t outFlips = 0;
							if ((flip & RotateFlag)) {
								// if rotate flag and not allowed to rotate at least keep upper left corner in same direction
								if (!(properties & TileSet::CanRot)) {
									if (flip == RotateCW) { outFlips = TileLayer::TileFlipX; }
									else if (flip == RotateCCW) { outFlips = TileLayer::TileFlipY; }
								} else if (flip == RotateCW) { outFlips = TileLayer::TileRot; }
								else { outFlips = TileLayer::TileRot | TileLayer::TileFlipX | TileLayer::TileFlipY; }
							} else if (flip & (FlipY|FlipX)) {
								if ((flip & FlipX) && !(properties & TileSet::CanFlipX)) {
									if (properties & TileSet::CanRot) {
										outFlips = (flip&FlipY) ? (TileLayer::TileRot | TileLayer::TileFlipY ) : TileLayer::TileRot;
									} else { outFlips = (flip & FlipY) ? (TileLayer::TileFlipY) : TileLayer::NoFlip; }
								}
							} else if ((flip & FlipY) && !(properties & TileSet::CanFlipY)) {
								if (properties & TileSet::CanRot) {
									outFlips = (flip & FlipX) ? TileLayer::TileRot : (TileLayer::TileRot | TileLayer::TileFlipX);
								} else { outFlips = (flip & FlipX) ? (TileLayer::TileFlipX) : TileLayer::NoFlip; }
							} else {
								if (flip & FlipX) { outFlips |= TileLayer::TileFlipX; }
								if (flip & FlipY) { outFlips |= TileLayer::TileFlipY; }
							}
							// mask out unused bits, f.e. 4 -> (old & 3) | ((old>>1)&(~3))
							if (!(properties & TileSet::CanFlipX)) {
								outFlips = (outFlips & (TileSet::CanFlipX - 1)) || ((outFlips >> 1)& (~(TileSet::CanFlipX - 1)));
							}
							if (!(properties & TileSet::CanRot)) {
								outFlips = (outFlips & (TileSet::CanFlipY - 1)) || ((outFlips >> 1)& (~(TileSet::CanFlipY - 1)));
							}
							if (!(properties & TileSet::CanRot)) {
								outFlips = (outFlips & (TileSet::CanRot - 1)) || ((outFlips >> 1)& (~(TileSet::CanRot - 1)));
							}
							index += outFlips;
						}




						if (screen) { screen[slot] = index; }
					}
					break;
				}
			}
		}
	}

	// store the result in the layer
	layer.screen = screen;
}

// this stage happens before meta tile generation
bool MergeLayerData(TileMap &map, TileLayer &srcLayer, TileLayer &dstLayer)
{
	uint8_t* srcMap = srcLayer.screen;
	uint8_t* dstMap = srcLayer.type == TileLayer::CRAM ? dstLayer.color : dstLayer.screen;

	// check source and target exists
	if (!srcMap || !dstMap) { return false; }

	size_t shift = srcLayer.bitShift;
	uint8_t mask = ((1 << (srcLayer.bitLast+1)) - 1) << shift;
	uint8_t and = mask ^ 0xff;

	// all layers have the same size in Tiled so this shouldn't be an issue, can't merge different sized layers
	if (srcLayer.width != dstLayer.width || srcLayer.height != dstLayer.height) { return false; }

	size_t count = (size_t)srcLayer.width * (size_t)srcLayer.height;
	for (size_t slot = 0; slot < count; ++slot) {
		dstMap[slot] = (dstMap[slot] & and) | ((srcMap[slot] << shift) & mask);
	}
	return true;
}

// this stage happens after meta tile generation and gets applied to the meta map if it exists
bool MergeFlipData(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer)
{
	bool flipToCRAM = srcLayer.flipTarget.after(',').get_trimmed_ws().same_str("CRAM");
	uint8_t* srcMap = srcLayer.screen;
	uint8_t* dstMap = dstLayer.screen;
/*	if (dstLayer.metaMap) {
//		dstMap = dstLayer.metaMap;
		return true; // flips already applied to screen...
	} else*/ if (srcLayer.type == TileLayer::CRAM) {
		dstMap = dstLayer.color;
	}

	// check source and target exists
	if (!srcMap || !dstMap) { return false; }


	uint32_t numBits = 0;
	uint8_t flipX = 0, flipY = 0, rot = 0;
	if (srcLayer.flipType & TileLayer::TileFlipX) {
		flipX = 1 << (srcLayer.flipBitShift + numBits);
		++numBits;
	}
	if (srcLayer.flipType & TileLayer::TileFlipY) {
		flipY = 1 << (srcLayer.flipBitShift + numBits);
		++numBits;
	}
	if (srcLayer.flipType & TileLayer::TileRot) {
		rot = 1 << (srcLayer.flipBitShift + numBits);
		++numBits;
	}
	uint8_t mask = ((1 << numBits) - 1) << srcLayer.flipBitShift;
	size_t shift = srcLayer.bitShift;

	// all layers have the same size in Tiled so this shouldn't be an issue, can't merge different sized layers
	if (srcLayer.width != dstLayer.width || srcLayer.height != dstLayer.height) { return false; }

	size_t wid = dstLayer.width / dstLayer.metaX;
	size_t hgt = dstLayer.height / dstLayer.metaY;

	for (size_t y = 0; y < hgt; ++y) {
		for (size_t x = 0; x < wid; ++x) {
			uint8_t flip = srcLayer.flips[x * dstLayer.metaX + y * dstLayer.metaY * srcLayer.width];
			uint8_t fr = 0;
			if (flip == RotateCW) { fr |= rot; }
			else if (flip == RotateCCW) { fr |= rot | flipX | flipY; }
			else {
				if (flip & FlipX) { fr |= flipX; }
				if (flip & FlipY) { fr |= flipY; }
			}
			*dstMap = (*dstMap & (~mask)) | (flip & mask);
			++dstMap;
		}
	}
	return true;
}


// this stage happens before meta tile generation
bool MergeChars(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer)
{
	// check if either layer is missing and make sure the destination layer has the chars
	if (!srcLayer.chars) { return true; }
	if (!dstLayer.chars) {
		dstLayer.chars = srcLayer.chars;
		srcLayer.chars = nullptr;
		return true;
	}

	// fit both sets
	uint64_t* newChars = (uint64_t*)calloc(1, (srcLayer.numChars + dstLayer.numChars) * sizeof(uint64_t));
	memcpy(newChars, dstLayer.chars, dstLayer.numChars * sizeof(uint64_t));

	// make a conversion table
	uint8_t mapSrc[256] = {};

	size_t totalChars = dstLayer.numChars;

	bool overflow = false;

	// add chars missing in destination from source
	for (size_t c = 0, n = srcLayer.numChars; c < n; ++c) {
		uint64_t chr = srcLayer.chars[c];
		bool match = false;
		uint8_t matchIndex = 0;
		for (size_t d = 0; d < totalChars; ++d) {
			if (chr = newChars[d]) {
				match = true;
				matchIndex = (uint8_t)d;
				break;
			}
		}
		if (!match) {
			if (totalChars < 256) {
				matchIndex = (uint8_t)totalChars;
				newChars[totalChars++] = chr;
			} else {
				overflow = true;
			}
		}
		mapSrc[c] = matchIndex;
	}

	if (srcLayer.screen) {
		for (size_t slot = 0, n = (size_t)srcLayer.width * (size_t)srcLayer.height; slot < n; ++slot) {
			srcLayer.screen[slot] = mapSrc[srcLayer.screen[slot]];
		}
	}

	free(dstLayer.chars);
	free(srcLayer.chars);
	dstLayer.chars = (uint8_t*)newChars;
	dstLayer.numChars = (uint32_t)totalChars;
	srcLayer.numChars = 0;
	srcLayer.chars = nullptr;
	return !overflow;
}
