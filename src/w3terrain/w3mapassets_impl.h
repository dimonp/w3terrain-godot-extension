#ifndef _W3MAPASSETS_IMPL__H
#define _W3MAPASSETS_IMPL__H

#include <godot_cpp/classes/json.hpp>

#include "w3mapassets.h"
#include "w3defs.h"
#include "w3eresource.h"

namespace w3terr {

struct W3GeoResource : godot::Resource
{
	GDCLASS(W3GeoResource, Resource)
public:
    void set_geoset_config(const W3Ref<godot::JSON>& config) { geoset_config_ = config; emit_changed(); }
    W3Ref<godot::JSON> get_geoset_config() const { return geoset_config_; }

    void set_texture(const W3Ref<W3Texture>& texture) { this->texture_ = texture; emit_changed(); }
    W3Ref<W3Texture> get_texture() const { return texture_; };

    void set_cliff_geoset_mesh(const W3Ref<W3Mesh>& mesh) { cliff_geoset_mesh_ = mesh; emit_changed(); }
    W3Ref<W3Mesh> get_cliff_geoset_mesh() const { return cliff_geoset_mesh_; }

    void set_ramp_geoset_mesh(const W3Ref<W3Mesh>& mesh) { ramp_geoset_mesh_ = mesh; emit_changed(); }
    W3Ref<W3Mesh> get_ramp_geoset_mesh() const { return ramp_geoset_mesh_; }

    void set_ground_tileset_id(uint32_t tileset_id) { ground_tileset_id_ = tileset_id; emit_changed(); }
    uint32_t get_ground_tileset_id() const { return ground_tileset_id_; }

protected:
    static void _bind_methods();

private:
    W3Ref<godot::Resource> geoset_config_;
    W3Ref<W3Texture> texture_;
    W3Ref<W3Mesh> cliff_geoset_mesh_;
    W3Ref<W3Mesh> ramp_geoset_mesh_;
    uint32_t ground_tileset_id_ = 0;
};

class W3MapAssetsImpl : public W3MapAssets
{
public:
    const W3e* get_w3e() const override;

protected:
    godot::TypedArray<W3Texture> get_ground_assets() const;
    bool set_ground_assets(const godot::TypedArray<W3Texture>& assets);

    void prepare_geo_assets_rt();

    bool load_geo_config(size_t tileset_id, const W3Ref<godot::JSON>& config);
    static bool parse_geoset_resource(const godot::Dictionary& geoset, W3HashMap<uint32_t, uint32_t> &geo_keys_map);

    W3Ref<W3eResource> map_w3e_;
    godot::TypedArray<W3GeoResource> geo_assets_;
};

inline
const W3e*
W3MapAssetsImpl::get_w3e() const
{
    return map_w3e_.ptr();
}


}  // namespace w3terr

#endif  /// _W3MAPASSETS_IMPL__H

