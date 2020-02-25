#include <vector>
#include "struse.h"
#include "samples\xml.h"
#include "MapRead.h"

static const char* sLayerTypeNames[] = {
	"Text",
	"TextMC",
	"ECBM",
	"Bitmap",
	"BitmapMC",
};

static const int nLayerTypeNames = sizeof(sLayerTypeNames) / sizeof(sLayerTypeNames[0]);

char* LoadText(const char* filename, size_t& size)
{
	FILE* f;
	if (fopen_s(&f, filename, "r") == 0 && f != nullptr) {
		void* data;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		data = calloc(1, size);
		if (data) { fread(data, size, 1, f); }
		fclose(f);
		return (char*)data;
	}
	return nullptr;
}

TileSet* GetTilesetFromTile(TileMap& map, uint32_t tile)
{
	tile &= 0xfffffff;
	if (!tile) { return nullptr; }
	for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
		if (tile >= tileset->first && tile < (tileset->first + tileset->count)) { return &*tileset; }
	}
	return nullptr;
}

bool ReadMapXML(void* user, strref tag_or_data, const strref* tag_stack, int depth, XML_TYPE type)
{
	TileMap* map = (TileMap*)user;
	if (type == XML_TYPE_TAG_OPEN || type == XML_TYPE_TAG_SELF_CLOSE) {
		strref tag = tag_or_data.get_word();
		if (tag.same_str("map")) {
			map->tile_wid = (int)XMLFindAttr(tag_or_data, strref("tilewidth")).atoi()>>3;
			map->tile_hgt = (int)XMLFindAttr(tag_or_data, strref("tileheight")).atoi()>>3;
			map->width = (int)XMLFindAttr(tag_or_data, strref("width")).atoi();
			map->height = (int)XMLFindAttr(tag_or_data, strref("height")).atoi();
		} else if (tag.same_str("tileset")) {
			map->tileSets.push_back(TileSet());
			TileSet* tileset = &map->tileSets[map->tileSets.size() - 1];
			tileset->first = (int)XMLFindAttr(tag_or_data, strref("firstgid")).atoi();
			tileset->xmlName = XMLFindAttr(tag_or_data, strref("source"));
		} else if (tag.same_str("layer")) {
			map->layers.push_back(TileLayer());
			map->currLayer = (int)(map->layers.size() - 1);
			TileLayer* layer = &map->layers[map->currLayer];
			layer->id = (int)XMLFindAttr(tag_or_data, strref("id")).atoi();
			layer->width = (int)XMLFindAttr(tag_or_data, strref("width")).atoi();
			layer->height = (int)XMLFindAttr(tag_or_data, strref("height")).atoi();
			layer->name = XMLFindAttr(tag_or_data, strref("name"));
			layer->metaX = map->tile_wid >> 3;	// initialize meta tile size with layer tile char counts
			layer->metaY = map->tile_hgt >> 3;
			layer->screenBits = 8;
			layer->colorBits = 8;
		} else if (depth >= 2 && tag.same_str("property")) {
			strref parent = tag_stack[1].get_word();
			strref name = XMLFindAttr(tag_or_data, strref("name"));
			strref value = XMLFindAttr(tag_or_data, strref("value")).get_trimmed_ws();
			if (parent.same_str("layer")) {
				TileLayer* layer = &map->layers[map->layers.size() - 1];
				if (name.same_str("type")) {
					if (value.grab_prefix("CRAM")) {
						layer->type = TileLayer::CRAM;
						value.skip_whitespace();
						layer->bitShift = 4;
						layer->bitLast = 7;
						if (value) {
							layer->bitShift = value.atoi_skip(); value.trim_whitespace();
							if (value[0] == ',' || value[0] == '-') { ++value; value.skip_whitespace(); }
							layer->bitLast = value ? (uint32_t)value.atoi() : 7;
						}
					} else if (value.grab_prefix("Bits")) {
						layer->type = TileLayer::Bits;
						value.skip_whitespace();
						layer->bitShift = 4;
						layer->bitLast = 7;
						if (value) {
							layer->bitShift = value.atoi_skip(); value.trim_whitespace();
							if (value[0] == ',' || value[0] == '-') { ++value; value.skip_whitespace(); }
							layer->bitLast = value ? (uint32_t)value.atoi() : 7;
						}
					} else {
						int type;
						for (type = nLayerTypeNames - 1; type; --type) {
							if (value.same_str(sLayerTypeNames[type])) { break; }
						}
						layer->type = (TileLayer::Type)type;
					}
				} else if (name.same_str("target")) {
					layer->target = value.get_trimmed_ws();
				} else if (name.same_str("export")) {
					layer->exportFile = value.get_trimmed_ws();
				} else if (name.same_str("mergeChars")) {
					layer->mergeChars = value.get_trimmed_ws();
				} else if (name.same_str("ScreenBits")) {
					layer->screenBits = (int)value.get_trimmed_ws().atoi();
				} else if (name.same_str("ColorBits")) {
					layer->colorBits = (int)value.get_trimmed_ws().atoi();
				} else if (name.same_str("MetaMapBits")) {
					layer->metaMapBits = (int)value.get_trimmed_ws().atoi();
				} else if (name.same_str("MetaLookupBits")) {
					layer->metaLookupBits = (int)value.get_trimmed_ws().atoi();
				} else if (name.same_str("FlipSource")) {
					layer->flipSource = value.get_trimmed_ws();
				} else if (name.same_str("FlipTarget")) {
					layer->flipTarget = value.split_token_trim(',');
					layer->flipBitShift = value.atoi_skip();
					while (strref arg = value.split_token_trim(',')) {
						if (arg.same_str("X")) { layer->flipType |= TileLayer::TileFlipX; }
						if (arg.same_str("Y")) { layer->flipType |= TileLayer::TileFlipY; }
						if (arg.same_str("Rot")) { layer->flipType |= TileLayer::TileRot; }
						if (arg.same_str("CRAM")) { layer->flipType |= TileLayer::ToCRAM; }
					}
				} else if (name.same_str("bg")) {
					int i = 0;
					while (strref col = value.split_token(',')) {
						col.trim_whitespace();
						layer->bg[i++] = (char)col.atoi();
						if (i >= 4) { break; }
					}
				} else if (name.same_str("meta")) {
					layer->metaX = (uint32_t)value.split_token_trim(',').atoi();
					layer->metaY = (uint32_t)value.atoi();
				}
			} else if (parent.same_str("map")) {
				if (name.same_str("stats")) {
					map->stats = value.get_trimmed_ws();
				}
			}
		} else {
			printf(STRREF_FMT "\n", STRREF_ARG(tag_or_data));
		}
	} else if (type == XML_TYPE_TEXT && depth>1) {
		TileLayer &layer = map->layers[map->currLayer];
		if (layer.map) { free(layer.map); }
		size_t count = layer.width * layer.height;
		layer.map = (uint32_t*)calloc(1, count * sizeof(int));
		size_t index = 0;
		if (tag_stack[0].get_word().same_str("data") && tag_stack[1].get_word().same_str("layer")) {
			while( strref value = tag_or_data.split_token(',')) {
				layer.map[index++] = (uint32_t)value.atoi();
				if (index >= count) { break; }
			}
		}
	}
	return true;
}

