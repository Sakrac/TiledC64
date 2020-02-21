#pragma once
#include "MapRead.h"
bool LoadImgMap(TileMap& map,
				const char* image, const char *colStr, const char *metaStr,
				const char* scrBitStr, const char* colBitStr, const char* metaMapBitStr,
				const char* luBitStr, const char* typeStr, const char* expStr, const char* stats);

