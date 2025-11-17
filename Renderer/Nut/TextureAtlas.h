#ifndef NOAI_TEXTUREATLAS_H
#define NOAI_TEXTUREATLAS_H
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

#include "stb_image.h"
#include "stb_image_write.h"

struct AtlasMapping
{
    float uvOffset[2];
    float uvScale[2];
};

class TextureAtlas
{
    std::unordered_map<std::string, AtlasMapping> atlas;
    std::vector<unsigned char> atlasData;
    int atlasWidth = 0, atlasHeight = 0;

public:
    void Create(const std::vector<std::string>& imageFiles);

    void WriteToFile(const std::string& fileName);

    AtlasMapping GetAtlasMapping(const std::string& file);
};


#endif //NOAI_TEXTUREATLAS_H
