#ifndef _W3MAPASSETS__H
#define _W3MAPASSETS__H


#include <godot_cpp/classes/json.hpp>

#include "w3defs.h"
#include "w3e.h"

namespace w3terr {

class W3_API W3MapAssets {
public:
    virtual ~W3MapAssets() = default;

    struct GroundAsset {
        W3Ref<W3Texture> texture;
        float tile_tu_size, tile_tv_size;
        bool is_extended;
    };

    struct GeoAsset {
        W3Ref<W3Mesh> cliff_geoset_mesh;
        W3HashMap<uint32_t, uint32_t> geo_cliff_keys_map;
        W3Ref<W3Mesh> ramp_geoset_mesh;
        W3HashMap<uint32_t, uint32_t> geo_ramp_keys_map;
        W3Ref<W3Texture> texture;
        uint32_t ground_tileset_id = 0;
    };

    virtual const W3e* get_w3e() const = 0;

    const W3Array<GroundAsset>& ground_assets_rt() const;
    const GroundAsset& ground_asset_rt(size_t index) const;
    uint32_t ground_assets_size_rt() const;

    const W3Array<GeoAsset>& geo_assets_rt() const;
    const GeoAsset& geo_asset_rt(size_t index) const;
    uint32_t geo_assets_size_rt() const;

    bool is_map_loaded() const;
    bool is_assets_dirty() const;

protected:
    W3Array<GroundAsset> ground_assets_rt_;
    W3Array<GeoAsset> geo_assets_rt_;

    bool assets_dirty_flag_ = true;
};

inline
bool
W3MapAssets::is_map_loaded() const
{
    return get_w3e() != nullptr;
}

inline
const W3Array<W3MapAssets::GroundAsset>&
W3MapAssets::ground_assets_rt() const
{
    return ground_assets_rt_;
}

inline
const W3Array<W3MapAssets::GeoAsset>&
W3MapAssets::geo_assets_rt() const
{
    return geo_assets_rt_;
}

inline
bool
W3MapAssets::is_assets_dirty() const
{
    return assets_dirty_flag_;
}

inline
uint32_t
W3MapAssets::ground_assets_size_rt() const
{
    return ground_assets_rt_.size();
}

inline
const W3MapAssets::GroundAsset&
W3MapAssets::ground_asset_rt(size_t index) const
{
    return ground_assets_rt_[index];
}

inline
uint32_t
W3MapAssets::geo_assets_size_rt() const
{
    return geo_assets_rt_.size();
}

inline
const W3MapAssets::GeoAsset&
W3MapAssets::geo_asset_rt(size_t index) const
{
    return geo_assets_rt_[index];
}


}  // namespace w3terr

#endif  /// _W3MAPASSETS__H

