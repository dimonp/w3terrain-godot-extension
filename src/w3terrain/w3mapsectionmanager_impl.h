#ifndef _W3MAPSECTIONS_IMPL__H
#define _W3MAPSECTIONS_IMPL__H

#include <gsl/gsl>
#include "w3defs.h"
#include "w3mapsection.h"
#include "w3mapsectionmanager.h"

namespace w3terr {

class W3MapAssets;
class W3MapRuntimeManager;

class W3_API W3MapSectionManagerImpl final : public W3MapSectionManager {
public:
    W3MapSectionManagerImpl(const W3MapAssets* assets, W3MapRuntimeManager* runtime);
    ~W3MapSectionManagerImpl() noexcept override;

    SectionIdIterator begin() const override;
    SectionIdIterator end() const override;

    const W3MapSection& get_section_by_id(SectionId section_id) const override;
    W3MapSection& get_section_by_id(SectionId section_id) override;
    bool is_valid_section_id(SectionId section_id) const override;

    void update_all_sections() override;
    void set_dirty_all() override;
    void invalidate_sections_at_cellpoint(const Coord2D& coords) override;

    static uint64_t get_cache_allocation_size();

private:
    int32_t sections_2d_x_size_ = 0;
    int32_t sections_2d_y_size_ = 0;

    static size_t id_to_idx(uint32_t section_id);

    Coord2D calc_section_origin(SectionId section_id) const;
    SectionId get_section_id_from_coord(int32_t section_2d_x, int32_t section_2d_y) const;
    bool is_valid_section_coord(int32_t section_2d_x, int32_t section_2d_y) const;

    W3Array<W3MapSection> sections_array_;

    gsl::not_null<const W3MapAssets*> assets_;
    gsl::not_null<W3MapRuntimeManager*> runtime_;
};

inline
W3MapSectionManager::SectionIdIterator
W3MapSectionManagerImpl::begin() const
{
    return SectionIdIterator { 1 };
}

inline
W3MapSectionManager::SectionIdIterator
W3MapSectionManagerImpl::end() const
{
    return SectionIdIterator { static_cast<SectionId>(sections_array_.size() + 1) };
}

inline
size_t
W3MapSectionManagerImpl::id_to_idx(uint32_t section_id)
{
    return static_cast<size_t>(section_id - 1);
}

inline
bool
W3MapSectionManagerImpl::is_valid_section_coord(int32_t section_2d_x, int32_t section_2d_y) const
{
    return section_2d_x >= 0 && section_2d_y >= 0 &&
        section_2d_x < sections_2d_x_size_ && section_2d_y < sections_2d_y_size_;
}

inline
const W3MapSection&
W3MapSectionManagerImpl::get_section_by_id(uint32_t section_id) const
{
    w3_assert(section_id > 0 && section_id <= sections_array_.size());
    return sections_array_[id_to_idx(section_id)];
}

inline
W3MapSection&
W3MapSectionManagerImpl::get_section_by_id(SectionId section_id)
{
    w3_assert(is_valid_section_id(section_id));
    return sections_array_[id_to_idx(section_id)];
}

inline
bool
W3MapSectionManagerImpl::is_valid_section_id(SectionId section_id) const
{
    return section_id > 0 && section_id <= sections_array_.size();
}

}  // namespace w3terr

#endif // _W3MAPSECTIONS_IMPL__H
