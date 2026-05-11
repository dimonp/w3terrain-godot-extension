#ifndef _W3MAPSECTION__H
#define _W3MAPSECTION__H

#include <bitset>

#include "w3defs.h"
#include "w3map.h"

#include <godot_cpp/classes/rendering_server.hpp>


namespace w3terr {

class W3MapRuntimeManager;

#define RS godot::RenderingServer::get_singleton()

class W3_API W3MapSection {
public:
    static constexpr int32_t kNumberOfCells = kSectionDimension * kSectionDimension;

    explicit W3MapSection(const W3MapRuntimeManager* map_runtime): runtime_manager_(map_runtime) {}
    void initialize(size_t ground_tilesets_size, size_t geo_tilesets_size);

    void update_all_cells(const Coord2D& section_origin);
    bool refresh(const Coord2D& section_origin);

    void set_dirty(bool flag = true);
    bool is_dirty() const;

    static Coord2D calc_cell_coord_from_idx(const Coord2D& section_origin, size_t cell_idx);

    W3Array<std::bitset<kNumberOfCells>> ground_tileset_usage;
    W3Array<std::bitset<kNumberOfCells>> geo_tileset_usage;
    std::bitset<kNumberOfCells> water_usage;

    W3Array<uint32_t> ground_tileset_to_layer_map;

    mutable struct RenderedMesh {
        godot::RID instance_rid;
        godot::RID mesh_rid;
        int8_t surface_idx_ground = -1;
        int8_t surface_idx_geo = -1;
        int8_t surface_idx_water = -1;

        ~RenderedMesh() {
            free();
        }

        const godot::RID& get_mesh_rid()
        {
            if (!mesh_rid.is_valid()) {
                mesh_rid = RS->mesh_create();
            }
            return mesh_rid;
        }

        const godot::RID& get_inst_rid()
        {
            if (!instance_rid.is_valid()) {
                instance_rid = RS->instance_create();
            }
            return instance_rid;
        }

        void free()
        {
            if (instance_rid.is_valid()) {
                RS->free_rid(instance_rid);
                instance_rid = {};
            }
            if (mesh_rid.is_valid()) {
                RS->free_rid(mesh_rid);
                mesh_rid = {};
            }
            surface_idx_ground = -1;
            surface_idx_geo = -1;
            surface_idx_water = -1;
        }
    } rendered_mesh;

private:
    void free_cached_data();
    void update_cell(const Coord2D& cell_coords, size_t cell_idx);

    const W3MapRuntimeManager* runtime_manager_;

    bool dirty_ = true;
};

inline
void
W3MapSection::set_dirty(bool flag)
{
    dirty_ = flag;
}

inline
bool
W3MapSection::is_dirty() const
{
    return dirty_;
}

inline
Coord2D
W3MapSection::calc_cell_coord_from_idx(const Coord2D& section_origin, size_t cell_idx)
{
    return {
        section_origin.x + static_cast<int32_t>(cell_idx % kSectionDimension),
        section_origin.y + static_cast<int32_t>(cell_idx / kSectionDimension)
    };
}

}  // namespace w3terr

#endif  /// _W3MAPSECTION__H

