#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stack>
#include <limits>

#define bytes(value) reinterpret_cast<const char*>(value)

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct BGR { uint8_t b,g,r; };
#pragma pack(pop)

struct vertex { float x,y; };

struct BGRHash {
    size_t operator()(const BGR& c) const noexcept {
        return (c.r<<16)|(c.g<<8)|c.b;
    }
};

bool operator==(const BGR&a,const BGR&b){
    return a.r==b.r && a.g==b.g && a.b==b.b;
}

BGR EMPTY_COLOR{255,255,255};

uint8_t* imageData;
uint8_t* tempImageData;
size_t width,height,rowSize;

struct prov
{
    std::vector<size_t> vertices;
    std::vector<uint16_t> indices;
};

std::unordered_map<BGR,std::vector<prov>,BGRHash> pixelsPerProvincesPerState;

inline BGR getPixelSafe(size_t r,size_t c){
    if(r>=height||c>=width) return EMPTY_COLOR;
    return *reinterpret_cast<BGR*>(
        imageData + r*rowSize + c*3
    );
}

inline BGR getPixelSafeIndex(size_t i){
    return getPixelSafe(i/width,i%width);
}

/* ================= FLOOD FILL ================= */

void FloodFill(){
    size_t size=width*height;
    for(size_t i=0;i<size;i++){
        if(*reinterpret_cast<BGR*>(tempImageData + (i/width)*rowSize + (i%width)*3)==EMPTY_COLOR)
            continue;

        BGR color=*reinterpret_cast<BGR*>(tempImageData + (i/width)*rowSize + (i%width)*3);
        std::vector<size_t> prov;
        std::queue<size_t> q;
        q.push(i);

        while(!q.empty()){
            size_t n=q.front(); q.pop();
            BGR& px=*reinterpret_cast<BGR*>(tempImageData + (n/width)*rowSize + (n%width)*3);
            if(px!=color) continue;
            px=EMPTY_COLOR;
            prov.push_back(n);

            if(n>=width) q.push(n-width);
            if(n+width<size) q.push(n+width);
            if(n%width) q.push(n-1);
            if(n%width+1<width) q.push(n+1);
        }
        pixelsPerProvincesPerState[color].push_back({prov});
    }
}

/* ================= MOORE ================= */

struct offset{int8_t x,y;};

static const offset moore[8]={
    {1,0},{1,-1},{0,-1},{-1,-1},
    {-1,0},{-1,1},{0,1},{1,1}
};

bool MooreStep(size_t cur,size_t prev,BGR id,size_t& next){
    int cr=cur/width, cc=cur%width;
    int pr=prev/width,pc=prev%width;

    int dx=cc-pc, dy=cr-pr;
    int startDir=0;
    for(int i=0;i<8;i++)
        if(moore[i].x==dx&&moore[i].y==dy)
            startDir=(i+6)&7;

    for(int k=0;k<8;k++){
        int d=(startDir+k)&7;
        int nr=cr+moore[d].y;
        int nc=cc+moore[d].x;
        if(getPixelSafe(nr,nc)==id){
            next=size_t(nr)*width+size_t(nc);
            return true;
        }
    }
    return false;
}

/* ================= FIND CONTOUR ================= */

size_t FindStart(const std::vector<size_t>&prov,BGR id){
    for(size_t i:prov){
        size_t r=i/width,c=i%width;
        if(getPixelSafe(r-1,c)!=id||
           getPixelSafe(r+1,c)!=id||
           getPixelSafe(r,c-1)!=id||
           getPixelSafe(r,c+1)!=id)
            return i;
    }
    return std::numeric_limits<size_t>::max();
}

