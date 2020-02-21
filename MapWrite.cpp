// Save the result of the map export

#include "struse\struse.h"
#include <vector>
#include "MapRead.h"
#include "MapWrite.h"
#include "MergeMap.h"

uint8_t* PackBits(uint8_t* orig, size_t size, size_t bits, size_t *outSize)
{
	size_t newSize = (size * bits + 7) / 8;
	uint8_t *dest = (uint8_t*)calloc(1, newSize);
	uint16_t mask = uint16_t((1 << bits) - 1);
	size_t trgBit = 0;
	int bitFromTop = 7 - (int)bits + 1;

	if (!dest) { return dest; }

	// strore bits left to right, so start with highest bit at bit 7 of first byte
	// trgBit=0 => shift lowest bit = 8-bits
	for (uint8_t* end = orig + size; orig < end; ++orig) {
		uint8_t *trg = dest + (trgBit >> 3);
		// top bit = 7-(trgBit&7), bottom bit = 7-(trgBit&7)-(bits-1)
		int shift = bitFromTop - (int)(trgBit & 7);
		uint16_t byte = (*orig) & mask;

		// if shift < 0 then write to two bytes
		if (shift < 0) {
			trg[0] |= (byte >> -shift);
			trg[1] = trg[1] | (byte<<(shift+8));
		} else {
			*trg = *trg | (byte << shift);
		}
		trgBit += bits;
	}
	if (outSize) { *outSize = newSize; }
	return dest;
}

uint8_t* UnpackBits(uint8_t* packed, size_t count, size_t bits)
{
	uint8_t *dest = (uint8_t*)calloc(1, count);
	uint8_t mask = uint8_t((1 << bits) - 1);
	size_t srcBit = 0;

	if (!dest) { return dest; }

	// strore bits left to right, so start with highest bit at bit 7 of first byte
	// trgBit=0 => shift lowest bit = 8-bits
	for (size_t i = 0; i < count; ++i) {
		uint8_t *src = packed + (srcBit >> 3);
		int shift = (int)((~srcBit) & 7) - ((int)bits - 1);
		uint8_t byte = 0;
		uint8_t m = mask;

		// if shift < 0 then read from two bytes
		if (shift < 0) {
			byte = ((src[0] << (-shift))&mask) | (src[1] >> (shift+8));
		} else {
			byte = (*src >> shift) & mask;
		}
		dest[i] = byte;
		srcBit += bits;
	}
	free(packed);
	return dest;
}

FILE* OpenLog(strref path, strref file, strref ext)
{
	strown<_MAX_PATH> merge(path);
	merge.append(file);
	merge.append(ext);
	merge.cleanup_path();
	FILE* flog;
	if (fopen_s(&flog, merge.c_str(), "w")==0 && flog != nullptr) {
		return flog;
	}
	return nullptr;
}

