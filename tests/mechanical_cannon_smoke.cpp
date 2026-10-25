#include <voxen/scene/MechanicalCannonMesh.hpp>
#include <cassert>
#include <cmath>
int main() {
    using namespace voxen::scene::cannon;
    assert(geometry().size()==13);
    for (const auto& part:geometry()) {
        const auto mesh=partMesh(part);
        assert(!mesh.vertices.empty() && !mesh.indices.empty());
        assert(mesh.indices.size()%3==0);
        for(auto index:mesh.indices)assert(index<mesh.vertices.size());
    }
    assert(evaluate(0).projectiles.empty());
    assert(evaluate(9.9f).projectiles.empty());
    const auto a=evaluate(10.25f), b=evaluate(10.25f);
    assert(a.projectiles.size()==1 && b.projectiles.size()==1);
    assert(a.projectiles[0].position.z==b.projectiles[0].position.z);
    assert(a.muzzleFlash>0.f);
    for(float t=10.f;t<60.f;t+=.02f) {
        auto f=evaluate(t);
        assert(f.projectiles.size()<=12);
        for(const auto& p:f.projectiles) {
            assert(std::isfinite(p.position.x));
            assert(std::isfinite(p.position.y));
            assert(p.position.z<=-1.5f);
            assert(p.dissolve>=0.f && p.dissolve<=1.f);
        }
    }
}
