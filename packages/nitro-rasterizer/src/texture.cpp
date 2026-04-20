#define STB_IMAGE_IMPLEMENTATION
#include <rasterizer/stb_image.h>
#include <rasterizer/texture.h>
#include <math/math.h>
using namespace nitro::math;
namespace nitro::rasterizer
{
    Texture Texture::loadFromFilePath(std::string fileName)
    {
        Texture texture;

        int w, h, c;
        stbi_uc *raw = stbi_load(fileName.c_str(), &w, &h, &c, STBI_rgb_alpha);
        if (!raw)
            throw std::runtime_error("Failed to load: " + fileName);
        texture.width = w;
        texture.height = h;
        texture.channels = 4;
        texture.pixels.assign(raw, raw + w * h * 4);

        stbi_image_free(raw);

        return texture;
    }
    Color Texture::sampleAt(int x, int y) const
    {
        int idx = (y * width + x) * 4;

        return {pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]};
    }
    Color Texture::sample(float u, float v) const
    {

        u = std::fmod(u, 1.0f);
        v = std::fmod(v, 1.0f);

        if (u < 0)
            u += 1.0f;
        if (v < 0)
            v += 1.0f;
        int tx = (int)(u * width) % width;
        int ty = (int)(v * height) % height;
        int idx = (ty * width + tx) * channels;
        return Color{pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]};
    }
    Color Texture::sampleBilinear(float u, float v) const
    {

        u = std::fmod(u, 1.0f);
        v = std::fmod(v, 1.0f);

        if (u < 0)
            u += 1.0f;
        if (v < 0)
            v += 1.0f;
        float fx = u * width - 0.5f;
        float fy = v * height - 0.5f;

        fx = std::max(0.0f, fx);
        fy = std::max(0.0f, fy);

        int x0 = (int)std::floor(fx);
        int y0 = (int)std::floor(fy);

        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);

        x0 = std::min(x0, width - 1);
        y0 = std::min(y0, height - 1);

        float sx = fx - std::floor(fx);
        float sy = fy - std::floor(fy);

        Color c00 = sampleAt(x0, y0);
        Color c10 = sampleAt(x1, y0);
        Color c01 = sampleAt(x0, y1);
        Color c11 = sampleAt(x1, y1);

        return Color::lerp(Color::lerp(c00, c10, sx), Color::lerp(c01, c11, sx), sy);
    }

}