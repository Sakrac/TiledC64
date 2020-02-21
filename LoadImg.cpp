#include <vector>
#include "struse\struse.h"
#include "MapRead.h"
#include "ArtRead.h"
#include "stb/stb_image.h"

TileLayer* InitImageTilemap(TileMap& map, TileSet **retset)
{
	TileSet tileset = {};
	TileLayer layer = {};
	map.tileSets.push_back(tileset);
	map.layers.push_back(layer);

	*retset = &map.tileSets[0];
	return &map.layers[0];
}

// debug:
// -img c:\code\sk64\assets\pridemoore_outside.png -col=4,15,9,1 -meta=2,2 -colorBits=4 -type=ECBM -export=pridemoor_outside -stats=pridemoor_outside.txt



bool LoadImgMap(TileMap& map,
				const char* image, const char *colStr, const char *metaStr,
				const char* scrBitStr, const char* colBitStr, const char* metaMapBitStr,
				const char* luBitStr, const char* typeStr, const char* expStr, const char* stats)
{
	TileSet *tileset;
	TileLayer *layer = InitImageTilemap(map, &tileset);

	map.tile_wid = 8;
	map.tile_hgt = 8;
	tileset->tile_wid = 8;
	tileset->tile_hgt = 8;

	tileset->first = 1;


	// store filename of image
	tileset->imageName = strref(image);

	// load the image first to get the information about size etc.
	if (!LoadTileSetImages(map, "")) {
		return false;	// couldn't read the image file
	}

	tileset->width = tileset->img_wid / 8;
	tileset->height = tileset->img_hgt / 8;
	tileset->columns = tileset->width;
	layer->width = tileset->width;
	layer->height = tileset->height;
	tileset->count = tileset->width * tileset->height;

	layer->metaX = layer->metaY = 1;

	// set the colors from the command line
	if (colStr) {
		strref value(colStr);
		int i = 0;
		while (strref col = value.split_token(',')) {
			col.trim_whitespace();
			layer->bg[i++] = (char)col.atoi();
			if (i >= 4) { break; }
		}
	}

	// set meta tile size
	if (metaStr) {
		strref height(metaStr);
		height.trim_whitespace();
		strref width = height.split_token_trim(',');
		layer->metaX = width.atoi();
		layer->metaY = height.atoi();
	}

	if (scrBitStr) {
		layer->screenBits = (uint32_t)strref(scrBitStr).atoi();
	}

	if (colBitStr) {
		layer->colorBits = (uint32_t)strref(colBitStr).atoi();
	}

	if (metaMapBitStr) {
		layer->metaMapBits = (uint32_t)strref(metaMapBitStr).atoi();
	}

	if (luBitStr) {
		layer->metaLookupBits = (uint32_t)strref(luBitStr).atoi();
	}

	layer->type = TileLayer::Text;

	if (typeStr) {
		strref type(typeStr);
		if( type.same_str("Text")) {}
		else if (type.same_str("TextMC")) { layer->type = TileLayer::TextMC; }
		else if (type.same_str("ECBM")) { layer->type = TileLayer::ECBM; }
		else if (type.same_str("Bitmap")) { layer->type = TileLayer::Bitmap; }
		else if (type.same_str("BitmapMC")) { layer->type = TileLayer::BitmapMC; }
	}

	if (expStr) {
		layer->exportFile = strref(expStr);
	}

	if (stats) {
		map.stats = strref(stats);
	}

	map.currLayer = 0;
	layer->name = strref(image).after_last_or_full('/', '\\').before_or_full('.');
	tileset->xmlName = layer->name;

	layer->map = (uint32_t*)malloc(sizeof(uint32_t) * layer->width * layer->height);
	for (size_t i = 0, n = layer->width * layer->height; i < n; ++i) {
		layer->map[i] = i + 1;
	}
}
