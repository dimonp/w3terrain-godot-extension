#ifndef _W3MAPCOLLECTOR__H
#define _W3MAPCOLLECTOR__H

#include "w3defs.h"
#include "w3math.h"

namespace w3terr {

class W3MapCollector {
public:
    virtual ~W3MapCollector() = default;
    virtual math::bbox3 get_bbox() const = 0;
    virtual void collect_intersected(const math::line3& line, W3Array<uint32_t>& sections) const = 0;

    virtual void collect_visible(
        const godot::Projection camera_projection, 
        const godot::Transform3D camera_transform, 
        const godot::Transform3D& node_transform) = 0;
    virtual const W3Array<uint32_t>& get_visible_sections() const = 0;
};

}  // namespace w3terr

#endif // _W3MAPCOLLECTOR__H