void FindContour(){
    for(auto&[state,provs]:pixelsPerProvincesPerState){
        for(auto&prov:provs){
            if(prov.vertices.empty()) continue;
            BGR id=getPixelSafeIndex(prov.vertices[0]);
            size_t start=FindStart(prov.vertices,id);
            if(start==std::numeric_limits<size_t>::max()) continue;

            std::vector<size_t> contour;
            contour.push_back(start);

            size_t cur=start;
            size_t prev = (start % width > 0) ? start - 1 : start + 1;


            const size_t MAX=prov.vertices.size()*8;
            for(size_t i=0;i<MAX;i++){
                size_t next;
                if(!MooreStep(cur,prev,id,next)) break;
                if(next==start) break;
                contour.push_back(next);
                prev=cur;
                cur=next;
            }
            prov.vertices.swap(contour);
        }
    }
}

static double PerpendicularDistance(size_t p, size_t a, size_t b, size_t width)
{
    double px = p % width;
    double py = p / width;

    double ax = a % width;
    double ay = a / width;

    double bx = b % width;
    double by = b / width;

    double dx = bx - ax;
    double dy = by - ay;

    if(dx == 0 && dy == 0)
        return std::hypot(px - ax, py - ay);

    double num = std::abs(dy * px - dx * py + bx * ay - by * ax);
    double den = std::sqrt(dx * dx + dy * dy);

    return num / den;
}

