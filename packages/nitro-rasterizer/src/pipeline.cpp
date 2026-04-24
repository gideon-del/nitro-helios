#include <rasterizer/pipeline.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <math/math.h>
using namespace nitro::math;
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

                    tri.a = {
                        positions[i0.v - 1],
                        uvs[i0.vt - 1],
                        normals[i0.vn - 1],

                    };

                    tri.b = {
                        positions[i1.v - 1],
                        uvs[i1.vt - 1],
                        normals[i1.vn - 1],

                    };

                    tri.c = {
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

        return triangles;
    }

    VertexClipIn processVertexClip(
        const VertexIn &v,
        const Mat4 &model,
        const Mat4 &view, const Mat4 &proj)
    {
        Vec4D pos = Vec4D{v.position.x, v.position.y, v.position.z, 1};

        Vec4D worldPos = model * pos;
        Vec4D viewPos = view * worldPos;
        Vec4D clipPos = proj * viewPos;
        Vec3D normalVS = (Mat4::normalMatrix(view * model) * Vec4D{v.normal.x, v.normal.y, v.normal.z, 0}).toVec3D().normalize();

        return VertexClipIn{
            viewPos,
            clipPos,
            normalVS,
            v.uv};
    }
    bool isInsidePlane(const Vec4D &a, ClipPlane plane)
    {

        switch (plane)
        {
        case ClipPlane::Near:
            return -a.w <= a.z;
        case ClipPlane::Far:
            return a.z <= a.w;
        case ClipPlane::Left:
            return -a.w <= a.x;
        case ClipPlane::Right:
            return a.x <= a.w;
        case ClipPlane::Bottom:
            return -a.w <= a.y;
        case ClipPlane::Top:
            return a.y <= a.w;
        }
    }
    float findVertexDistanceByPlane(const VertexClipIn &a, const ClipPlane &plane)
    {

        switch (plane)
        {
        case ClipPlane::Near:
            return a.clipPos.z + a.clipPos.w;
        case ClipPlane::Far:
            return a.clipPos.w - a.clipPos.z;
        case ClipPlane::Left:
            return a.clipPos.x + a.clipPos.w;
        case ClipPlane::Right:
            return a.clipPos.w - a.clipPos.x;
        case ClipPlane::Top:
            return a.clipPos.w - a.clipPos.y;
        case ClipPlane::Bottom:
            return a.clipPos.y + a.clipPos.w;
        }
    }
    std::vector<VertexClipIn> filterVertexByPlane(const std::vector<VertexClipIn> &v, const ClipPlane &plane)
    {

        std::vector<VertexClipIn> result;
        for (int i = 0; i < v.size(); i++)
        {
            int nextIdx = i == v.size() - 1 ? 0 : i + 1;

            VertexClipIn currentV = v[i];
            VertexClipIn nextV = v[nextIdx];

            bool cVInPlane = isInsidePlane(currentV.clipPos, plane);
            bool nextVInPlane = isInsidePlane(nextV.clipPos, plane);

            if (cVInPlane && nextVInPlane)
            {
                result.push_back(nextV);
                continue;
            }
            if (!cVInPlane && nextVInPlane)
            {
                float dA = findVertexDistanceByPlane(currentV, plane);
                float dB = findVertexDistanceByPlane(nextV, plane);
                float t = dA / (dA - dB);
                VertexClipIn newVertex;
                newVertex.viewPos = currentV.viewPos.lerp(nextV.viewPos, t);
                newVertex.clipPos = currentV.clipPos.lerp(nextV.clipPos, t);
                newVertex.normalVS = currentV.normalVS.lerp(nextV.normalVS, t).normalize();
                newVertex.uv = currentV.uv.lerp(nextV.uv, t);

                result.push_back(newVertex);
                result.push_back(nextV);
                continue;
            }
            if (cVInPlane && !nextVInPlane)
            {
                float dA = findVertexDistanceByPlane(currentV, plane);
                float dB = findVertexDistanceByPlane(nextV, plane);
                float t = dA / (dA - dB);
                VertexClipIn newVertex;
                newVertex.viewPos = currentV.viewPos.lerp(nextV.viewPos, t);
                newVertex.clipPos = currentV.clipPos.lerp(nextV.clipPos, t);
                newVertex.normalVS = currentV.normalVS.lerp(nextV.normalVS, t).normalize();
                newVertex.uv = currentV.uv.lerp(nextV.uv, t);

                result.push_back(newVertex);
                continue;
            }
        }

        return result;
    }
    std::vector<ClippedTriangle> clipPolygons(const std::vector<VertexClipIn> &v)
    {
        std::vector<VertexClipIn> nearPlaneVertex = filterVertexByPlane(v, ClipPlane::Near);

        std::vector<VertexClipIn> farPlaneVertex = filterVertexByPlane(nearPlaneVertex, ClipPlane::Far);

        std::vector<VertexClipIn> leftPlaneVertex = filterVertexByPlane(farPlaneVertex, ClipPlane::Left);

        std::vector<VertexClipIn> rightPlaneVertex = filterVertexByPlane(leftPlaneVertex, ClipPlane::Right);

        std::vector<VertexClipIn> topPlaneVertex = filterVertexByPlane(rightPlaneVertex, ClipPlane::Top);

        std::vector<VertexClipIn> bottomPlaneVertex = filterVertexByPlane(topPlaneVertex, ClipPlane::Bottom);

        std::vector<VertexClipIn> result = bottomPlaneVertex;
        if (result.size() < 3)
            return {};
        std::vector<ClippedTriangle> triangles;

        for (int i = 1; i <= (int)result.size() - 2; i++)
        {
            triangles.push_back({result[0], result[i], result[i + 1]});
        }

        return triangles;
    }

    VertexOut processScreenVertex(const VertexClipIn &v,

                                  int width, int height)
    {

        Vec4D clip = v.clipPos;

        float invW = 1.0f / clip.w;
        Vec3D pos_over_w = v.viewPos.toVec3D() * invW;
        Vec3D ndc = clip.perspectiveDivide();

        Vec3D screen;
        screen.x = (ndc.x + 1) * 0.5 * width;
        screen.y = (1 - ndc.y) * 0.5 * height;
        screen.z = ndc.z;

        return VertexOut{
            screen,
            v.uv,
            v.normalVS,
            pos_over_w,
            clip.w};
    }

}