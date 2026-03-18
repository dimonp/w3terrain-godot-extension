#include <lru_memory_manager/lrumemorymanager.h>

#include "w3mapassets.h"
#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"


namespace w3terr {

// NOLINTNEXTLINE(misc-use-internal-linkage)
lrumm::LRUMemoryManager* get_lrumm_instance()
{
    static lrumm::LRUMemoryManager lru_memory_manager;
    return &lru_memory_manager;
}

W3MapSectionManagerImpl::W3MapSectionManagerImpl(
    const W3MapAssets* assets,
    W3MapRuntimeManager* runtime
) : runtime_(runtime) , assets_(assets)
{
    update_all_sections();
}

W3MapSectionManagerImpl::~W3MapSectionManagerImpl() noexcept
{
    get_lrumm_instance()->arena_clean();
}

inline
W3MapSectionManagerImpl::SectionId
W3MapSectionManagerImpl::get_section_id_from_coord(int32_t section_2d_x, int32_t section_2d_y) const
{
    w3_assert(is_valid_section_coord(section_2d_x, section_2d_y));
    return (section_2d_y * sections_2d_x_size_) + section_2d_x + 1;
}

void
W3MapSectionManagerImpl::update_all_sections()
{
    const auto* map_w3e = assets_->get_w3e();
    sections_2d_x_size_ = map_w3e->get_map_2d_size_x() / W3MapSection::kCellsDimension;
    sections_2d_y_size_ = map_w3e->get_map_2d_size_y() / W3MapSection::kCellsDimension;

    const size_t sections_array_size = static_cast<size_t>(sections_2d_x_size_) * sections_2d_y_size_;
    sections_array_.assign(sections_array_size, W3MapSection { runtime_ } );

    // loop and initialize all sections
    for(const auto section_id : *this) {
        const Coord2D origin = calc_section_origin(section_id);
        const math::bbox3 section_bbox = map_w3e->calc_cellpoints_bbox(
            origin.x,
            origin.y,
            W3MapSection::kCellsDimension);

        W3MapSection& section = get_section_by_id(section_id);
        section.initialize(
            map_w3e->get_ground_tilesets_count(),
            map_w3e->get_geo_tilesets_count(),
            origin,
            section_bbox);
    }
}

void
W3MapSectionManagerImpl::set_dirty_all()
{
    for(const auto section_id : *this) {
        get_section_by_id(section_id).set_dirty();
    }
}

Coord2D
W3MapSectionManagerImpl::calc_section_origin(SectionId section_id) const
{
    return {
        static_cast<int32_t>((section_id - 1) % sections_2d_x_size_) * W3MapSection::kCellsDimension,
        static_cast<int32_t>((section_id - 1) / sections_2d_x_size_) * W3MapSection::kCellsDimension
    };
}

void
W3MapSectionManagerImpl::invalidate_sections_at_cellpoint(const Coord2D& coords)
{
    // update neighbour cells
    static constexpr int32_t kAreaMargin = 2;
    runtime_->update_area_rt(static_cast<Coord2D>(coords), kAreaMargin);

    const int32_t section_2d_x = coords.x / W3MapSection::kCellsDimension;
    const int32_t section_2d_y = coords.y / W3MapSection::kCellsDimension;
    static constexpr int32_t kSectionOffset = 1;
    for(int32_t idx_2d_y = section_2d_y - kSectionOffset; idx_2d_y <= section_2d_y + kSectionOffset; ++idx_2d_y) {
        for(int32_t idx_2d_x = section_2d_x - kSectionOffset; idx_2d_x <= section_2d_x + kSectionOffset; ++idx_2d_x) {
            if (!is_valid_section_coord(idx_2d_y, idx_2d_x)) {
                continue;
            }
            SectionId section_id = get_section_id_from_coord(idx_2d_x, idx_2d_y);
            get_section_by_id(section_id).set_dirty();
        }
    }
}

uint64_t
W3MapSectionManagerImpl::get_cache_allocation_size()
{
    return get_lrumm_instance()->get_allocated_memory_size();
}


}  // namespace w3terr