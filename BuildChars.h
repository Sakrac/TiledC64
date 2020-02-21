#pragma once
#include "MapRead.h"
bool ConvertECBMLayer(TileMap& map, int layerIndex);
bool ConvertTextLayer(TileMap& map, int layerIndex);
bool ConvertTextMCLayer(TileMap& map, int layerIndex);
bool ConvertBitmapLayer(TileMap& map, int layerIndex);
void ConvertDataLayer(TileMap& map, int layerIndex);
bool MergeLayerData(TileMap &map, TileLayer &srcLayer, TileLayer &dstLayer);
bool MergeFlipData(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer);
bool MergeChars(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer);