bool WriteMapData(TileMap& map, const char* mapFilename)
{
	strref mapFilestr(mapFilename);
	strref path = mapFilestr.get_substr(0, mapFilestr.find_last('/', '\\') + 1);

	for (std::vector<TileLayer>::iterator layer = map.layers.begin(); layer != map.layers.end(); ++layer) {
		if (strref exp = layer->exportFile) {
			size_t metaTileSize = layer->metaX * layer->metaY;
			if (FILE* flog = OpenLog(path, layer->exportFile, ".log")) {
				fprintf(flog, "width = %d\nheight = %d\nmeta = %d\n", layer->width, layer->height, layer->metaMap ? 1 : 0);
				fprintf(flog, "screenBits = %d\ncolorBits = %d\n", layer->screenBits ? layer->screenBits : 8, layer->colorBits ? layer->colorBits : 8);
				fprintf(flog, "hasColor = %d\n", layer->color ? 1 : 0);
				fprintf(flog, "hasChars = %d\n", layer->numChars ? 1 : 0);

				if (layer->metaMap) {
					fprintf(flog, "metaLookup = %d\n", layer->metaLookupMap ? 1 : 0);
					fprintf(flog, "metaX = %d\nmetaY = %d\nmetaMapBits = %d\n", layer->metaX, layer->metaY, layer->metaMapBits ? layer->metaMapBits : 8);
					fprintf(flog, "numMetaIndex = %d\nnumMetaColor = %d\nnumMetaLookup = %d\n",
							layer->numMetaIndex, layer->numMetaColor, layer->numMeta);
					if (layer->metaLookupMap) {
						fprintf(flog, "metaLookupBits = %d\n", layer->metaLookupBits ? layer->metaLookupBits : 8);
					}
				}
				fclose(flog);
			}
			if (layer->chars && layer->numChars) {
				WriteData(path, layer->exportFile, ".chr", layer->chars, layer->numChars * 8);
			}
			if (layer->metaLookupMap && layer->numMeta) {
				WriteDataBits(path, layer->exportFile, ".mls", layer->metaLookupMap, layer->numMeta, layer->metaLookupBits);
			}
			if (layer->metaLookupColor && layer->numMeta) {
				WriteDataBits(path, layer->exportFile, ".mlc", layer->metaLookupColor, layer->numMeta, layer->metaLookupBits);
			}
			if (layer->metaTileIndex && layer->numMetaIndex) {
				WriteDataBits(path, layer->exportFile, ".mts", layer->metaTileIndex, layer->numMetaIndex * metaTileSize, layer->screenBits);
			}
			if (layer->metaTileColor && layer->numMetaColor) {
				WriteDataBits(path, layer->exportFile, ".mtc", layer->metaTileColor, layer->numMetaColor * metaTileSize, layer->colorBits);
			}
			if (layer->metaMap) {
				WriteDataBits(path, layer->exportFile, ".mta", layer->metaMap,
					(layer->width / layer->metaX) * (layer->height / layer->metaY), layer->metaMapBits);
			} else {
				if (layer->screen) {
					WriteDataBits(path, layer->exportFile, ".scr", layer->screen, layer->width * layer->height, layer->screenBits);
				}
				if (layer->color) {
					WriteDataBits(path, layer->exportFile, ".col", layer->color, layer->width * layer->height, layer->colorBits);
				}
			}
		}
	}
	return true;
}

uint32_t BitSize(uint32_t bytes, size_t bits)
{
	return uint32_t(bits ? (bytes * bits + 7) / 8 : bytes);
}

// manifest file
//	* list of layers
//	 - list of files for each layer
//	* total counts & sizes

