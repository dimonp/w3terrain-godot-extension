#include "w3mapassets_impl.h"

namespace w3terr {

void
W3GeoResource::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_geoset_config"), &W3GeoResource::get_geoset_config);
    godot::ClassDB::bind_method(godot::D_METHOD("set_geoset_config", "config"), &W3GeoResource::set_geoset_config);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "geoset_config", godot::PROPERTY_HINT_RESOURCE_TYPE, "JSON"),
        "set_geoset_config", "get_geoset_config");

    godot::ClassDB::bind_method(godot::D_METHOD("get_texture"), &W3GeoResource::get_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_texture", "texture"), &W3GeoResource::set_texture);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture",godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"),
        "set_texture", "get_texture");

    godot::ClassDB::bind_method(godot::D_METHOD("get_cliff_geoset_mesh"), &W3GeoResource::get_cliff_geoset_mesh);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cliff_geoset_mesh", "cliff_geoset_mesh"), &W3GeoResource::set_cliff_geoset_mesh);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cliff_geoset_mesh",godot::PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_cliff_geoset_mesh", "get_cliff_geoset_mesh");

    godot::ClassDB::bind_method(godot::D_METHOD("get_ramp_geoset_mesh"), &W3GeoResource::get_ramp_geoset_mesh);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ramp_geoset_mesh", "ramp_geoset_mesh"), &W3GeoResource::set_ramp_geoset_mesh);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "ramp_geoset_mesh",godot::PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_ramp_geoset_mesh", "get_ramp_geoset_mesh");

    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_id"), &W3GeoResource::get_ground_tileset_id);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ground_id", "ground_id"), &W3GeoResource::set_ground_tileset_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "ground_id", godot::PROPERTY_HINT_RANGE, "0,255"),
        "set_ground_id", "get_ground_id");
}

godot::TypedArray<W3Texture>
W3MapAssetsImpl::get_ground_assets() const
{
    godot::TypedArray<W3Texture> result;
    for(const auto& asset : ground_assets_rt_) {
        result.push_back(asset.texture);
    }
    return result;
}

bool
W3MapAssetsImpl::set_ground_assets(const godot::TypedArray<W3Texture>& assets)
{
    const size_t ground_tileset_size = assets.size();
    ground_assets_rt_.clear();
    ground_assets_rt_.resize(ground_tileset_size);
    assets_dirty_flag_ = true;

    // load ground resources
    for(int64_t i = 0; i < ground_tileset_size; ++i) {
        W3Ref<W3Texture> ground_texture = assets[i];
        if (!ground_texture.is_valid()) {
            w3_log_error("Ground texture %d is not defined.", i);
            continue;
        }

        w3_log_debug("Loaded ground texture %d: %s", i, ground_texture);

        auto& ground_type_rt = ground_assets_rt_[i];
        ground_type_rt.tile_tu_size = kGroundTextureTileSize / static_cast<float>(ground_texture->get_width());
        ground_type_rt.tile_tv_size = kGroundTextureTileSize / static_cast<float>(ground_texture->get_height());
        ground_type_rt.is_extended = ground_texture->get_width() > ground_texture->get_height();
        ground_type_rt.texture = ground_texture;
    }
    return true;
}

