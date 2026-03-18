#ifndef _W3MAPSURFACE__H
#define _W3MAPSURFACE__H

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
class W3MapNode;

class W3Surface: public godot::VisualInstance3D {
    GDCLASS(W3Surface, VisualInstance3D)
public:
    void _notification(int p_what);

protected:
    static void _bind_methods();

    bool is_mesh_dirty() const;

    W3Ref<W3Mesh> get_mesh() const;
    W3Ref<godot::SurfaceTool> get_surface_tool() const;

    void begin_render(bool render_lines = false);
    void end_render();

    const W3MapAssets* get_assets() const;
    const W3MapRuntimeManagerImpl* get_runtime_manager() const;
    const W3MapSectionManagerImpl* get_section_manager() const;
    const W3MapCollectorImpl* get_collector() const;

    W3MapNode* get_map_node() const;
    void set_map_node(W3MapNode* map_node);

    W3Ref<godot::ArrayMesh> mesh_;
    W3Ref<godot::SurfaceTool> surface_tool_;
    int64_t vertices_counter_ = 0;

private:
    godot::RID instance_rid_;
    godot::RID scenario_rid_;

    W3MapNode* map_node_ptr_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAP_SURFACE__H
