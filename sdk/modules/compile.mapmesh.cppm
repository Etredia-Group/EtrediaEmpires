module;

#include <glm/glm.hpp>

export module compile.mapmesh;
import utils;
import io;
import AssetData;
import std;

struct Pt;

namespace ee_sdk {
#pragma pack(push, 1)
    struct BMPFileHeader {
        std::uint16_t bfType;
        std::uint32_t bfSize;
        std::uint16_t bfReserved1;
        std::uint16_t bfReserved2;
        std::uint32_t bfOffBits;
    };

    struct BMPInfoHeader {
        std::uint32_t biSize;
        std::int32_t  biWidth;
        std::int32_t  biHeight;
        std::uint16_t biPlanes;
        std::uint16_t biBitCount;
        std::uint32_t biCompression;
        std::uint32_t biSizeImage;
        std::int32_t  biXPelsPerMeter;
        std::int32_t  biYPelsPerMeter;
        std::uint32_t biClrUsed;
        std::uint32_t biClrImportant;
    };
#pragma pack(pop)

    using BGR = glm::vec<3, std::uint8_t>;
    struct BGRHash {
        std::size_t operator()(const BGR& c) const noexcept {return colorToHex(c.r, c.g, c.b, true);}
    };

    struct prov
    {
        std::vector<std::size_t> vertices; // pixels indices
        std::vector<std::uint16_t> indices;
    };

    using PixelMesh = std::unordered_map<BGR,std::vector<prov>,BGRHash>;

    struct Image {
        std::int32_t width;
        std::int32_t height;
        std::size_t rowSize;
        std::vector<std::byte> image;
        std::vector<std::byte> tempImage;
    };

    class Point {
    public:
        std::int32_t x, y;
        std::size_t id;

        Point() = default;
        Point (const std::size_t i, const std::size_t width) {
            x = i % width;
            y = i / width;
            id = i;
        }

        static std::int64_t Cross(const Point &a, const Point &b, const Point &c) {
            return 1LL * (b.x - a.x) * (c.y - a.y) - 1LL * (b.y - a.y) * (c.x - a.x);
        }

        static std::int64_t SignedArea2(const std::span<Point> &p) {
            std::int64_t a = 0;
            for (std::size_t i = 0; i < p.size(); i++) {
                const Point &p0 = p[i];
                const Point& p1 = p[(i + 1) % p.size()];
                a += 1LL * p0.x * p1.y - 1LL * p1.x * p0.y;
            }
            return a;
        }

        bool IsInTriangle(const Point &a, const Point &b, const Point &c) const {
            const std::int64_t c1 = Cross(a, b, *this);
            const std::int64_t c2 = Cross(b, c, *this);
            const std::int64_t c3 = Cross(c, a, *this);
            return (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0);
        }
    };
}

export namespace ee_sdk {
    class MapMeshCompiler {
    public:
        static ee_engine::MapMesh CompileBMP(std::span<std::byte> asset) {

            ee_engine::MapMesh mesh;

            // Read BMP file info from header
            BMPFileHeader fileHeader;
            BMPInfoHeader infoHeader;
            std::memcpy(&fileHeader, asset.data(), sizeof(BMPFileHeader));
            std::memcpy(&infoHeader, asset.data() + sizeof(BMPFileHeader), sizeof(BMPInfoHeader));

            Image imageInfo {
                .width = infoHeader.biWidth,
                .height = infoHeader.biHeight,
                .rowSize = static_cast<std::size_t>(infoHeader.biWidth)*3+3&~3
            };

            // Copy file to buffers for algorithms
            imageInfo.image.resize(imageInfo.rowSize * imageInfo.height);
            imageInfo.tempImage.resize(imageInfo.rowSize * imageInfo.height);

            std::memcpy(imageInfo.image.data(), asset.data() + fileHeader.bfOffBits, imageInfo.image.size());
            std::memcpy(imageInfo.tempImage.data(), asset.data() + fileHeader.bfOffBits, imageInfo.tempImage.size());

            PixelMesh pixelMesh;

            // Extract mesh from image
            FloodFill(imageInfo, pixelMesh);
            FindContour(imageInfo, pixelMesh);
            Aproximate(imageInfo, pixelMesh, 1);
            Triangulate(imageInfo, pixelMesh);
            Optimize(pixelMesh);

            // Provide mesh in suitable format
            for(auto&[color, provs]: pixelMesh)
            {
                std::vector<ee_engine::MapMesh::Province> outProvinces;

                for(auto&[vertices, indices]: provs)
                {
                    ee_engine::MapMesh::Province outProvince;

                    //Skip invalid meshes
                    if (indices.size() % 3 != 0)
                    {
                        vertices.clear();
                        indices.clear();
                    }

                    for(std::size_t i: vertices)
                    {
                        glm::vec2 v{
                            static_cast<float>(i % imageInfo.width),
                            static_cast<float>(i / imageInfo.width)
                        };
                        outProvince.vertices.push_back(v);
                    }
                    outProvince.indices = indices;

                    outProvinces.push_back(outProvince);
                }

                mesh.regions[colorToHex(color.r, color.g, color.b, true)] = outProvinces;
            }

            LOG(INFO, "COMPILED MAPMESH: generated [%d] regions", mesh.regions.size());

            return mesh;
        }

