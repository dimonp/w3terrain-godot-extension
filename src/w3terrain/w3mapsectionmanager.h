#ifndef _W3MAPSECTIONS__H
#define _W3MAPSECTIONS__H

#include "w3defs.h"
#include "w3mapsection.h"

namespace w3terr {

class W3MapSectionManager {
public:
    using SectionId = uint32_t;
    struct SectionIdIterator {
        explicit SectionIdIterator(SectionId section_id) : section_id(section_id) {}

        SectionId operator*() const { return section_id; }
        SectionIdIterator& operator++() { ++section_id; return *this; }
        auto operator <=>(const SectionIdIterator& other) const = default;

        SectionId section_id;
    };

    virtual ~W3MapSectionManager() = default;

    virtual SectionIdIterator begin() const = 0;
    virtual SectionIdIterator end() const = 0;

    virtual const W3MapSection& get_section_by_id(SectionId section_id) const = 0;
    virtual W3MapSection& get_section_by_id(SectionId section_id) = 0;
    virtual bool is_valid_section_id(SectionId section_id) const = 0;

    virtual void update_all_sections() = 0;
    virtual void set_dirty_all() = 0;
    virtual void invalidate_sections_at_cellpoint(const Coord2D& coords) = 0;
};

}  // namespace w3terr

#endif // _W3MAPSECTIONS__H