bool WriteMapStats(TileMap& map, const char* mapFilename)
{
	if (!map.stats) { return false; }

	// file is path relative to map folder
	strref mapFilestr(mapFilename);
	strref path = mapFilestr.get_substr(0, mapFilestr.find_last('/', '\\') + 1);
	strown<_MAX_PATH> merge(path);
	merge.append(map.stats).cleanup_path();

	// path from root dir to stats file
	strref rel = map.stats.get_substr(0, mapFilestr.find_last('/', '\\') + 1);

	FILE* f;
	if (!fopen_s(&f, merge.c_str(), "w") && f != nullptr) {
		strown<1024> line;

		fprintf(f, "# Export Summary For Map Export #\n#\n# TILESETS\n");
		for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
			fprintf(f, "# * " STRREF_FMT " (" STRREF_FMT "): %d - %d\n",
					STRREF_ARG(tileset->xmlName), STRREF_ARG(tileset->imageName), tileset->first, tileset->first + tileset->count);
		}

		fprintf(f, "#\n# LAYERS\n");
		for (std::vector<TileLayer>::iterator layer = map.layers.begin(); layer != map.layers.end(); ++layer) {
			fprintf(f, "# * " STRREF_FMT ": ", STRREF_ARG(layer->name));
			switch (layer->type) {
				case TileLayer::Text: fprintf(f, "Text,"); break;
				case TileLayer::TextMC: fprintf(f, "Text Multicolor,"); break;
				case TileLayer::ECBM: fprintf(f, "ECBM,"); break;
				case TileLayer::Bitmap: fprintf(f, "Bitmap hires,"); break;
				case TileLayer::BitmapMC: fprintf(f, "Bitmap multicolor,"); break;
				case TileLayer::Bits: fprintf(f, "Data,"); break;
				case TileLayer::CRAM: fprintf(f, "CRAM Data,"); break;
				default: fprintf(f, "???,"); break;
			}
			if (layer->exportFile) { fprintf(f, " => " STRREF_FMT, STRREF_ARG(layer->exportFile)); }
			if (layer->target) { fprintf(f, " (data => " STRREF_FMT ", bits %d-%d)", STRREF_ARG(layer->target), layer->bitShift, layer->bitLast); }
			if (layer->mergeChars) { fprintf(f, " (chars merged to " STRREF_FMT ")", STRREF_ARG(layer->mergeChars)); }
			if (layer->flipTarget) { fprintf(f, " (flips => " STRREF_FMT ")", STRREF_ARG(layer->flipTarget)); }
			if (layer->metaX > 1 || layer->metaY > 1) { fprintf(f, " Meta Tiles: %d x %d", layer->metaX, layer->metaY); }
			fprintf(f, "\n");
		}

		fprintf(f, "#\n# FILES\n");
		for (std::vector<TileLayer>::iterator layer = map.layers.begin(); layer != map.layers.end(); ++layer) {
			if (strref exp = layer->exportFile) {
				uint32_t total = layer->chars ? layer->numChars * 8 : 0;
				uint32_t metaMap;
				if (layer->metaMap) {
					// 1 byte per meta tile
					metaMap = (layer->width / layer->metaX) * (layer->height / layer->metaY);
					total += BitSize(metaMap, layer->metaMapBits);
					if (layer->metaLookupColor) {
						// color + data lookup tiles
						total += layer->numMeta * 2 +
							BitSize(layer->numMetaIndex * layer->metaX * layer->metaY, layer->screenBits) +
							BitSize(layer->numMetaColor * layer->metaX * layer->metaY, layer->colorBits);
					} else {
						// just data, no lookup tiles
						total += BitSize(layer->numMetaIndex * layer->metaX * layer->metaY, layer->screenBits);
					}
				} else {
					// just screen indices for screen & color
					total += BitSize(layer->width * layer->height, layer->screenBits) + BitSize(layer->width * layer->height, layer->colorBits);
				}
				fprintf(f, "# * " STRREF_FMT ": %d bytes\n", STRREF_ARG(layer->name), total);
				if (layer->chars && layer->numChars) {
					fprintf(f, STRREF_FMT ".chr ; %d chars ($%x bytes)\n", STRREF_ARG(exp), layer->numChars, layer->numChars * 8);
				}
				if (layer->metaMap) {
					fprintf(f, STRREF_FMT ".mta ; meta indices, %d meta tiles ($%x bytes)\n", STRREF_ARG(exp), metaMap, BitSize(metaMap, layer->metaMapBits));
					if (layer->metaLookupMap) {
						fprintf(f, STRREF_FMT ".mls ; meta screen lookup tiles, %d indices ($%x bytes)\n", STRREF_ARG(exp), layer->numMetaIndex, BitSize(layer->numMetaIndex, layer->metaLookupBits));
					}
					if (layer->metaLookupColor) {
						fprintf(f, STRREF_FMT ".mlc ; meta color lookup tiles, %d indices ($%x bytes)\n", STRREF_ARG(exp), layer->numMetaColor, BitSize(layer->numMetaColor, layer->metaLookupBits));
					}
					if (layer->metaTileIndex) {
						fprintf(f, STRREF_FMT ".mts ; %d meta screen tiles ($%x bytes)\n",
								STRREF_ARG(exp), layer->numMetaIndex, BitSize(layer->metaX*layer->metaY*layer->numMetaIndex, layer->screenBits));
					}
					if (layer->metaTileColor) {
						fprintf(f, STRREF_FMT ".mtc ; %d meta color tiles ($%x bytes)\n",
								STRREF_ARG(exp), layer->numMetaColor, BitSize(layer->metaX*layer->metaY*layer->numMetaColor, layer->colorBits));
					}
				} else {
					fprintf(f, STRREF_FMT ".scr ; screen data %d x %d ($%x bytes)\n", STRREF_ARG(exp), layer->width, layer->height, BitSize(layer->width * layer->height, layer->screenBits));
					fprintf(f, STRREF_FMT ".col ; color data %d x %d ($%x bytes)\n", STRREF_ARG(exp), layer->width, layer->height, BitSize(layer->width * layer->height, layer->colorBits));
				}
			}
		}
		fclose(f);
		return true;
	}
	return false;
}

// no meta file => save chars/screen/color

// meta file