    private:
        static constexpr BGR EMPTY_COLOR = {255, 255, 255};

        static inline BGR getPixelSafe(const Image &image, const std::size_t r, const std::size_t c) {
            if(r >= image.height || c >= image.width) return EMPTY_COLOR;
            return *reinterpret_cast<const BGR*>(
                image.image.data() + r * image.rowSize + c * 3
            );
        }

        static inline BGR getPixelSafeIndex(const Image &image, const std::size_t i) {
            return getPixelSafe(image, i / image.width, i % image.width);
        }

        static void FloodFill(Image &image, PixelMesh &mesh) {
            const std::size_t size = image.width * image.height;
            for(std::size_t i = 0; i < size; i++) {
                BGR color= *reinterpret_cast<BGR*>(image.tempImage.data() + (i/image.width)*image.rowSize + (i%image.width)*3);

                if(color == EMPTY_COLOR)
                    continue;

                std::vector<std::size_t> prov;
                std::queue<std::size_t> q;
                q.push(i);

                while (!q.empty()) {
                    std::size_t n = q.front(); q.pop();
                    BGR& px = *reinterpret_cast<BGR*>(image.tempImage.data() + (n/image.width)*image.rowSize + (n%image.width)*3);
                    if(px != color) continue;
                    px = EMPTY_COLOR;
                    prov.push_back(n);

                    if(n >= image.width) q.push(n - image.width);
                    if(n + image.width < size) q.push(n + image.width);
                    if(n % image.width) q.push(n - 1);
                    if(n % image.width + 1 < image.width) q.push(n + 1);
                }
                mesh[color].push_back({.vertices = prov});
            }
        }

        static bool MooreStep(
            const Image &image,
            const std::size_t cur,
            const std::size_t prev,
            const BGR id,
            std::size_t &next
        ) {
            struct offset{int8_t x,y;};

            static const offset moore[8]={
                {1,0},{1,-1},{0,-1},{-1,-1},
                {-1,0},{-1,1},{0,1},{1,1}
            };

            const std::int32_t cr = cur / image.width, cc = cur % image.width;
            const std::int32_t pr = prev / image.width, pc = prev % image.width;

            const std::int32_t dx = cc - pc, dy = cr - pr;
            std::int32_t startDir = 0;
            for(std::int32_t i = 0; i < 8; i++) {
                if(moore[i].x == dx && moore[i].y == dy)
                    startDir=(i+6)&7;
            }

            for(std::int32_t k=0; k<8; k++){
                const std::int32_t d = (startDir+k)&7;
                const std::int32_t nr = cr + moore[d].y;
                if(
                    const std::int32_t nc = cc + moore[d].x;
                    getPixelSafe(image, nr,nc) == id
                ){
                    next= static_cast<std::size_t>(nr) * image.width + static_cast<std::size_t>(nc);
                    return true;
                }
            }
            return false;
        }


        static std::size_t FindStart(const Image &image, const std::vector<size_t>&prov, const BGR &id) {
            for(const std::size_t i : prov){
                const std::size_t r = i / image.width;
                const std::size_t c = i % image.width;

                if(getPixelSafe(image, r - 1, c) != id||
                   getPixelSafe(image, r + 1, c) != id||
                   getPixelSafe(image, r, c- 1) != id||
                   getPixelSafe(image, r, c+ 1) != id)
                    return i;
            }
            return std::numeric_limits<std::size_t>::max();
        }

        static void FindContour(const Image &image, PixelMesh &mesh) {
            for(auto &provs : mesh | std::views::values){
                for(auto&[vertices, indices] : provs){
                    if(vertices.empty()) continue;
                    BGR id = getPixelSafeIndex(image, vertices[0]);
                    std::size_t start = FindStart(image, vertices,id);
                    if(start == std::numeric_limits<std::size_t>::max()) continue;

                    std::vector<std::size_t> contour;
                    contour.push_back(start);

                    std::size_t cur=start;
                    std::size_t prev = (start % image.width > 0) ? start - 1 : start + 1;


                    const std::size_t MAX = vertices.size()*8;
                    for(std::size_t i = 0 ;i < MAX; i++){
                        std::size_t next;
                        if(!MooreStep(image, cur, prev, id, next)) break;
                        if(next == start) break;
                        contour.push_back(next);
                        prev = cur;
                        cur = next;
                    }
                    vertices.swap(contour);
                }
            }
        }

        static double PerpendicularDistance(
            const std::size_t p,
            const std::size_t a,
            const std::size_t b,
            const std::size_t width
        )
        {
            const double px = p % width;
            const double py = p / width;

            const double ax = a % width;
            const double ay = a / width;

            const double bx = b % width;
            const double by = b / width;

            const double dx = bx - ax;
            const double dy = by - ay;

            if(dx == 0 && dy == 0)
                return std::hypot(px - ax, py - ay);

            const double num = std::abs(dy * px - dx * py + bx * ay - by * ax);
            const double den = std::sqrt(dx * dx + dy * dy);

            return num / den;
        }

