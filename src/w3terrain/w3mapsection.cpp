#include <algorithm>
#include <lru_memory_manager/lrumemorymanager.h>

#include "w3mapsection.h"
#include "w3mapruntimemanager_impl.h"

namespace w3terr {

using LRUHandle = lrumm::LRUMemoryManager::LRUMemoryHandle;

lrumm::LRUMemoryManager* get_lrumm_instance();

W3MapSection::CachedMesh::CachedMesh() noexcept
{
    static_assert(sizeof(LRUHandle) <= kPimplSize, "W3MapSection::CachedMesh Buffer too small!");
    static_assert(alignof(LRUHandle) <= kPimplAlign, "W3MapSection::CachedMesh Wrong alignment!");
    new (storage_.data()) LRUHandle();
}

W3MapSection::CachedMesh::~CachedMesh() noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    LRUHandle* cache_mem_handle = reinterpret_cast<LRUHandle*>(storage_.data());
    cache_mem_handle->~LRUHandle();
}

auto*
W3MapSection::CachedMesh::get_cache_handle() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::launder(reinterpret_cast<LRUHandle*>(storage_.data()));
}


const void*
W3MapSection::CachedMesh::get_cached_buffer() const
{
    LRUHandle* cache_mem_handle = get_cache_handle();
    return get_lrumm_instance()->get_buffer_and_refresh(cache_mem_handle);
}

void*
W3MapSection::CachedMesh::alloc_cached_buffer(const size_t alloc_size) const
{
    LRUHandle* cache_mem_handle = get_cache_handle();
    return get_lrumm_instance()->alloc(cache_mem_handle, alloc_size);
}

void
W3MapSection::initialize(
    size_t ground_tilesets_size,
    size_t geo_tilesets_size,
    const Coord2D& origin_2d,
    const math::bbox3& bbox)
{
    ground_tileset_to_layer_map_.assign(ground_tilesets_size, {});
    ground_cached_meshes_.assign(ground_tilesets_size, {});
    geo_cached_meshes_.assign(geo_tilesets_size, {});
    origin_2d_ = origin_2d;
    bbox_ = bbox;
}

void
W3MapSection::free_cached_data()
{
    auto& lrumm = *get_lrumm_instance();

    // free ground cache data
    for(size_t tileset_id = 0; tileset_id < get_ground_tilesets_size(); ++tileset_id) {
        W3MapSection::CachedMesh &ts_desc = ground_cached_meshes_[tileset_id];

        LRUHandle* cache_mem_handle = ts_desc.get_cache_handle();
        // already cached ?
        if (lrumm.get_buffer_and_refresh(cache_mem_handle) != nullptr) {
            lrumm.free(cache_mem_handle);
        }
    }

    // free geo cache data
    for(size_t tileset_id = 0; tileset_id < get_geo_tilesets_size(); ++tileset_id) {
        W3MapSection::CachedMesh &ts_desc = geo_cached_meshes_[tileset_id];

        LRUHandle* cache_mem_handle = ts_desc.get_cache_handle();
        // already cached ?
        if (lrumm.get_buffer_and_refresh(cache_mem_handle) != nullptr) {
            lrumm.free(cache_mem_handle);
        }
    }

    LRUHandle* cache_mem_handle = water_mesh_.get_cache_handle();
    //free water cache data
    if (lrumm.get_buffer_and_refresh(cache_mem_handle) != nullptr) {
        lrumm.free(cache_mem_handle);
    }
}

