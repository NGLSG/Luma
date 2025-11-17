//
// Created by 92703 on 2025/10/14.
//

#include "TextureAtlas.h"

namespace Nut {

void TextureAtlas::Create(const std::vector<std::string>& imageFiles)
{
    struct ImageData
    {
        int width, height, channels;
        unsigned char* data;
        std::string name;
    };
    std::vector<ImageData> imageData;
    imageData.resize(imageFiles.size());
    int i = 0;

    for (auto& image : imageFiles)
    {
        imageData[i].data = stbi_load(image.c_str(), &imageData[i].width, &imageData[i].height,
                                      &imageData[i].channels,
                                      STBI_rgb_alpha);
        atlasWidth += imageData[i].width;
        atlasHeight = std::max(imageData[i].height, atlasHeight);
        if (!imageData[i].data)
        {
            std::cerr << "Failed to load image: " << image.c_str() << std::endl;
        }
        imageData[i].name = image;
        i++;
    }
    int xOff = 0;
    atlasData = std::vector<unsigned char>(atlasHeight * atlasWidth * 4.0f);
    for (int i = 0; i < imageData.size(); i++)
    {
        auto& image = imageData[i].data;
        for (int y = 0; y < imageData[i].height; y++)
        {
            for (int x = 0; x < imageData[i].width; x++)
            {
                int dstInx = (y * imageData[i].width + (x + xOff)) * 4;
                int srcInx = (y * imageData[i].width + x) * 4;
                memcpy(&atlasData[dstInx], &image[srcInx], 4);
            }
        }
        AtlasMapping mapping;
        mapping.uvOffset[0] = xOff / atlasWidth;
        mapping.uvOffset[1] = 0;
        mapping.uvScale[0] = imageData[i].width / atlasWidth;
        mapping.uvScale[1] = imageData[i].height / atlasHeight;
        atlas[imageData[i].name] = mapping;
        xOff += imageData[i].width;
    }
}

void TextureAtlas::WriteToFile(const std::string& fileName)
{
    if (fileName.find(".jpg") != std::string::npos)
    {
        stbi_write_jpg(fileName.c_str(), atlasWidth, atlasHeight, 4, atlasData.data(), 0);
    }
    else if (fileName.find(".bmp") != std::string::npos)
    {
        stbi_write_bmp(fileName.c_str(), atlasWidth, atlasHeight, 4, atlasData.data());
    }
    else
    {
        stbi_write_png(fileName.c_str(), atlasWidth, atlasHeight, 4, atlasData.data(), 0);
    }
}

AtlasMapping TextureAtlas::GetAtlasMapping(const std::string& file)
{
    auto it = atlas.find(file);
    if (it == atlas.end())
    {
        return AtlasMapping();
    }
    return atlas[file];
}

} // namespace Nut
