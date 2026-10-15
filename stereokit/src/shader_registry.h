#pragma once
#include <array>
#include <cstddef>
namespace voxen_effects {
enum class Kind { Material, Volume, Environment };
struct Definition {
    int legacy_id;
    const char* id;
    const char* label;
    Kind kind;
};
inline constexpr std::array<Definition,9> materials = {{
    {0,"matte","MATTE",Kind::Material},
    {1,"neon","NEON",Kind::Material},
    {2,"plasma","PLASMA",Kind::Material},
    {3,"holo","HOLO",Kind::Material},
    {4,"lava","LAVA",Kind::Material},
    {5,"ice","ICE",Kind::Material},
    {6,"rasengan","RASENGAN",Kind::Material},
    {7,"particles","PARTICLES",Kind::Material},
    {8,"noise","NOISE",Kind::Material}
}};
inline constexpr int count = static_cast<int>(materials.size());
inline constexpr bool valid(int id) {return id>=0 && id<count;}
inline constexpr const Definition& get(int id) { return materials[static_cast<std::size_t>(id)]; }
} // namespace voxen_effects
