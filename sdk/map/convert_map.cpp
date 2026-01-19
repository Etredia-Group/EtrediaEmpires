#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
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

std::unordered_map<BGR,std::vector<std::vector<size_t>>,BGRHash>
pixelsPerProvincesPerState;

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
        pixelsPerProvincesPerState[color].push_back(prov);
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
            if(prov.empty()) continue;
            BGR id=getPixelSafeIndex(prov[0]);
            size_t start=FindStart(prov,id);
            if(start==std::numeric_limits<size_t>::max()) continue;

            std::vector<size_t> contour;
            contour.push_back(start);

            size_t cur=start;
            size_t prev = (start % width > 0) ? start - 1 : start + 1;


            const size_t MAX=prov.size()*8;
            for(size_t i=0;i<MAX;i++){
                size_t next;
                if(!MooreStep(cur,prev,id,next)) break;
                if(next==start) break;
                contour.push_back(next);
                prev=cur;
                cur=next;
            }
            prov.swap(contour);
        }
    }
}

/* ================= STUBS ================= */

void Aproximate(size_t){}
void Triangulate(){}

/* ================= OUTPUT ================= */

void GenVertices(std::ofstream&out){
    uint32_t sc=pixelsPerProvincesPerState.size();
    out.write(bytes(&sc),4);
    for(auto&[s,provs]:pixelsPerProvincesPerState){
        out.write(bytes(&s),3);
        uint32_t pc=provs.size();
        out.write(bytes(&pc),4);
        for(auto&p:provs){
            uint32_t vc=p.size();
            out.write(bytes(&vc),4);
            for(size_t i:p){
                vertex v{float(i%width),float(i/width)};
                out.write(bytes(&v),sizeof(v));
            }
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
    Aproximate(0);
    Triangulate();
    GenVertices(fout);
}
