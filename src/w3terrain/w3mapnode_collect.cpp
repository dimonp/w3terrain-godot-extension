#include "w3mapnode.h"

#include <godot_cpp/classes/engine.hpp>
#ifdef EDITOR_SUPPORT_ENABLE
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#endif

#include "w3mapsectionmanager_impl.h"
#include "w3mapcollector_impl.h"

namespace w3terr {

void
W3MapNode::collect_visible_sections()
{
    static uint64_t last_farme_id = std::numeric_limits<uint64_t>::max();


    const godot::Camera3D* camera = get_camera();

#ifdef EDITOR_SUPPORT_ENABLE
    if (use_editor_camera_ && godot::Engine::get_singleton()->is_editor_hint()) {
        godot::EditorInterface *editor_gui = godot::EditorInterface::get_singleton();
        camera = editor_gui->get_editor_viewport_3d(0)->get_camera_3d();
    }
#endif

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
            get_section_manager()->refresh_section(section_id);
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
        const std::optional<Coord2D> coords = sections_manager_->find_intersected_cell(section_id, line,  ipoint);
        if (coords.has_value()) {
            return coords;
        }
    }
    return std::nullopt;
}

}  // namespace w3terr