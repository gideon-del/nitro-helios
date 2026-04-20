#pragma once
#include <iostream>
#include <vector>
#include <rasterizer/color.h>
namespace nitro::rasterizer
{
    struct Texture
    {
        int width, height, channels;
        std::vector<uint8_t> pixels;
        Color sample(float u, float v) const;
        Color sampleBilinear(float u, float v) const;
        Color sampleAt(int x, int y) const;

        static Texture loadFromFilePath(std::string fileName);
    };
}