#include <rasterizer/pipeline.h>
#include <fstream>
#include <iostream>
#include <sstream>
namespace nitro::rasterizer
{

    struct Index
    {
        int v, vt, vn;
    };

    Index parseIndex(const std::string &token)
    {
        Index idx{0, 0, 0};

        sscanf(token.c_str(), "%d/%d/%d", &idx.v, &idx.vt, &idx.vn);

        return idx;
    }
    Vec4D processVertex(const Vec4D &v,
                        const Mat4 &model,
                        const Mat4 &view,
                        const Mat4 &projection,
                        int width, int height)
    {

        Vec4D clip = projection * view * model * v;

        float invW = 1.0f / clip.w;

        Vec3D ndc = clip.perspectiveDivide();

        Vec4D screen;
        screen.x = (ndc.x + 1) * 0.5 * width;
        screen.y = (1 - ndc.y) * 0.5 * height;
        screen.z = ndc.z;
        screen.w = clip.w;
        return screen;
    }

    std::vector<Triangle> processOBJFile(std::string fileName)
    {

        std::vector<Vec3D> positions;

        std::vector<Vec2D> uvs;
        std::vector<Vec3D> normals;

        std::vector<Triangle> triangles;

        std::ifstream file(fileName);
        std::string line;

        try
        {
            while (std::getline(file, line))
            {
                if (line.rfind("v ", 0) == 0)
                {
                    std::istringstream ss(line.substr(2));
                    Vec3D v;
                    ss >> v.x >> v.y >> v.z;
                    positions.push_back(v);
                }
                else if (line.rfind("vt ", 0) == 0)
                {
                    std::istringstream ss(line.substr(3));
                    Vec2D uv;
                    ss >> uv.x >> uv.y;
                    uvs.push_back(std::move(uv));
                }
                else if (line.rfind("vn ", 0) == 0)
                {
                    std::istringstream ss(line.substr(3));
                    Vec3D n;
                    ss >> n.x >> n.y >> n.z;
                    normals.push_back(n);
                }
                else if (line.rfind("f ", 0) == 0)
                {
                    std::istringstream ss(line.substr(2));
                    std::string t1, t2, t3;
                    ss >> t1 >> t2 >> t3;

                    Index i0 = parseIndex(t1);
                    Index i1 = parseIndex(t2);
                    Index i2 = parseIndex(t3);

                    Triangle tri;

                    tri.v0 = {
                        positions[i0.v - 1],
                        uvs[i0.vt - 1],
                        normals[i0.vn - 1],

                    };

                    tri.v1 = {
                        positions[i1.v - 1],
                        uvs[i1.vt - 1],
                        normals[i1.vn - 1],

                    };

                    tri.v2 = {
                        positions[i2.v - 1],
                        uvs[i2.vt - 1],
                        normals[i2.vn - 1],

                    };
                    triangles.push_back(std::move(tri));
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cout << triangles.size() << " Triangles" << "\n";
        return triangles;
    }

}