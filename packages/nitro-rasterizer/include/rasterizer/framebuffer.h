#pragma once
#include <algorithm>
#include <vector>
#include <math/vec.h>
#include <rasterizer/color.h>
using namespace nitro::math;

namespace nitro::rasterizer
{
    struct FrameBuffer
    {
        int width, height;
        int channels = 4;
        std::vector<uint8_t> data;
        std::vector<float> depth;
        FrameBuffer(int width, int height) : width(width), height(height), data(width * height * channels, 0), depth(width * height, 1.0f) {}

        void setPixel(int x, int y, Color col)
        {
            if (x < 0 || x >= width || y < 0 || y >= height)
                return;
            int idx = (y * width + x) * channels;

            data[idx] = col.r;
            data[idx + 1] = col.g;
            data[idx + 2] = col.b;
            data[idx + 3] = col.a;
        }

        std::vector<uint8_t> getPixel(int x, int y)
        {
            if (x < 0 || x >= width || y < 0 || y >= height)
                return {};
            int idx = (y * width + x) * channels;

            return {data[idx], data[idx + 1], data[idx + 2], data[idx + 3]};
        }

        void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            for (int i = 0; i < width * height; i++)
            {
                int idx = i * channels;
                data[idx] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = a;
            }
        }
        void clearDepth()
        {
            std::fill(depth.begin(), depth.end(), 1.0f);
        }

        bool testAndSetDepth(int x, int y, float z)
        {
            int idx = y * width + x;

            if (z < depth[idx])
            {
                depth[idx] = z;
                return true;
            }
            return false;
        }
        void writePPM(const std::string &path)
        {
            FILE *f = fopen(path.c_str(), "wb");
            fprintf(f, "P6\n%d %d\n255\n", width, height);
            for (int i = 0; i < width * height; i++)
            {
                fwrite(&data[i * channels], 1, 3, f);
            }
            fclose(f);
        }
        void drawLine(int x0, int y0, int x1, int y1, const Color color)
        {

            setPixel(x0, y0, color);
            int dx = std::abs(x1 - x0);
            int dy = std::abs(y1 - y0);

            int sx = x0 < x1 ? 1 : -1;
            int sy = y0 < y1 ? 1 : -1;

            int err = dx - dy;
            while (true)
            {
                if (x0 == x1 && y0 == y1)
                {
                    break;
                }

                int err2 = err * 2;

                if (err2 > -dy)
                {
                    err -= dy;
                    x0 += sx;
                }
                if (err2 < dx)
                {
                    err += dx;
                    y0 += sy;
                }

                setPixel(x0, y0, color);
            }
        }

        void drawTriangle(Vec2D v0, Vec2D v1, Vec2D v2, Color color)
        {
            if (v1.y < v0.y)
                std::swap(v0, v1);
            if (v2.y < v0.y)
                std::swap(v0, v2);
            if (v2.y < v1.y)
                std::swap(v1, v2);

            float dxLong = (v2.x - v0.x) / (v2.y - v0.y);
            float dxTop = (v1.x - v0.x) / (v1.y - v0.y);
            float dxBottom = (v2.x - v1.x) / (v2.y - v1.y);

            float yStart = (int)v0.y;
            float yEnd = (int)v1.y;

            float xLong = v0.x;
            float xTop = v0.x;

            for (int y = yStart; y <= yEnd; y++)
            {
                int xl = (int)std::min(xLong, xTop);
                int xr = (int)std::max(xLong, xTop);

                for (int x = xl; x <= xr; x++)
                {
                    setPixel(x, y, color);
                }

                xLong += dxLong;
                xTop += dxTop;
            }
            yStart = yEnd;
            yEnd = v2.y;

            float xBottom = v1.x;
            for (int y = yStart; y <= yEnd; y++)
            {
                int xl = (int)std::min(xLong, xBottom);
                int xr = (int)std::max(xLong, xBottom);

                for (int x = xl; x <= xr; x++)
                {
                    setPixel(x, y, color);
                }

                xLong += dxLong;
                xBottom += dxBottom;
            }
        }
    };

}