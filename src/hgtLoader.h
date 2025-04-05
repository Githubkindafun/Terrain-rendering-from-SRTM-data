#ifndef HGT_LOADER_H
#define HGT_LOADER_H

#include <vector>
#include <string>
#include <cstdint>

struct HeightMap {
    int width;
    int height; // u nas oba beda 1201
    std::vector<short> data; 
};

HeightMap loadHgtFile(const std::string& filePath, short minValidHeight = -500, short maxValidHeight = 9000,
    short noDataValue = -9999);

#endif
