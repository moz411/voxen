#include <voxen/scene/SceneIds.hpp>
#include <cassert>
#include <set>
#include <string>
#include <iostream>

int main() {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) {
        auto id = voxen::gallery::newUuid();
        assert(id.size() == 36);
        assert(id[8] == '-' && id[13] == '-' && id[18] == '-' && id[23] == '-');
        assert(id[14] == '4');
        assert(id[19] == '8' || id[19] == '9' || id[19] == 'a' || id[19] == 'b');
        assert(ids.insert(id).second);
    }
    std::cout << "Scene IDs: PASS\n";
}