void MakeRoomForTiles(TileMap& map, uint32_t oldFirst, uint32_t newFirst)
{
	// update all layers
	for (std::vector<TileLayer>::iterator layer = map.layers.begin(); layer != map.layers.end(); ++layer) {
		size_t count = layer->width * layer->height;
		for (size_t slot = 0; slot < count; ++slot) {
			uint32_t tile = layer->map[slot] & 0xfffffff;
			if (tile >= oldFirst) {
				tile += (newFirst - oldFirst);
				tile |= layer->map[slot] & 0xf0000000;
				layer->map[slot] = tile;
			}
		}
	}
	// update all tilesets
	for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
		if (tileset->first > oldFirst) {
			tileset->first += newFirst - oldFirst;
		}
	}
}

// convert all tilesets & layers to 8x8 cells
void ExplodeTiles(TileMap& map)
{
	// first rebuild all tilesets, the layers will reference the top left char of each tile
	for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {
		uint32_t set_wid = tileset->width * tileset->tile_wid;
		uint32_t set_hgt = tileset->height * tileset->tile_hgt;
		uint32_t set_count = set_wid * set_hgt;
		if (set_count > tileset->count) {
			MakeRoomForTiles(map, tileset->first + tileset->count, tileset->first + set_count);
			tileset->count = set_count;
			tileset->tile_wid = 1;
			tileset->tile_hgt = 1;
		}
	}

	// now apply to all layers
	if (map.tile_wid > 1 || map.tile_hgt > 1) {
		for (std::vector<TileLayer>::iterator layer = map.layers.begin(); layer != map.layers.end(); ++layer) {
			uint32_t layer_wid = layer->width * map.tile_wid;
			uint32_t layer_hgt = layer->height * map.tile_hgt;
			uint32_t layer_count = layer_wid * layer_hgt;
			uint32_t *newMap = (uint32_t*)calloc(1, layer_count);
			size_t old_slot = 0;
			for (uint32_t oy = 0; oy < layer->height; ++oy) {
				for (uint32_t ox = 0; ox < layer->width; ++ox) {
					uint32_t tl = layer->map[old_slot];
					TileSet *ts = GetTilesetFromTile(map, tl);
					for (uint32_t ny = 0; ny < map.tile_hgt; ++ny) {
						for (uint32_t nx = 0; nx < map.tile_wid; ++nx) {
							// TODO: Handle flip & rotation of tile here!!
							newMap[ox*map.tile_wid + nx + (oy*map.tile_hgt + ny)*layer_wid] =
								tl + nx + ny * ts->width;
						}
					}
				}
			}
		}
	}
}