void
W3MapAssetsImpl::prepare_geo_assets_rt()
{
    const size_t geo_tileset_size = geo_assets_.size();
    geo_assets_rt_.clear();
    geo_assets_rt_.resize(geo_tileset_size);
    assets_dirty_flag_ = true;

    // load cliff textures
    for(int64_t i = 0; i < geo_tileset_size; ++i) {
        W3Ref<W3GeoResource> geo_resource = geo_assets_[i];
        if (!geo_resource.is_valid()) {
            w3_log_error("Geo resource %d is not defined.", i);
            continue;
        }

        w3_log_debug("Loaded geo texture %d: %s", i, geo_resource->get_texture());

        auto& geo_tileset_rt = geo_assets_rt_[i];
        geo_tileset_rt.texture = geo_resource->get_texture();
        geo_tileset_rt.cliff_geoset_mesh = geo_resource->get_cliff_geoset_mesh();
        geo_tileset_rt.ramp_geoset_mesh = geo_resource->get_ramp_geoset_mesh();
        geo_tileset_rt.ground_tileset_id = geo_resource->get_ground_tileset_id();

        load_geo_config(i, geo_resource->get_geoset_config());

        const auto& cliff_mesh = geo_resource->get_cliff_geoset_mesh();
        if (cliff_mesh.is_null()) {
            w3_log_error("Cliff mesh %d is not defined.", i);
            continue;
        }

        const auto& ramp_mesh = geo_resource->get_ramp_geoset_mesh();
        if (ramp_mesh.is_null()) {
            w3_log_error("Ramp mesh %d is not defined.", i);
            continue;
        }

        if (geo_tileset_rt.geo_cliff_keys_map.size() != cliff_mesh->get_surface_count()) {
            w3_log_error("Cliff mesh %d has %d surfaces, but %d cliff keys in config.",
                i,
                static_cast<size_t>(cliff_mesh->get_surface_count()),
                geo_tileset_rt.geo_cliff_keys_map.size());
            continue;
        }

        if (geo_tileset_rt.geo_ramp_keys_map.size() != ramp_mesh->get_surface_count()) {
            w3_log_error("Ramp mesh %d has %d surfaces, but %d ramp keys in config",
                i,
                static_cast<size_t>(cliff_mesh->get_surface_count()),
                geo_tileset_rt.geo_ramp_keys_map.size());
            continue;
        }
    }
}

bool
W3MapAssetsImpl::load_geo_config(size_t tileset_id, const W3Ref<godot::JSON>& config)
{
    if (config.is_null()) {
        return false;
    }

    W3MapAssetsImpl::GeoAsset &geo_type_rt = this->geo_assets_rt_[tileset_id];

    const godot::Variant data = config->get_data();
    if (data.get_type() != godot::Variant::DICTIONARY) {
        w3_log_error("nTileMapNode::load_cliffs_config: unknown config format.");
        return false;
    }

    const godot::Dictionary geo_dict_data = data;
    for (const auto& key : geo_dict_data.keys()) {
        const godot::Variant& value_data = geo_dict_data[key];

        if (value_data.get_type() != godot::Variant::DICTIONARY) {
            w3_log_error("Geoset element not found.");
            return false;
        }

        const godot::Dictionary geoset_dict_data = value_data;
        const godot::String& geoset_type = static_cast<godot::String>(geoset_dict_data["type"]);
        const godot::String& geoset_mesh = static_cast<godot::String>(geoset_dict_data["mesh"]);

        if (geoset_type == "cliffs" && !geoset_mesh.is_empty()) {
            if (!parse_geoset_resource(geoset_dict_data, geo_type_rt.geo_cliff_keys_map)) {
                w3_log_error("nTileMapNode::load_cliffs_config: can't read cliffs geoset.");
                return false;
            }

        } else if (geoset_type == "ramps" && !geoset_mesh.is_empty()) {
            if (!parse_geoset_resource(geoset_dict_data, geo_type_rt.geo_ramp_keys_map)) {
                w3_log_error("nTileMapNode::load_cliffs_config: can't read ramps geoset.");
                return false;
            }
        }
    }
    return true;
}

bool
W3MapAssetsImpl::parse_geoset_resource(const godot::Dictionary& geoset, W3HashMap<uint32_t, uint32_t> &geo_keys_map)
{
    size_t geoset_groups_count = static_cast<size_t>(geoset["count"]);
    if (geoset_groups_count == 0) {
        w3_log_error("XML geo config file error.");
        return false;
    }

    geo_keys_map.reserve(geoset_groups_count);

    const godot::Variant gropups_data = geoset["groups"];
    if (gropups_data.get_type() != godot::Variant::DICTIONARY) {
        w3_log_error("Geoset gropus not found.");
        return false;
    }

    const godot::Dictionary groups_dict_data = gropups_data;
    for (const auto& mesh_idx : groups_dict_data.keys()) {

        const godot::Variant& key_data = groups_dict_data[mesh_idx];
        const godot::Variant key = static_cast<uint32_t>(key_data);
        geo_keys_map[key] = static_cast<uint32_t>(mesh_idx);

        --geoset_groups_count;
    }

    if (geoset_groups_count != 0) {
        w3_log_error("Geo config file error. Wrong geo groups count.");
        return false;
    }
    return true;
}

}  // namespace w3terr