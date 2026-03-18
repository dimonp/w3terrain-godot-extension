#ifndef _W3MAPCOLLECTOR_IMPL__H
#define _W3MAPCOLLECTOR_IMPL__H

#include <quadtree/quadtree.h>
#include <quadtree/quadtree_collector.h>

#include "w3defs.h"
#include "w3math.h"
#include "w3mapcollector.h"

namespace w3terr {

template<typename T>
using W3QuadTree = qtree::QuadTree<T, math::vector3, math::bbox3, std::vector, std::allocator>;

template<typename T>
using W3QuadTreeNode = typename W3QuadTree<T>::template Node<T>;

template<typename T>
using W3QuadTreeCollector = qtree::QuadTreeCollector<T, math::vector3, math::bbox3, std::vector, std::allocator>;

class W3MapSectionManager;

class W3_API W3MapCollectorImpl final : public W3MapCollector {
public:
    W3MapCollectorImpl(
        const W3MapSectionManager* sections,
        const math::bbox3& root_bbox,
        uint8_t tree_depth);

    math::bbox3 get_bbox() const override;

    void collect_intersected(const math::line3& line, W3Array<uint32_t>& sections) const override;

    void collect_visible(
        const godot::Projection camera_projection, 
        const godot::Transform3D camera_transform, 
        const godot::Transform3D& node_transform) override;
    const W3Array<uint32_t>& get_visible_sections() const override;

private:
    void build_quad_tree(const math::bbox3& root_bbox, uint8_t depth);

    // Array of section after frustum culling collecting
    W3Array<uint32_t> visible_sections_;

    W3QuadTree<uint32_t> quadtree_;
    gsl::not_null<const W3MapSectionManager*> sections_;
};

inline
const W3Array<uint32_t>&
W3MapCollectorImpl::get_visible_sections() const
{
    return visible_sections_;
}

inline
math::bbox3
W3MapCollectorImpl::get_bbox() const
{
    return math::bbox3::from_pos_size(
        quadtree_.get_root_bbox().get_min(),
        quadtree_.get_root_bbox().get_size());
}

}  // namespace w3terr

#endif // _W3MAPCOLLECTOR_IMPL__H
