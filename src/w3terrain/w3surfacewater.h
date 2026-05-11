#ifndef _W3MAPSURFACE_WATER__H
#define _W3MAPSURFACE_WATER__H

#include "w3surface.h"

namespace w3terr {

class W3MapRuntimeManagerImpl;

class W3SurfaceWater final: public W3Surface {
    GDCLASS(W3SurfaceWater, W3Surface)
public:
    void _process(double delta) override;

    W3Ref<W3Marerial> get_water_material() const;
    void set_water_material(const W3Ref<W3Marerial>& material);

    void _enter_tree() override;
    void _exit_tree() override;

protected:
    static void _bind_methods();

private:
#ifdef W3MAP_STATS_ENABLE
    static constexpr auto kStatWaterTilesRenderedId = "W3Terrain/stat_water_tiles_rendered";
#endif

    void render(const W3Array<uint32_t>& collected_sections);

    void render_section(uint32_t section_id);
    void render_section_cells(uint32_t section_id);

    W3Ref<W3Marerial> water_material_asset_;
#ifdef W3MAP_STATS_ENABLE
    uint64_t get_stat_water_tiles_rendered() const {
        return stat_water_tiles_rendered_;
    }
    mutable uint64_t stat_water_tiles_rendered_;
#endif
};

}  // namespace w3terr

#endif // _W3MAPSURFACE_WATER__H