        // Douglas-Peucker iterative
        static void Aproximate(const Image &image, PixelMesh &mesh, const std::size_t epsilon)
        {
            for(auto &provs : mesh | std::views::values)
            {
                for(auto&[vertices, indices] : provs)
                {
                    if(vertices.size() < 3)
                        continue;

                    const std::size_t n = vertices.size();

                    std::vector keep_point(n, true);
                    std::stack<std::pair<std::size_t, std::size_t>> st;

                    // в стек кладем первый и последний индекс
                    st.push({0, n - 1});

                    while(!st.empty())
                    {
                        auto [startIndex, endIndex] = st.top();
                        st.pop();

                        double dMax = 0.0;
                        std::size_t index = startIndex;

                        for(std::size_t i = startIndex + 1; i < endIndex; ++i)
                        {
                            if(keep_point[i])
                            {
                                double d = PerpendicularDistance(
                                    vertices[i],
                                    vertices[startIndex],
                                    vertices[endIndex],
                                    image.width
                                );

                                if(d > dMax)
                                {
                                    dMax = d;
                                    index = i;
                                }
                            }
                        }

                        if(dMax >= epsilon)
                        {
                            st.push({startIndex, index});
                            st.push({index, endIndex});
                        }
                        else
                        {
                            for(size_t j = startIndex + 1; j < endIndex; ++j)
                                keep_point[j] = false;
                        }
                    }

                    // формируем итоговый список
                    std::vector<std::size_t> result;
                    result.reserve(n);

                    for(std::size_t i = 0; i < n; ++i)
                    {
                        if(keep_point[i])
                            result.push_back(vertices[i]);
                    }

                    vertices.swap(result);
                }
            }
        }

        static std::vector<std::size_t> TriangulateProv(const std::vector<std::size_t> &prov, const std::size_t width) {
            std::vector<Point> poly;
            poly.reserve(prov.size());
            for (const std::size_t i : prov)
                poly.push_back(Point(i, width));

            // CCW
            if (Point::SignedArea2(poly) < 0)
                std::ranges::reverse(poly);

            std::vector<std::size_t> out; // a,b,c,a,b,c,...

            while (poly.size() > 3) {
                bool ear_found = false;

                for (std::size_t i = 0; i < poly.size(); ++i) {
                    const std::size_t i0 = (i + poly.size() - 1) % poly.size();
                    const std::size_t i1 = i;
                    const std::size_t i2 = (i + 1) % poly.size();

                    const Point& a = poly[i0];
                    const Point& b = poly[i1];
                    const Point& c = poly[i2];

                    if (Point::Cross(a, b, c) <= 0)
                        continue; // не выпуклая

                    bool inside = false;
                    for (std::size_t j = 0; j < poly.size(); ++j) {
                        if (j == i0 || j == i1 || j == i2) continue;
                        if (poly[j].IsInTriangle(a, b, c)) {
                            inside = true;
                            break;
                        }
                    }

                    if (!inside) {
                        out.push_back(a.id);
                        out.push_back(b.id);
                        out.push_back(c.id);
                        poly.erase(poly.begin() + i1);
                        ear_found = true;
                        break;
                    }
                }

                if (!ear_found) {
                    // плохой контур (самопересечение / коллинеарность)
                    break;
                }
            }

            if (poly.size() == 3) {
                out.push_back(poly[0].id);
                out.push_back(poly[1].id);
                out.push_back(poly[2].id);
            }

            return out;
        }

        static void Triangulate(const Image &image, PixelMesh &pixelMesh)
        {
            for(auto &provs : pixelMesh | std::views::values)
            {
                for(auto&[vertices, indices] : provs)
                {
                    if (vertices.size() < 3)
                        continue;

                    vertices = TriangulateProv(vertices, image.width);
                }
            }
        }

        static void Optimize(PixelMesh &pixelMesh)
        {
            for(auto &provs : pixelMesh | std::views::values)
            {
                for(auto&[vertices, indices] : provs)
                {
                    std::unordered_map<std::size_t, std::uint16_t> vertexToIndex;
                    vertexToIndex.reserve(vertices.size());

                    std::vector<std::size_t> uniqueVertices;
                    uniqueVertices.reserve(vertices.size());

                    indices.clear();

                    for (std::size_t v : vertices)
                    {
                        if (auto it = vertexToIndex.find(v); it != vertexToIndex.end())
                        {
                            indices.push_back(it->second);
                        }
                        else
                        {
                            std::uint16_t newIndex = static_cast<std::uint16_t>(uniqueVertices.size());
                            uniqueVertices.push_back(v);
                            vertexToIndex[v] = newIndex;
                            indices.push_back(newIndex);
                        }
                    }

                    vertices.swap(uniqueVertices);
                }
            }
        }
    };
}