// Douglas-Peucker iterative
void Aproximate(size_t epsilon)
{
    for(auto& [state, provs] : pixelsPerProvincesPerState)
    {
        for(prov& prov : provs)
        {
            if(prov.vertices.size() < 3)
                continue;

            const size_t n = prov.vertices.size();

            std::vector<bool> keep_point(n, true);
            std::stack<std::pair<size_t, size_t>> st;

            // в стек кладем первый и последний индекс
            st.push({0, n - 1});

            while(!st.empty())
            {
                auto [startIndex, endIndex] = st.top();
                st.pop();

                double dMax = 0.0;
                size_t index = startIndex;

                for(size_t i = startIndex + 1; i < endIndex; ++i)
                {
                    if(keep_point[i])
                    {
                        double d = PerpendicularDistance(
                            prov.vertices[i],
                            prov.vertices[startIndex],
                            prov.vertices[endIndex],
                            width
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
            std::vector<size_t> result;
            result.reserve(n);

            for(size_t i = 0; i < n; ++i)
            {
                if(keep_point[i])
                    result.push_back(prov.vertices[i]);
            }

            prov.vertices.swap(result);
        }
    }
}

struct Pt {
    int x, y;
    size_t id;
};

inline Pt make_pt(size_t i, size_t width) {
    return { int(i % width), int(i / width), i };
}

inline long long cross(const Pt& a, const Pt& b, const Pt& c) {
    return 1LL * (b.x - a.x) * (c.y - a.y)
         - 1LL * (b.y - a.y) * (c.x - a.x);
}

long long signed_area2(const std::vector<Pt>& p) {
    long long a = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const auto& p0 = p[i];
        const auto& p1 = p[(i + 1) % p.size()];
        a += 1LL * p0.x * p1.y - 1LL * p1.x * p0.y;
    }
    return a;
}

bool point_in_triangle(const Pt& p,
                       const Pt& a,
                       const Pt& b,
                       const Pt& c)
{
    long long c1 = cross(a, b, p);
    long long c2 = cross(b, c, p);
    long long c3 = cross(c, a, p);
    return (c1 >= 0 && c2 >= 0 && c3 >= 0);
}

std::vector<size_t> triangulate_prov(const std::vector<size_t>& prov,
                                     size_t width)
{
    std::vector<Pt> poly;
    poly.reserve(prov.size());
    for (size_t i : prov)
        poly.push_back(make_pt(i, width));

    // CCW
    if (signed_area2(poly) < 0)
        std::reverse(poly.begin(), poly.end());

    std::vector<size_t> out; // a,b,c,a,b,c,...

    while (poly.size() > 3) {
        bool ear_found = false;

        for (size_t i = 0; i < poly.size(); ++i) {
            size_t i0 = (i + poly.size() - 1) % poly.size();
            size_t i1 = i;
            size_t i2 = (i + 1) % poly.size();

            const Pt& a = poly[i0];
            const Pt& b = poly[i1];
            const Pt& c = poly[i2];

            if (cross(a, b, c) <= 0)
                continue; // не выпуклая

            bool inside = false;
            for (size_t j = 0; j < poly.size(); ++j) {
                if (j == i0 || j == i1 || j == i2) continue;
                if (point_in_triangle(poly[j], a, b, c)) {
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


void Triangulate()
{
    for(auto& [state, provs] : pixelsPerProvincesPerState)
    {
        for(prov& prov : provs)
        {
            if (prov.vertices.size() < 3)
                continue;

            prov.vertices = triangulate_prov(prov.vertices, width);
        }
    }
}

void Optimize()
{
    for(auto& [state, provs] : pixelsPerProvincesPerState)
    {
        for(prov& prov : provs)
        {
            std::unordered_map<size_t, uint16_t> vertexToIndex;
            vertexToIndex.reserve(prov.vertices.size());

            std::vector<size_t> uniqueVertices;
            uniqueVertices.reserve(prov.vertices.size());

            prov.indices.clear();

            for (size_t v : prov.vertices)
            {
                auto it = vertexToIndex.find(v);
                if (it != vertexToIndex.end())
                {
                    // вершина уже есть
                    prov.indices.push_back(it->second);
                }
                else
                {
                    // новая вершина
                    uint16_t newIndex = static_cast<uint16_t>(uniqueVertices.size());
                    uniqueVertices.push_back(v);
                    vertexToIndex[v] = newIndex;
                    prov.indices.push_back(newIndex);
                }
            }

            prov.vertices.swap(uniqueVertices);
        }
    }
}

/* ================= OUTPUT ================= */

void GenVertices(std::ofstream&out){
    uint32_t statesCount = static_cast<uint32_t>(pixelsPerProvincesPerState.size());
    out.write(bytes(&statesCount),4);

    for(auto&[color, provs]:pixelsPerProvincesPerState)
    {
        out.write(bytes(&color), sizeof(BGR));

        uint32_t provsCount = static_cast<uint32_t>(provs.size());
        out.write(bytes(&provsCount), sizeof(uint32_t));

        for(prov& prov:provs)
        {
            //Skip invalid meshes
            if (prov.indices.size() % 3 != 0)
            {
                prov.vertices.clear();
                prov.indices.clear();
            }

            uint32_t verticesCount = static_cast<uint32_t>(prov.vertices.size());
            out.write(bytes(&verticesCount), sizeof(uint32_t));

            for(size_t i: prov.vertices)
            {
                vertex v{
                    static_cast<float>(i%width),
                    static_cast<float>(i/width)
                };
                out.write(bytes(&v),sizeof(v));
            }

            uint32_t indicesCount = static_cast<uint32_t>(prov.indices.size());
            out.write(bytes(&indicesCount), sizeof(uint32_t));

            out.write(reinterpret_cast<char*>(prov.indices.data()), prov.indices.size() * sizeof(uint16_t));
        }
    }
}

/* ================= MAIN ================= */

int main(int argc,char**argv){
    std::ifstream fin(argv[1],std::ios::binary);
    std::ofstream fout(argv[2],std::ios::binary);

    BMPFileHeader fh;
    BMPInfoHeader ih;
    fin.read((char*)&fh,sizeof(fh));
    fin.read((char*)&ih,sizeof(ih));

    width=ih.biWidth;
    height=ih.biHeight;
    rowSize=(width*3+3)&~3;

    std::vector<uint8_t> img(rowSize*height);
    std::vector<uint8_t> tmp(rowSize*height);

    fin.seekg(fh.bfOffBits);
    fin.read((char*)img.data(),img.size());
    fin.seekg(fh.bfOffBits);
    fin.read((char*)tmp.data(),tmp.size());

    imageData=img.data();
    tempImageData=tmp.data();

    FloodFill();
    FindContour();
    Aproximate(1);
    Triangulate();
    Optimize();
    GenVertices(fout);
}