void
W3MapSection::update_cell(const size_t cell_idx)
{
    const Coord2D cell_coord = calc_cell_coord_from_idx(cell_idx);
    const W3MapRuntimeManagerImpl::CellPointRT& cell_rt = runtime_manager_->get_cellpoint_rt(cell_coord);

    // mark cell as unusable for all geoset ground meshes
    for(size_t tileset_id = 0; tileset_id < get_ground_tilesets_size(); ++tileset_id) {
        W3MapSection::CachedMesh &cached_mesh = ground_cached_meshes_[tileset_id];
        cached_mesh.usage_flags_.set(cell_idx, false);
    }

    // mark cell as unusable for all geo meshes
    for(size_t tileset_id = 0; tileset_id < get_geo_tilesets_size(); ++tileset_id) {
        W3MapSection::CachedMesh &cached_mesh = geo_cached_meshes_[tileset_id];
        cached_mesh.usage_flags_.set(cell_idx, false);
    }

    if (cell_rt.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GROUND)) {  // ground cellpoint
        for(size_t layer_idx = 0; layer_idx < W3MapRuntimeManagerImpl::CellPointRT::kMaxGroundLayers; ++layer_idx) {

            size_t tileset_id = cell_rt.get_ground_tileset_id(layer_idx);
            if (tileset_id == W3MapRuntimeManagerImpl::CellPointRT::kEmptyTilesetId) {
                continue;
            }

            // init cells
            W3MapSection::CachedMesh &ground_mesh = ground_cached_meshes_[tileset_id];
            ground_mesh.usage_flags_.set(cell_idx, true);
            ground_tileset_to_layer_map_[tileset_id] |= layer_idx << (cell_idx << 1U);

            // update precached mesh
            ground_mesh.vertices_count_ += 4;
            ground_mesh.indices_count_ += 6;
        }
    } else if (cell_rt.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_CLIFF)) { // geo cellpoint (cliffs)
        const size_t tileset_id = cell_rt.tileset_id;
        W3MapSection::CachedMesh &cliff_mesh = geo_cached_meshes_[tileset_id];
        cliff_mesh.usage_flags_ |= (1U << cell_idx);

        // update precached mesh
        cliff_mesh.vertices_count_ += cell_rt.vertices_count;
        cliff_mesh.indices_count_ += cell_rt.indices_count;
    } else if (cell_rt.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_RAMP)) { // geo cellpoint (ramps)
        const size_t tileset_id = cell_rt.tileset_id;
        W3MapSection::CachedMesh &ramp_mesh = geo_cached_meshes_[tileset_id];
        ramp_mesh.usage_flags_.set(cell_idx, true);

        // update precached mesh
        ramp_mesh.vertices_count_ += cell_rt.vertices_count;
        ramp_mesh.indices_count_ += cell_rt.indices_count;
    }

    // update water info
    if (cell_rt.check_flag(W3MapRuntimeManagerImpl::CellPointRT::WATER)) {
        water_mesh_.usage_flags_.set(cell_idx, true);

        // update precached mesh
        water_mesh_.vertices_count_ += 4;
        water_mesh_.indices_count_ += 6;
    }
}

void
W3MapSection::update_all_cells()
{
    std::ranges::fill(ground_tileset_to_layer_map_, 0);
    std::ranges::fill(ground_cached_meshes_, W3MapSection::CachedMesh {});
    std::ranges::fill(geo_cached_meshes_, W3MapSection::CachedMesh {});
    water_mesh_ = W3MapSection::CachedMesh {};
    for(uint32_t cell_idx = 0; cell_idx < kNumberOfCells; ++cell_idx) {
        update_cell(cell_idx);
    }
}

std::optional<Coord2D>
W3MapSection::find_intersected_cell(const math::line3 &line, math::vector3& ipoint) const
{
    for(uint32_t cell_idx = 0; cell_idx < kNumberOfCells; ++cell_idx) {
        const Coord2D cell_coord = calc_cell_coord_from_idx(cell_idx);

        auto point_opt = runtime_manager_->get_cell_intersection_point(
            cell_coord, line);

        if (point_opt.has_value()) {
            ipoint = point_opt.value();
            return std::make_optional<Coord2D>(cell_coord.x, cell_coord.y);
        }
    }
    return std::nullopt;
}

bool
W3MapSection::refresh()
{
    if (is_dirty()) {
        free_cached_data();
        update_all_cells();
        set_dirty(false);
        return true;
    }
    return false;
}

}  // namespace w3terr