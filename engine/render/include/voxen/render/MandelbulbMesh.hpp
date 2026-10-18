#pragma once
// One-time surface extraction of a power-8 Mandelbulb through tetrahedral
// decomposition of a regular voxel grid. No GPU-side fractal calculations.
#include <openxr/openxr.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace voxen::render {
inline std::vector<XrVector3f> generateMandelbulbMesh() {
    constexpr int N=44;
    constexpr float extent=1.22f;
    constexpr float step=(2.f*extent)/float(N);
    constexpr float scale=0.40f;
    constexpr int tet[6][4]={
        {0,5,1,6},{0,1,2,6},{0,2,3,6},
        {0,3,7,6},{0,7,4,6},{0,4,5,6}
    };
    constexpr int corner[8][3]={
        {0,0,0},{1,0,0},{1,1,0},{0,1,0},
        {0,0,1},{1,0,1},{1,1,1},{0,1,1}
    };
    const int stride=N+1;
    auto index=[&](int x,int y,int z)->size_t {
        return (size_t(z)*stride+size_t(y))*stride+size_t(x);
    };
    std::vector<uint8_t> inside(size_t(stride)*stride*stride);
    for(int z=0;z<=N;z++)for(int y=0;y<=N;y++)for(int x=0;x<=N;x++){
        XrVector3f p{-extent+x*step,-extent+y*step,-extent+z*step};
        float zx=p.x,zy=p.y,zz=p.z;
        bool escaped=false;
        for(int it=0;it<9;it++){
            float r=std::sqrt(zx*zx+zy*zy+zz*zz);
            if(r>2.f){escaped=true;break;}
            float safe=std::fmax(r,0.00001f);
            float theta=std::acos(std::fmax(-1.f,std::fmin(1.f,zz/safe)))*8.f;
            float phi=std::atan2(zy,zx)*8.f;
            float zr=std::pow(safe,8.f);
            zx=zr*std::sin(theta)*std::cos(phi)+p.x;
            zy=zr*std::sin(theta)*std::sin(phi)+p.y;
            zz=zr*std::cos(theta)+p.z;
        }
        inside[index(x,y,z)]=escaped?0:1;
    }
    std::vector<XrVector3f> mesh;
    mesh.reserve(150000);
    for(int z=0;z<N;z++)for(int y=0;y<N;y++)for(int x=0;x<N;x++){
        XrVector3f p[8];
        uint8_t in[8];
        bool anyIn=false,anyOut=false;
        for(int c=0;c<8;c++){
            int xx=x+corner[c][0],yy=y+corner[c][1],zz=z+corner[c][2];
            p[c]=XrVector3f{(-extent+xx*step)*scale,(-extent+yy*step)*scale,(-extent+zz*step)*scale};
            in[c]=inside[index(xx,yy,zz)];
            anyIn|=in[c]!=0;
            anyOut|=in[c]==0;
        }
        if(!anyIn||!anyOut)continue;
        auto mid=[&](int a,int b)->XrVector3f {
            return {(p[a].x+p[b].x)*0.5f,(p[a].y+p[b].y)*0.5f,(p[a].z+p[b].z)*0.5f};
        };
        auto tri=[&](XrVector3f a,XrVector3f b,XrVector3f c){
            mesh.push_back(a);mesh.push_back(b);mesh.push_back(c);
        };
        for(const auto& t:tet){
            int a[4],b[4],na=0,nb=0;
            for(int j=0;j<4;j++)(in[t[j]]?a[na++]:b[nb++])=t[j];
            if(na==1){
                tri(mid(a[0],b[0]),mid(a[0],b[1]),mid(a[0],b[2]));
            }else if(na==3){
                tri(mid(b[0],a[0]),mid(b[0],a[2]),mid(b[0],a[1]));
            }else if(na==2){
                auto p0=mid(a[0],b[0]),p1=mid(a[0],b[1]);
                auto p2=mid(a[1],b[0]),p3=mid(a[1],b[1]);
                tri(p0,p1,p2);tri(p1,p3,p2);
            }
        }
    }
    return mesh;
}
} // namespace voxen::render
