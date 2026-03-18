#include "w3mapinformator_impl.h" // IWYU pragma: keep
#include "w3mapruntimemanager_impl.h"
#include "w3mapcollector_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapnode.h"

namespace w3terr {

bool
W3MapNode::refresh_runtime()
{
    return runtime_manager_ || load_map();
}

bool
W3MapNode::create_empty_map(int32_t dim_2d_x, int32_t dim_2d_y)
{
    reset_runtime();
    get_w3e_resource().unref();
    get_w3e_resource().instantiate();
    return get_w3e_resource()->create_empty(dim_2d_x, dim_2d_y, 1, 1);
}

bool
W3MapNode::load_map()
{
    w3_assert(is_map_loaded());

    if (get_w3e()->get_ground_tilesets_count() > ground_assets_size_rt()) {
        w3_log_error("The number of ground textures(%d) is less than the number of map ground assets(%d).",
            get_w3e()->get_ground_tilesets_count(), ground_assets_size_rt());
        return false;
    }
    if (get_w3e()->get_geo_tilesets_count() > geo_assets_size_rt()) {
        w3_log_error("The number of geo resources(%d) is less than the number of map geo assets(%d).",
            get_w3e()->get_geo_tilesets_count(), geo_assets_size_rt());
        return false;
    }

    const int32_t map_2d_size_x = get_w3e()->get_map_2d_size_x();
    const int32_t map_2d_size_y = get_w3e()->get_map_2d_size_y();

    if ((map_2d_size_x - 1) % W3MapSection::kCellsDimension != 0) {
        w3_log_error("Map size must be divided by section size.");
        return false;
    }

    if ((map_2d_size_y - 1) % W3MapSection::kCellsDimension != 0) {
        w3_log_error("Map size must be divided by section size.");
        return false;
    }

    set_transform({
        godot::Basis(),
        {
            get_w3e()->get_map_3d_offset_x(),
            0.0F,
            -get_w3e()->get_map_3d_offset_z() }
    });

    runtime_manager_ = std::make_unique<W3MapRuntimeManagerImpl>(get_assets(), informator_.get());
    sections_manager_ = std::make_unique<W3MapSectionManagerImpl>(get_assets(), runtime_manager_.get());

    math::bbox3 root_bbox;
    root_bbox.begin_extend();
    for(const auto section_id : *sections_manager_) {
        const auto section_bbox = sections_manager_->get_section_by_id(section_id).get_bbox();
        root_bbox.extend(section_bbox);
    }

    // Extend root bounding box to power of two size (192 -> 256, 384 -> 512)
    // This is necessary for the quadtree to work with maps whose size is not a power of two.
    // Also make the root bbox square by xz with the maximum size (to support non-square cards)
    const int32_t ext_map_size_x = math::lower_bound_power_of_two(get_w3e()->get_map_2d_size_x() - 1) + 1;
    const int32_t ext_map_size_y = math::lower_bound_power_of_two(get_w3e()->get_map_2d_size_y() - 1) + 1;

    math::vector3 ext_cell_position_x = W3eResource::map_cellpoint_position_2d_to_3d(
        ext_map_size_x - 1, ext_map_size_x - 1);
    math::vector3 ext_cell_position_z = W3eResource::map_cellpoint_position_2d_to_3d(
        ext_map_size_y - 1, ext_map_size_y - 1);

    const float cell00_height = get_w3e()->get_cellpoint_total_height(0, 0);
    ext_cell_position_x.y = cell00_height;
    root_bbox.extend(ext_cell_position_x);
    ext_cell_position_z.y = cell00_height;
    root_bbox.extend(ext_cell_position_z);

    const uint8_t quad_tree_depth = static_cast<uint8_t>(std::log2(std::max(ext_map_size_x, ext_map_size_y) - 1)) - 1;
    collector_ = std::make_unique<W3MapCollectorImpl>(
        sections_manager_.get(),
        root_bbox,
        quad_tree_depth
    );

    w3_log_info("W3MapNode::load_map map loaded: %s", map_w3e_->get_path());

    emit_signal(kSignalMapInitialized, this);
    return true;
}

void
W3MapNode::reset_runtime()
{
    collector_.reset();
    sections_manager_.reset();
    runtime_manager_.reset();
    assets_dirty_flag_ = true;
    emit_signal(kSignalMapDestroyed, this);
}

}  // namespace w3terr