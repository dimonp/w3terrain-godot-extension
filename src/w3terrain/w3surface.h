#ifndef _W3MAPSURFACE__H
#define _W3MAPSURFACE__H

#include <cstdint>

#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include "w3defs.h"

namespace w3terr {

class W3MapAssets;
class W3MapRuntimeManagerImpl;
class W3MapSectionManagerImpl;
class W3MapCollectorImpl;
class W3SectionRenderedCache;
class RenderedSection;
class W3MapNode;

class W3Surface: public godot::VisualInstance3D {
    GDCLASS(W3Surface, VisualInstance3D)
public:
    void _notification(int p_what);

    godot::AABB _get_aabb() const override;
    void _enter_tree() override;

protected:
    static void _bind_methods();

    W3Ref<godot::SurfaceTool> get_surface_tool() const;

    bool get_render_debug() const;
    void set_render_debug(bool flag);

    W3Ref<W3Marerial> get_debug_material() const;
    void set_debug_material(const W3Ref<W3Marerial>& material);

    bool is_mesh_dirty() const;

    int8_t begin_render(uint32_t section_id, bool render_lines = false);
    RenderedSection* end_render(uint32_t section_id);

    void clear_rendered();

    const W3MapAssets* get_assets() const;
    const W3MapRuntimeManagerImpl* get_runtime_manager() const;
    const W3MapSectionManagerImpl* get_section_manager() const;
    const W3MapCollectorImpl* get_collector() const;
    W3SectionRenderedCache* get_rendered_sections_cache() const;

    W3MapNode* get_map_node() const;
    void set_map_node(W3MapNode* map_node);

    W3Ref<godot::SurfaceTool> surface_tool_;
    int64_t vertices_counter_;

    bool render_debug_ = false;
    W3Ref<W3Marerial> debug_material_;

private:
    W3MapNode* map_node_ptr_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAP_SURFACE__H
