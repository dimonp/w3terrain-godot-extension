#ifndef _W3MAPBINDINGS__H
#define _W3MAPBINDINGS__H

#include <godot_cpp/classes/ref_counted.hpp>

#include "w3math.h"

namespace w3terr {

class W3MapNode;
class W3MapRuntimeManagerImpl;

class W3MapBindings final: public godot::Object {
    GDCLASS(W3MapBindings, godot::Object)
public:
    W3MapBindings() = default;
    explicit W3MapBindings(W3MapNode* map_node_ptr): map_node_(map_node_ptr) {}

protected:
    static void _bind_methods();

    const W3MapRuntimeManagerImpl* get_runtime() const;

    godot::Variant get_map_size() const;
    godot::Variant get_map_name() const;
    godot::Variant get_cell_bbox(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_grond_tileset(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_geo_tileset(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_type(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_layer(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_ground_height(const godot::Vector2i& coords) const;
    godot::Variant get_cellpoint_position(const godot::Vector2i& coords) const;
    godot::Variant get_cell_height(const godot::Vector2i& coords) const;
    godot::Variant get_height_at_point(const godot::Vector3& point) const;

    godot::Variant pick_cell_by_screen_position(const math::vector2& screen_position) const;

private:
    auto w3e_map() const;

    W3MapNode* map_node_ = nullptr;
};

}  // namespace w3terr

#endif // _W3MAPBINDINGS__H
