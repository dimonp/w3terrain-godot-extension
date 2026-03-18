#include <quadtree/quadtree.h>
#include <quadtree/quadtree_collector.h>

#include "w3mapsection.h"
#include "w3mapsectionmanager.h"
#include "w3mapcollector_impl.h"

namespace w3terr {

W3MapCollectorImpl::W3MapCollectorImpl(
    const W3MapSectionManager* sections,
    const math::bbox3& root_bbox,
    uint8_t tree_depth
) : sections_(sections)
{
    visible_sections_.reserve(16);
    build_quad_tree(root_bbox, tree_depth);
}

void
W3MapCollectorImpl::build_quad_tree(const math::bbox3& root_bbox, uint8_t depth)
{
    quadtree_.initialize(root_bbox, depth);
    // loop all sections
    for(auto section_id : *sections_) {
        const auto& section = sections_->get_section_by_id(section_id);
        const auto& bbox = section.get_bbox();
        auto *const tree_node = quadtree_.find_containment_node(bbox);
        if (tree_node == nullptr) {
            w3_log_error("Section %d does not fit quadtree.", section_id);
            continue;
        }

        const int prev_element = static_cast<int>(tree_node->get_element());
        if (prev_element != 0) {
            w3_log_error("Section %d overlaps with section %d.", section_id, prev_element);
            continue;
        }
        tree_node->set_element(section_id);
    }
    // optimize unused quadtree nodes
    quadtree_.get_root_node().optimize_recursive();
}

void
W3MapCollectorImpl::collect_visible(
    const godot::Projection camera_projection,
    const godot::Transform3D camera_transform,
    const godot::Transform3D& node_transform
) {
    static uint64_t last_farme_id = std::numeric_limits<uint64_t>::max();

    if (quadtree_.get_number_nodes() == 0) {
        return;
    }

    const godot::Transform3D view_matrix = camera_transform.inverse();
    const math::matrix44 vp_matrix = camera_projection * view_matrix;
    const math::matrix44 mvp_matrix = vp_matrix * node_transform;

    const godot::Plane planes[] = {
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_NEAR),
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_FAR),
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_LEFT),
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_RIGHT),
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_TOP),
        mvp_matrix.get_projection_plane(godot::Projection::PLANE_BOTTOM),
    };

    W3QuadTreeCollector<uint32_t>::collect_by_frustum(
        quadtree_.get_root_node(),
        planes,
        visible_sections_);
}

void
W3MapCollectorImpl::collect_intersected(const math::line3& line, W3Array<uint32_t>& sections) const
{
    if (quadtree_.get_number_nodes() == 0) {
        return;
    }

    W3QuadTreeCollector<uint32_t>::collect_by_line_intersect(
        quadtree_.get_root_node(),
        line,
        sections);
}


}  // namespace w3terr