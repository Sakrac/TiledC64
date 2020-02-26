// Merge exported maps so that each individual map has meta tiles but
// shares chars, meta tiles, meta lookup. determine what to do based on which files exist
// selection of file types must match between all source maps

#include <vector>
#include "struse\struse.h"
#include "MapRead.h"
#include "MapWrite.h"

uint8_t* LoadData(const char* filename, size_t &size)
{
	FILE* f;
	if (fopen_s(&f, filename, "rb") == 0 && f != nullptr) {
		void* data;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		data = calloc(1, size);
		if (data) { fread(data, size, 1, f); }
		fclose(f);
		return (uint8_t*)data;
	}
	return nullptr;
}

uint8_t* LoadData(strref path, strref file, strref ext, size_t &size)
{
	strown<_MAX_PATH> merge(path);
	merge.append(file);
	merge.append(ext);
	merge.cleanup_path();
	return LoadData(merge.c_str(), size);
}

uint8_t* LoadDataBits(strref path, strref file, strref ext, size_t &size, size_t bits, size_t unpackSize )
{
	uint8_t* data = LoadData(path, file, ext, size);
	if (!data) { return nullptr; }
	if (bits && bits != 8) {
		if (size != (unpackSize * bits + 7) / 8) {
			free(data);
			return nullptr;
		}
		uint8_t *unpacked = UnpackBits(data, unpackSize, bits);
		size = unpackSize;
		return unpacked;
	}
	return data;
}

bool WriteData(const char* filename, void* data, size_t size)
{
	FILE* f;
	if (fopen_s(&f, filename, "wb") == 0 && f != nullptr) {
		fwrite(data, size, 1, f);
		fclose(f);
		return true;
	}
	return false;
}

bool WriteData(strref path, strref file, strref ext, void *data, size_t size)
{
	strown<_MAX_PATH> merge(path);
	merge.append(file);
	merge.append(ext);
	merge.cleanup_path();
	return WriteData(merge.c_str(), data, size);
}

bool WriteDataBits(strref path, strref file, strref ext, void *data, size_t size, size_t bits)
{
	if (bits && bits != 8) {	// 0 bits => 8 bits
		size_t packedBits = 0;
		if (uint8_t *packed = PackBits((uint8_t*)data, size, bits, &packedBits)) {
			bool ret = WriteData(path, file, ext, (void*)packed, packedBits);
			free(packed);
			return ret;
		}
		return false;
	}
	return WriteData(path, file, ext, data, size);
}

uint8_t* UnpackFile(uint8_t *packed, size_t &packSize, size_t unpackSize, size_t packBits)
{
	if (packBits == 8 && packSize == unpackSize) { return packed; }
	if (packSize != (unpackSize * packBits + 7) / 8) {
		free(packed);
		return nullptr;
	}
	uint8_t *unpacked = UnpackBits(packed, unpackSize, packBits);
	packSize = unpackSize;
	free(packed);
	return unpacked;
}

