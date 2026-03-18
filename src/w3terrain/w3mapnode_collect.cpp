#include <godot_cpp/classes/engine.hpp>
#include "w3mapsectionmanager_impl.h"
#include "w3mapcollector_impl.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3MapNode::collect_visible_sections()
{
    static uint64_t last_farme_id = std::numeric_limits<uint64_t>::max();
    const godot::Camera3D* camera = get_camera();
    if (camera == nullptr) {
        return;
    }

    if (!collector_) {
        return;
    }

    const uint64_t frame_id = godot::Engine::get_singleton()->get_process_frames();
    if (last_farme_id != frame_id) {
        collector_->collect_visible(
            camera->get_camera_projection(), 
            camera->get_global_transform(), 
            get_global_transform());

        for(uint32_t section_id : collector_->get_visible_sections()) {
            W3MapSection& section = get_section_manager()->get_section_by_id(section_id);
            section.refresh();
        }
        last_farme_id = frame_id;
    }
}

std::optional<Coord2D>
W3MapNode::get_intersected_cell(const math::line3& line, math::vector3& ipoint) const
{
    W3Array<uint32_t> intersected_sections(16);

    if (!collector_) {
        return std::nullopt;
    }

    collector_->collect_intersected(line, intersected_sections);
    if (intersected_sections.empty()) {
        return std::nullopt;
    }

    for(uint32_t section_id : intersected_sections) {
        const W3MapSection& section = sections_manager_->get_section_by_id(section_id);
        const std::optional<Coord2D> coords = section.find_intersected_cell(line,  ipoint);
        if (coords.has_value()) {
            return coords;
        }
    }
    return std::nullopt;
}

}  // namespace w3terr