bool ReadTilesetXML(void* user, strref tag_or_data, const strref* tag_stack, int depth, XML_TYPE type)
{
	TileMap *map = (TileMap*)user;
	TileSet &tileset = map->tileSets[map->currTileSet];

	if (type == XML_TYPE_TAG_OPEN || type == XML_TYPE_TAG_SELF_CLOSE) {
		strref tag = tag_or_data.get_word();
		if (tag.same_str("tileset")) {
			tileset.tile_wid = (int)XMLFindAttr(tag_or_data, strref("tilewidth")).atoi();
			tileset.tile_hgt = (int)XMLFindAttr(tag_or_data, strref("tileheight")).atoi();
			tileset.columns = (int)XMLFindAttr(tag_or_data, strref("columns")).atoi();
			tileset.count = (int)XMLFindAttr(tag_or_data, strref("tilecount")).atoi();
		} else if (tag.same_str("image")) {
			tileset.width = (int)XMLFindAttr(tag_or_data, strref("width")).atoi() / tileset.tile_wid;
			tileset.height = (int)XMLFindAttr(tag_or_data, strref("height")).atoi() / tileset.tile_hgt;
			tileset.imageName = XMLFindAttr(tag_or_data, strref("source"));
		}
	}

	return true;
}

bool LoadMap(const char *mapFilename, TileMap &map)
{
	size_t mapFileSize = 0;
	char* mapFile = LoadText(mapFilename, mapFileSize);
	if (!mapFile) {
		printf("failed to open %s for reading\n", mapFilename);
		return false;
	}

	strref mapXML(mapFile, (strl_t)mapFileSize);
	map.tileMapFile = mapFile;

	if (!ParseXML(mapXML, ReadMapXML, &map)) {
		printf("map parsing failure in %s\n", mapFilename);
		return false;
	}

	// turn any non 1x1 char maps into 
	ExplodeTiles(map);	// TODO: Test!

	strref path;
	strref mapFilestr(mapFilename);
	int pathEnd = mapFilestr.find_last('/', '\\');
	if (pathEnd >= 0) { path = mapFilestr.get_substr(0, pathEnd + 1); }
	map.currTileSet = 0;
	for (std::vector<TileSet>::iterator tileset = map.tileSets.begin(); tileset != map.tileSets.end(); ++tileset) {

		strown<_MAX_PATH> tilesetFile(path);
		tilesetFile.append(tileset->xmlName);

		size_t tilesetFileSize = 0;
		void* tilesetData = LoadText(tilesetFile.c_str(), tilesetFileSize);
		if (!tilesetData) {
			printf("failed to open " STRREF_FMT " for reading\n", STRREF_ARG(tilesetFile));
			return false;
		}

		tileset->tileSetFile = tilesetData;
		strref tilesetXML((const char*)tilesetData, (strl_t)tilesetFileSize);
		ParseXML(tilesetXML, ReadTilesetXML, &map);

		++map.currTileSet;
	}
	return true;
}
