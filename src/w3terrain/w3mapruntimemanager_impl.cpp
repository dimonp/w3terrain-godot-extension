#include <array>
#include "w3math.h"
#include "w3mapassets.h"
#include "w3mapinformator_impl.h"
#include "w3mapruntimemanager_impl.h"

namespace w3terr {

/**
 * @brief Calculates texture UV indices for a tile based on layer and variation codes.
 *
 * Performance-optimized using Look-Up Tables (LUT) to minimize branching and bitwise arithmetic.
 * - For layer_code < 15: Uses a 1:1 mapping to a 4x4 grid.
 * - For layer_code == 15 (Special Case): If 'is_extended' is true, maps 'variation' (0-16)
 *   to an extended tile set; otherwise, defaults to (0,0).
 *
 * @param is_extended Enables extended variation mapping for layer 15.
 * @param layer_code  Primary layer identifier (0-15).
 * @param variation   Sub-type identifier used only when layer_code is 15.
 * @param tile_idx_u  Output: Resulting U (column) texture index.
 * @param tile_idx_v  Output: Resulting V (row) texture index.
 */
[[nodiscard]]
static inline
W3Pair<uint8_t, uint8_t>
calc_tile_texture_indices_from_layer_code(
    const bool is_extended,
    const uint8_t layer_mask, // < 16
    const uint8_t variation   // <= 16
) {
    // LUT for layer_code 0-14
    static constexpr auto kBaseLutU = std::to_array<uint8_t>(
        {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2}
    );
    static constexpr auto kBaseLutV = std::to_array<uint8_t>(
        {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3}
    );

    // LUT for variation when layer_code == 15
    static constexpr auto kExtLutU = std::to_array<uint8_t>(
        {4,5,6,7,4,5,6,7,4,5,6,7,4,5,6,7,3}
    );
    static constexpr auto kExtLutV = std::to_array<uint8_t>(
        {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,3}
    );

    if (layer_mask < 15) {
        return { kBaseLutU[layer_mask], kBaseLutV[layer_mask] }; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    const bool use_ext = is_extended && (variation <= 16);
    if (use_ext) {
        return { kExtLutU[variation], kExtLutV[variation] }; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    return { 0, 0 };
}

W3MapRuntimeManagerImpl::W3MapRuntimeManagerImpl(
    const W3MapAssets* assets,
    const W3MapInformator* informator
) : map_asset_(assets) , informator_(informator)
{
    update_all_cells_rt();
}

inline
const auto*
W3MapRuntimeManagerImpl::w3e_map() const
{
    return map_asset_->get_w3e();
}

inline
W3MapRuntimeManagerImpl::CellPointRT&
W3MapRuntimeManagerImpl::get_cellpoint_rt(const Coord2D& coords)
{
    const auto map_size_x =  w3e_map()->get_map_2d_size_x();
    return cellpoints_rt_[static_cast<size_t>(coords.y * map_size_x) + coords.x];
}

const W3MapRuntimeManagerImpl::CellPointRT&
W3MapRuntimeManagerImpl::get_cellpoint_rt(const Coord2D& coords) const
{
    const auto map_size_x =  w3e_map()->get_map_2d_size_x();
    return cellpoints_rt_[static_cast<size_t>(coords.y * map_size_x) + coords.x];
}

float
W3MapRuntimeManagerImpl::get_cellpoint_layer_height(const Coord2D& coords) const
{
    return w3e_map()->get_cellpoint_layer_height(coords.x, coords.y);
}

float
W3MapRuntimeManagerImpl::get_cellpoint_ground_height(const Coord2D& coords) const
{
    return w3e_map()->get_cellpoint_ground_height(coords.x, coords.y);
}

math::vector3
W3MapRuntimeManagerImpl::get_cellpoint_position(const Coord2D& coords) const
{
    math::vector3 pos = w3e_map()->get_cellpoint_position(coords.x, coords.y);
    const W3MapRuntimeManagerImpl::CellPointRT& cell_rt = get_cellpoint_rt(coords);
    if (cell_rt.check_flag(CellPointRT::Flags::RAMP_MIDDLE)) {
        pos.y += kW3MapTile2DHalfSize;
    }
    return pos;
}

math::vector3
W3MapRuntimeManagerImpl::get_cellpoint_water_position(const Coord2D& coords) const
{
    return w3e_map()->get_cellpoint_water_position(coords.x, coords.y);
}

uint32_t
W3MapRuntimeManagerImpl::get_cliff_geoset_id_for_geokey(const size_t geo_tileset_id, const uint32_t geo_key) const
{
    const uint32_t geo_variation = geo_key >> 8U;
	const auto& keys_map = map_asset_->geo_asset_rt(geo_tileset_id).geo_cliff_keys_map;

    // find geoset id
    uint32_t geo_variation_mask = 7;
    size_t max_iter = 32;
    while(--max_iter != 0U) {
		const uint32_t geo_key_tmp = (geo_key & 0xffU) | ((geo_variation & geo_variation_mask) << 8U);

        const auto itr = keys_map.find(geo_key_tmp);
		if (itr != keys_map.end()) {
			return itr->second;
		}
        geo_variation_mask >>= 1U;
    }

    w3_log_error("Cliff geoset is not compatible with this map.");
    // Should not reach here, invalid geoset
    return 0;
}

uint32_t
W3MapRuntimeManagerImpl::get_ramp_geoset_id_for_geokey(const size_t geo_tileset_id, const uint32_t geo_key) const
{
    const uint32_t geo_variation = geo_key >> 16U;
    const auto& keys_map = map_asset_->geo_asset_rt(geo_tileset_id).geo_ramp_keys_map;

    // find geoset id
    uint32_t geo_variation_mask = 7;
    size_t max_iter = 32;
    while(--max_iter != 0U) {
        const uint32_t geo_key_tmp = (geo_key & 0x00f0ffffU) | ((geo_variation & geo_variation_mask) << 16U);
        const auto itr = keys_map.find(geo_key_tmp);
        if (itr != keys_map.end()) {
            return itr->second;
        }
        geo_variation_mask >>= 1U;
    }

    w3_log_error("Ramp geoset is not compatible with this map.");
    // Should not reach here, invalid geoset
    return 0;
}

math::bbox3
W3MapRuntimeManagerImpl::get_cell_bbox(const Coord2D& coords) const
{
    math::bbox3 bbox;
    bbox.begin_extend();
    bbox.extend(get_cellpoint_position(coords));
    bbox.extend(get_cellpoint_position({coords.x,       coords.y + 1 }));
    bbox.extend(get_cellpoint_position({coords.x + 1,   coords.y + 1 }));
    bbox.extend(get_cellpoint_position({coords.x + 1,   coords.y     }));
    return bbox;
}

float
W3MapRuntimeManagerImpl::get_cell_ground_height(const Coord2D& coords, const float t_lerp, const float s_lerp) const
{
    const float height00 = w3e_map()->get_cellpoint_total_height(coords.x,     coords.y    );
    const float height10 = w3e_map()->get_cellpoint_total_height(coords.x + 1, coords.y    );
    const float height11 = w3e_map()->get_cellpoint_total_height(coords.x + 1, coords.y + 1);
    const float height01 = w3e_map()->get_cellpoint_total_height(coords.x,     coords.y + 1);
    return math::w3_lerp_bi(height00, height01, height10, height11, t_lerp, s_lerp);
}

bool
W3MapRuntimeManagerImpl::test_cell_intersection(const Coord2D& coords, const math::line3& line) const
{
    const auto bbox = w3e_map()->calc_cell_bbox(coords.x, coords.y);
    return bbox.test_intersection(line);
}

std::optional<math::vector3>
W3MapRuntimeManagerImpl::get_cell_intersection_point(const Coord2D& coords, const math::line3& line) const
{
    const auto bbox = w3e_map()->calc_cell_bbox(coords.x, coords.y);
    if (bbox.test_intersection(line)) {
        const math::vector3 vertex0 = get_cellpoint_position(coords);
        const math::vector3 vertex1 = get_cellpoint_position({ coords.x + 1,  coords.y     });
        const math::vector3 vertex2 = get_cellpoint_position({ coords.x + 1,  coords.y + 1 });
        const math::vector3 vertex3 = get_cellpoint_position({ coords.x,      coords.y + 1 });

        const auto triangles = std::to_array<math::triangle>({
                { vertex0, vertex1, vertex2 },
                { vertex0, vertex2, vertex3 }
            });

        for(const auto& tri : triangles) {
            math::vector3 intersection_point;
            if (tri.intersect(line, intersection_point)) {
                return std::make_optional(intersection_point);
            }
        }
    }
    return std::nullopt;
}

void
W3MapRuntimeManagerImpl::update_runtime_ground(const auto& cell_info, CellPointRT& cell_rt) const
{
    cell_rt.uv_indices = 0;
    cell_rt.tileset_ids = 0;

    uint32_t current_layer_mask = 0;
    for(size_t i = 0; i < cell_info.ground_layers.size(); ++i) {
        const auto& layer = cell_info.ground_layers[i];

        if ((current_layer_mask & layer.mask) != 0) {
            cell_rt.set_ground_tileset_id(i);
            continue;
        }

        current_layer_mask |= layer.mask;

        cell_rt.set_ground_tileset_id(i, layer.tileset_id);
        cell_rt.flags |= CellPointRT::GROUND;

        const uint32_t tileset_id = layer.tileset_id;
        const bool is_extended = map_asset_->ground_asset_rt(tileset_id).is_extended;
        const uint8_t layer_mask = i != 0 ? layer.mask : 15; // fixed tile at first layer
        const uint32_t variation = (cell_info.key >> 16U) & 0b11111U;

        auto [tile_idx_u, tile_idx_v] = calc_tile_texture_indices_from_layer_code(is_extended, layer_mask, variation);
        cell_rt.set_ground_tile_uv_indices(i, tile_idx_u, tile_idx_v);
    }
}

void
W3MapRuntimeManagerImpl::update_runtime_cliff(const auto& cell_info, CellPointRT& cell_rt) const
{
    const uint32_t tileset_id = cell_info.geo_tileset;
    const uint16_t geoset_id = get_cliff_geoset_id_for_geokey(tileset_id, cell_info.key);

    cell_rt.flags |= CellPointRT::GEO_CLIFF;
    cell_rt.geoset_id = geoset_id;
    cell_rt.tileset_id = tileset_id;

    // get info about the cliff mesh
    const W3Mesh *mesh_ptr = map_asset_->geo_asset_rt(tileset_id).cliff_geoset_mesh.ptr();
    if (mesh_ptr == nullptr) {
        w3_log_error("Cliff geoset mesh is null");
        return;
    }

    const int32_t surfaces = mesh_ptr->get_surface_count();
    if (geoset_id >= surfaces) {
        w3_log_error("Geoset id %d is more than the number(%d) of surfaces in cliff geoset.", geoset_id, surfaces);
        return;
    }

    const auto& geoset_surface = mesh_ptr->surface_get_arrays(geoset_id);
    const godot::PackedVector3Array& vertices = geoset_surface[W3Mesh::ARRAY_VERTEX];
    const godot::PackedInt32Array& indices = geoset_surface[W3Mesh::ARRAY_INDEX];

    // update precached mesh
    cell_rt.vertices_count = vertices.size();
    cell_rt.indices_count = indices.size();
}

void
W3MapRuntimeManagerImpl::update_runtime_ramp(const auto& cell_info, CellPointRT& cell_rt) const
{
    const uint32_t tileset_id = cell_info.geo_tileset;
    const uint16_t geoset_id = get_ramp_geoset_id_for_geokey(cell_info.geo_tileset, cell_info.key);

    cell_rt.flags |= CellPointRT::GEO_RAMP;
    cell_rt.geoset_id = geoset_id;
    cell_rt.tileset_id = tileset_id;

    // get info about the ramp mesh
    const W3Mesh *mesh_ptr = map_asset_->geo_asset_rt(tileset_id).ramp_geoset_mesh.ptr();
    if (mesh_ptr == nullptr) {
        w3_log_error("Ramp geoset mesh is null");
        return;
    }

    const int32_t surfaces = mesh_ptr->get_surface_count();
    if (geoset_id >= surfaces) {
        w3_log_error("Geoset id %d is more than the number(%d) of surfaces in ramp geoset.", geoset_id, surfaces);
        return;
    }

    const auto& geoset_surface = mesh_ptr->surface_get_arrays(geoset_id);
    const godot::PackedVector3Array& vertices = geoset_surface[W3Mesh::ARRAY_VERTEX];
    const godot::PackedInt32Array& indices = geoset_surface[W3Mesh::ARRAY_INDEX];

    // update precached mesh
    cell_rt.vertices_count = vertices.size();
    cell_rt.indices_count = indices.size();
}

void
W3MapRuntimeManagerImpl::update_cell_rt(const Coord2D& coords)
{
    const W3MapInformatorImpl::W3CPInfo cell_info = informator_->collect_cellpoint_info(coords);
    CellPointRT& cell_rt = get_cellpoint_rt(coords);
    cell_rt = {};

    cell_rt.packed_normal = pack_vector3_to_32bit(cell_info.normal);
    cell_rt.flags = 0;

    if (cell_info.check_flag(W3MapInformatorImpl::W3CPInfo::RAMP_MIDDLE)) {
        cell_rt.flags |= CellPointRT::RAMP_MIDDLE;
    }

    if (cell_info.check_flag(W3MapInformatorImpl::W3CPInfo::GROUND)) {
        update_runtime_ground(cell_info, cell_rt);
    } else if (cell_info.check_flag(W3MapInformatorImpl::W3CPInfo::GEOCLIFF)) {
        update_runtime_cliff(cell_info, cell_rt);
    } else if (cell_info.check_flag(W3MapInformatorImpl::W3CPInfo::GEORAMP)) {
        update_runtime_ramp(cell_info, cell_rt);
    }

    // update water info
    if (cell_info.check_flag(W3MapInformatorImpl::W3CPInfo::WATER)) {
        cell_rt.flags |= CellPointRT::WATER;
    }

    set_dirty(true);
}

void
W3MapRuntimeManagerImpl::update_all_cells_rt()
{
    const auto map_size_x = w3e_map()->get_map_2d_size_x();
    const auto map_size_y = w3e_map()->get_map_2d_size_y();
    const size_t runtime_array_size = static_cast<size_t>(map_size_x) * map_size_y;
    cellpoints_rt_.resize(runtime_array_size);

    for(int32_t idx_2d_y = 0; idx_2d_y < map_size_y; ++idx_2d_y) {
        for(int32_t idx_2d_x = 0; idx_2d_x < map_size_x; ++idx_2d_x) {
            update_cell_rt({ idx_2d_x, idx_2d_y });
        }
    }
}

void
W3MapRuntimeManagerImpl::update_area_rt(const Coord2D& coords, int32_t area_margin)
{
    for(int32_t idx_2d_y = coords.y - area_margin; idx_2d_y <= coords.y + area_margin; ++idx_2d_y) {
        for(int32_t idx_2d_x = coords.x - area_margin; idx_2d_x <= coords.x + area_margin; ++idx_2d_x) {
            if (!w3e_map()->is_valid_cell(idx_2d_x, idx_2d_y)) {
                continue;
            }
            update_cell_rt({ idx_2d_x, idx_2d_y });
        }
    }
}

}  // namespace w3terr