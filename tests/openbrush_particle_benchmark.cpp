// CPU geometry benchmark, not a Quest GPU frame-time measurement.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <voxen/render/OpenBrushStarsStroke.hpp>
#include <voxen/render/OpenBrushFireStroke.hpp>
#include <voxen/render/OpenBrushSnowStroke.hpp>
#include <voxen/render/OpenBrushEmbersStroke.hpp>
#include <voxen/render/OpenBrushBubblesStroke.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <stdexcept>
int main() {
    using namespace voxen::render::openbrush;
    using clock=std::chrono::steady_clock;
    for(int count : {100,1000,5000}) {
        std::vector<SmokeStrokePoint> points;
        points.reserve(count);
        for(int i=0;i<count;++i) {
            SmokeStrokePoint p{};
            p.center={float(i%50)*0.03f,float(i/50)*0.03f,-1.f};
            p.size=0.08f;
            p.opacity=1.f;
            p.rgb={1.f,0.7f,0.4f};
            points.push_back(p);
        }
        auto start=clock::now();
        auto snow=makeSnowStrokeScene(points);
        auto embers=makeEmbersStrokeScene(points);
        auto bubbles=makeBubblesStrokeScene(points);
        auto end=clock::now();
        std::size_t vertices=0,triangles=0;
        for(const auto* scene : {&snow,&embers,&bubbles}) {
            for(const auto& mesh:scene->meshes) {
                vertices+=mesh.positions.size()/3;
                triangles+=mesh.indices.size()/3;
                for(auto idx:mesh.indices)
                    if(idx>=mesh.positions.size()/3)
                        throw std::runtime_error("particle index out of range");
            }
        }
        std::cout<<"points_per_brush="<<count
                 <<" brushes=3 vertices="<<vertices
                 <<" triangles="<<triangles
                 <<" cpu_geometry_ms="
                 <<std::chrono::duration<double,std::milli>(end-start).count()
                 <<" (not GPU time)\n";
    }
}
