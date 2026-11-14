#include "interaction/layer_ops.h"
#include <algorithm>

namespace interaction {

int duplicateElement(std::vector<SavedElement>& elems, int index, const PageArea& area, Vec2 offset) {
    if (index < 0 || index >= (int)elems.size()) return -1;

    int maxZ = elems[index].zOrder;
    for (const SavedElement& e : elems) maxZ = std::max(maxZ, e.zOrder);

    SavedElement copy = elems[index];
    copy.xform.position = area.clamp({copy.xform.position.x + offset.x,
                                      copy.xform.position.y + offset.y});
    copy.zOrder = maxZ + 1;                     // strictly above everything: it sorts last
    elems.push_back(copy);
    return (int)elems.size() - 1;
}

int moveLayer(std::vector<SavedElement>& elems, int index, int delta) {
    const int n = (int)elems.size();
    if (index < 0 || index >= n) return -1;

    const int target = std::clamp(index + delta, 0, n - 1);
    if (target != index) {
        SavedElement moving = elems[index];
        elems.erase(elems.begin() + index);
        elems.insert(elems.begin() + target, moving);
    }
    for (int i = 0; i < n; i++) elems[i].zOrder = i;    // z now matches list order exactly
    return target;
}

}  // namespace interaction
