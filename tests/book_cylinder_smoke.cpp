#include <voxen/scene/BookCylinder.hpp>
#include <cassert>
#include <cmath>

int main() {
    using voxen::gallery::makeBookCylinderVertices;
    const auto mesh=makeBookCylinderVertices();
    assert(mesh.size()==48u*6u);
    assert(mesh.front().uv[0]==0.f);
    assert(mesh[mesh.size()-5].uv[0]>2.9f);
    for(const auto& v:mesh) {
        const auto& p=v.position;
        assert(std::abs(p[0]*p[0]+p[2]*p[2]-1.f)<0.0001f);
        assert(p[1]==0.5f || p[1]==-0.5f);
        assert(std::abs(v.normal[0]*p[0]+v.normal[2]*p[2]-1.f)<0.0001f);
    }
    const auto a=mesh[0].position,b=mesh[1].position,c=mesh[2].position;
    const float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2];
    const float vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
    const float nx=uy*vz-uz*vy, nz=ux*vy-uy*vx;
    assert(nx*a[0]+nz*a[2]>0.f); // outward triangle winding
}