bool MergeMaps(const char *sourceListFile, const char *targetFile)
{
	size_t sourceSize;
	if (char* sources = LoadText(sourceListFile, sourceSize)) {
		// first file determines valid files
		bool hasChars = true;
		bool hasMeta = true;
		bool hasMetaLookup = true;
		bool hasMetaScreen = true;
		bool hasMetaColor = true;
		bool hasScreen = true;
		bool hasColor = true;


		// file is path relative to source list folder
		strref mapFilestr(sourceListFile);
		strref path = mapFilestr.get_substr(0, mapFilestr.find_last('/', '\\') + 1);

		strref src((const char*)sources, (strl_t)sourceSize);
		bool first = true;
		bool success = true;

		size_t metaTileSize = 1;

		uint64_t *mergedChars = (uint64_t*)calloc(1, 256 * 8);
		uint8_t *mergedTileIndex = (uint8_t*)calloc(1, 256 * 256);
		uint8_t *mergedTileColor = (uint8_t*)calloc(1, 256 * 256);
		uint8_t mergedLookupIndex[256];
		uint8_t mergedLookupColor[256];

		size_t numChars = 0;
		size_t numLookup = 0;
		size_t numMetaTileScreen = 0;
		size_t numMetaTileColor = 0;
		size_t metaX = 1, metaY = 1;

		size_t screenBits = 8, colorBits = 8, metaMapBits = 8, metaLookupBits = 8;

		while (strref target = src.line()) {
			
			target.trim_whitespace();
			strref source = target.split_token_trim(':');

			size_t charSize = 0;
			uint64_t *charData = nullptr;
			size_t metaSize = 0;
			uint8_t *metaData = nullptr;
			size_t metaLookupIndexSize = 0;
			size_t metaLookupColorSize = 0;
			uint8_t *metaLookupIndex = nullptr;
			uint8_t *metaLookupColor = nullptr;
			size_t metaTileIndexSize = 0;
			uint8_t *metaTileIndex = nullptr;
			size_t metaTileColorSize = 0;
			uint8_t *metaTileColor = nullptr;
			size_t screenSize = 0;
			uint8_t *screen = nullptr;
			size_t colorSize = 0;
			uint8_t *color = nullptr;

			size_t wid = 40, hgt = 40;
			size_t numMetaLookup = 0;
			size_t numMetaIndex = 0;
			size_t numMetaColor = 0;
			size_t layerNumLookup = 0;

			{
				size_t logSize = 0;
				if (uint8_t *logData = LoadData(path, source, (".log"), logSize)) {
					strref log((const char*)logData, (strl_t)logSize);
					while (strref line = log.line()) {
						line.trim_whitespace();
						strref param = line.split_token_trim('=');
						if (param.same_str("width")) { wid = (size_t)line.atoi(); }
						else if (param.same_str("height")) { hgt = (size_t)line.atoi(); }
						else if (param.same_str("meta")) {
							if (first) { hasMeta = !!line.atoi(); hasScreen = !hasMeta; }
							else if (hasMeta != !!line.atoi()) { success = false; }
						} else if (param.same_str("screenBits")) {
							if (first) { screenBits = (size_t)line.atoi(); }
							else if (screenBits != line.atoi()) { success = false; }
						} else if (param.same_str("colorBits")) {
							if (first) { colorBits = (size_t)line.atoi(); }
							else if (colorBits != line.atoi()) { success = false; }
						}  else if (param.same_str("hasColor")) {
							if (first) { hasColor = !!line.atoi(); }
							else if (hasColor != !!line.atoi()) { success = false; }
						}  else if (param.same_str("hasChars")) {
							if (first) { hasChars = !!line.atoi(); }
							else if (hasChars != !!line.atoi()) { success = false; }
						}  else if (param.same_str("metaLookup")) {
							if (first) { hasMetaLookup = !!line.atoi(); }
							else if (hasMetaLookup != !!line.atoi()) { success = false; }
						} else if (param.same_str("metaX")) {
							if (first) { metaX = (size_t)line.atoi(); }
							else if (metaX != line.atoi()) { success = false; }
						} else if (param.same_str("metaY")) {
							if (first) { metaY = (size_t)line.atoi(); }
							else if (metaY != line.atoi()) { success = false; }
						} else if (param.same_str("metaMapBits")) {
							if (first) { metaMapBits = (size_t)line.atoi(); }
							else if (metaMapBits != line.atoi()) { success = false; }
						} else if (param.same_str("metaLookupBits")) {
							if (first) { metaLookupBits = (size_t)line.atoi(); }
							else if (metaLookupBits != line.atoi()) { success = false; }
						} else if (param.same_str("numMetaIndex")) {
							numMetaIndex = (size_t)line.atoi();
						} else if (param.same_str("numMetaColor")) {
							numMetaColor = (size_t)line.atoi();
						} else if (param.same_str("numMetaLookup")) {
							layerNumLookup = (size_t)line.atoi();
						}
					}
					free(logData);
				} else { success = false; }
			}

			if (first) { metaTileSize = metaX * metaY; }

			size_t metaWid = wid / metaX;
			size_t metaHgt = hgt / metaY;

			if (!success) {
				printf("problem with layer " STRREF_FMT "\n", STRREF_ARG(source));
				break;
			}

			if (hasChars) {
				charData = (uint64_t*)LoadData(path, source, strref(".chr"), charSize);
				if (!charData) {
					printf("could not read chars for map " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasMeta) {
				metaData = LoadDataBits(path, source, strref(".mta"), metaSize, metaMapBits, metaWid * metaHgt);
				if (!metaData) {
					printf("could not read meta map for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasMeta && hasMetaLookup) {
				metaLookupIndex = LoadDataBits(path, source, strref(".mls"), metaLookupIndexSize, metaLookupBits, layerNumLookup);
				metaLookupColor = LoadDataBits(path, source, strref(".mlc"), metaLookupColorSize, metaLookupBits, layerNumLookup);
				if (!metaLookupIndex || !metaLookupColor) {
					printf("could not read meta lookup table for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasMetaScreen) {
				metaTileIndex = LoadDataBits(path, source, strref(".mts"), metaTileIndexSize, screenBits, numMetaIndex * metaX * metaY);
				if (!metaTileIndex) {
					printf("could not read meta screen tiles for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasMetaLookup && hasMetaColor) {
				metaTileColor = LoadDataBits(path, source, strref(".mtc"), metaTileColorSize, colorBits, numMetaIndex * metaX * metaY);
				if (!metaTileColor) {
					printf("could not read meta color tiles for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasScreen) {
				screen = LoadDataBits(path, source, strref(".scr"), screenSize, screenBits, wid * hgt);
				if (!screen) {
					printf("could not read screen for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}
			if (hasScreen && hasColor) {
				color = LoadDataBits(path, source, strref(".col"), colorSize, colorBits, wid * hgt);
				if (!color) {
					printf("could not read color for " STRREF_FMT "\n", STRREF_ARG(source)); success = false;
				}
			}

			// first insert chars
			uint8_t remapChars[256] = {};
			for (size_t c = 0, n = charSize / 8; c < n; ++c) {
				int match = -1;
				for (size_t d = 0; d < numChars; ++d) {
					if (charData[c] == mergedChars[d]) {
						match = (int)d;
						break;
					}
				}
				if (match < 0) {
					if (numChars < 256) {
						match = (int)numChars;
						mergedChars[numChars++] = charData[c];
					} else {
						match = 0;
						success = false;	// too many chars!
					}
				}
				remapChars[c] = (uint8_t)match;
			}
			{	// replace chars in screen or meta tiles
				uint8_t indexMask = 0;
				while (numChars && indexMask < (numChars - 1)) { indexMask = (indexMask << 1) | 1; }
				size_t numCharIndices = hasMetaScreen ? metaTileIndexSize : screenSize;
				uint8_t *charIndices = hasMetaScreen ? metaTileIndex : screen;
				for (size_t i = 0; i < numCharIndices; ++i) {
					charIndices[i] = (charIndices[i] & (~indexMask)) | remapChars[charIndices[i] & indexMask];
				}
			}

			// insert meta tiles..
			if (hasMetaScreen && metaTileIndex) {
				uint8_t remapTileScreen[256] = {};
				for (size_t t = 0, n = metaTileIndexSize / metaTileSize; t < n; ++t) {
					int match = -1;
					uint8_t *loaded = metaTileIndex + t * metaTileSize;
					uint8_t *check = mergedTileIndex;
					for (size_t u = 0; u < numMetaTileScreen; ++u) {
						if (memcmp(loaded, check, metaTileSize) == 0) {
							match = (int)u;
							break;
						}
						check += metaTileSize;
					}
					if (match < 0) {
						if (numMetaTileScreen < 256) {
							memcpy(mergedTileIndex + numMetaTileScreen * metaTileSize, loaded, metaTileSize);
							match = (int)numMetaTileScreen;
							++numMetaTileScreen;
						} else {
							match = 0;
							success = false;
						}
					}
					remapTileScreen[t] = (uint8_t)match;
				}
				if (hasMeta) {
					uint8_t indexMask = 0;
					while (numMetaTileScreen && indexMask < (numMetaTileScreen - 1)) { indexMask = (indexMask << 1) | 1; }
					if (hasMetaLookup) {
						if (metaLookupIndex) {
							for (size_t i = 0; i < metaLookupIndexSize; ++i) {
								metaLookupIndex[i] = (metaLookupIndex[i] & (~indexMask)) | remapTileScreen[metaLookupIndex[i] & indexMask];
							}
						}
					} else if( metaData ) {
						for (size_t i = 0; i < metaSize; ++i) {
							metaData[i] = (metaData[i] & (~indexMask)) | remapTileScreen[metaData[i] & indexMask];
						}
					}
				}
			}

			// insert meta color tiles..
			if (hasMetaColor && metaTileColor) {
				uint8_t remapTileColor[256] = {};
				for (size_t t = 0, n = metaTileColorSize / metaTileSize; t < n; ++t) {
					int match = -1;
					uint8_t *loaded = metaTileColor + t * metaTileSize;
					uint8_t *check = mergedTileColor;
					for (size_t u = 0; u < numMetaTileColor; ++u) {
						if (memcmp(loaded, check, metaTileSize) == 0) {
							match = (int)u;
							break;
						}
						check += metaTileSize;
					}
					if (match < 0) {
						if (numMetaTileColor < 256) {
							memcpy(mergedTileColor + numMetaTileColor * metaTileSize, loaded, metaTileSize);
							match = (int)numMetaTileColor;
							++numMetaTileColor;
						} else {
							match = 0;
							success = false;
						}
					}
					remapTileColor[t] = (uint8_t)match;
				}
				if (hasMeta && metaLookupColor) {
					uint8_t indexMask = 0;
					while (numMetaTileColor && indexMask < (numMetaTileColor - 1)) { indexMask = (indexMask << 1) | 1; }
					if (hasMetaLookup) {
						if (metaLookupIndex) {
							for (size_t i = 0; i < metaLookupColorSize; ++i) {
								metaLookupColor[i] = (metaLookupColor[i] & (~indexMask)) | remapTileColor[metaLookupColor[i] & indexMask];
							}
						}
					}
				}
			}

			// now apply the meta lookup table to the merged table
			if (hasMetaLookup && metaLookupColor && metaLookupIndex && metaData) {
				uint8_t remapLookup[256] = {};
				for (size_t l = 0; l < metaLookupIndexSize; ++l) {
					uint8_t idx = metaLookupIndex[l];
					uint8_t col = metaLookupColor[l];
					int match = -1;
					for (size_t l2 = 0; l2 < numLookup; ++l2) {
						if (idx == mergedLookupIndex[l2] && col == mergedLookupColor[l2]) {
							match = (int)l2;
							break;
						}
					}
					if (match < 0) {
						if (numLookup < 256) {
							mergedLookupIndex[numLookup] = idx;
							mergedLookupColor[numLookup] = col;
							match = (int)numLookup;
							++numLookup;
						} else {
							printf("Too many meta tiles for indexing\n");
							success = false;
						}
					}
					remapLookup[l] = (uint8_t)match;
				}
				uint8_t indexMask = 0;
				while (numChars && indexMask < (numLookup - 1)) { indexMask = (indexMask << 1) | 1; }
				for (size_t m = 0; m < metaSize; ++m) {
					metaData[m] = (metaData[m] & (~indexMask)) | remapLookup[metaData[m] & indexMask];
				}
			}

			// each map still needs:
			// * meta tile map
			// or
			// * screen
			// * color

			//WriteDataBits(strref path, strref file, strref ext, void *data, size_t size, size_t bits)

			if (hasMeta && metaData) {
				WriteDataBits(path, target, ".mta", metaData, metaSize, screenBits);
			} else {
				if (hasScreen && screen) {
					WriteDataBits(path, target, ".scr", screen, screenSize, screenBits);
				}
				if (hasColor && color) {
					WriteDataBits(path, target, ".col", color, colorSize, colorBits);
				}
			}

			if (charData) { free(charData); }
			if (metaData) { free(metaData); }
			if (metaLookupIndex) { free(metaLookupIndex); }
			if (metaLookupColor) { free(metaLookupColor); }
			if (screen) { free(screen); }
			if (color) { free(color); }

			if (!success) { break; }

			first = false;
		}

		// shared files 
		// * chars
		// * meta lookup index
		// * meta lookup color
		// * meta index
		// * meta color
		if (numChars) {
			WriteData(strref(), strref(targetFile), strref(".chr"), mergedChars, 8 * numChars);
		}
		if (numLookup) {
			if (mergedLookupIndex) {
				WriteDataBits(strref(), strref(targetFile), strref(".mls"), mergedLookupIndex, numLookup, metaLookupBits);
			}
			if (mergedLookupColor) {
				WriteDataBits(strref(), strref(targetFile), strref(".mlc"), mergedLookupColor, numLookup, metaLookupBits);
			}
		}
		if (numMetaTileScreen) {
			WriteDataBits(strref(), strref(targetFile), strref(".mts"), mergedTileIndex, numMetaTileScreen * metaTileSize, screenBits);
		}
		if (numMetaTileColor) {
			WriteDataBits(strref(), strref(targetFile), strref(".mtc"), mergedTileColor, numMetaTileColor * metaTileSize, colorBits);
		}
		return true;
	}
	return false;
}