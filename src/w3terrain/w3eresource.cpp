#include "w3eresource.h"

#include <gsl/gsl>

namespace w3terr {

void
W3eResource::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_size_x"), &W3eResource::size_x);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::INT, "size_x",
            godot::PROPERTY_HINT_NONE, "",
            godot::PROPERTY_USAGE_EDITOR | godot::PROPERTY_USAGE_READ_ONLY),
        "", "get_size_x");

    godot::ClassDB::bind_method(godot::D_METHOD("get_size_y"), &W3eResource::size_y);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::INT, "size_y",
            godot::PROPERTY_HINT_NONE, "",
            godot::PROPERTY_USAGE_EDITOR | godot::PROPERTY_USAGE_READ_ONLY),
        "", "get_size_y");

    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_tilesets_count"), &W3eResource::ground_tilesets_count);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::INT, "ground_tilesets_count",
            godot::PROPERTY_HINT_NONE, "",
            godot::PROPERTY_USAGE_EDITOR | godot::PROPERTY_USAGE_READ_ONLY),
        "", "get_ground_tilesets_count");

    godot::ClassDB::bind_method(godot::D_METHOD("get_geo_tilesets_count"), &W3eResource::geo_tilesets_count);
    ADD_PROPERTY(godot::PropertyInfo(
            godot::Variant::INT, "geo_tilesets_count",
            godot::PROPERTY_HINT_NONE, "",
            godot::PROPERTY_USAGE_EDITOR | godot::PROPERTY_USAGE_READ_ONLY),
        "", "get_geo_tilesets_count");
}

uint32_t
W3eResource::ground_tilesets_count() const
{
    return get_ground_tilesets_count();
}

uint32_t
W3eResource::geo_tilesets_count() const
{
    return get_geo_tilesets_count();
}

int32_t
W3eResource::size_x() const
{
    return get_map_2d_size_x();
}

int32_t
W3eResource::size_y() const
{
    return get_map_2d_size_y();
}

godot::Variant
W3eResourceLoader::load_w3e_file(const godot::Ref<godot::FileAccess>& file)
{
    godot::Ref<W3eResource> resource;
    resource.instantiate();

    W3String magic = file->get_buffer(4).get_string_from_utf8();
    if (magic != W3e::kW3eMagik) {
        return godot::Error::ERR_FILE_UNRECOGNIZED;
    }

    uint32_t version = file->get_32();
    if (version != W3e::kW3eVersion) {
        return godot::Error::ERR_FILE_UNRECOGNIZED;
    }

    resource->main_tileset_id_ = file->get_8();
    resource->custom_tileset_flag_ = file->get_32();

    resource->number_of_ground_tilesets_ = file->get_32();
    for(size_t i = 0; i < resource->number_of_ground_tilesets_; ++i) {
        file->get_32();
    }

    resource->number_of_geo_tilesets_ = file->get_32();
    for(size_t i = 0; i < resource->number_of_geo_tilesets_; ++i) {
        file->get_32();
    }

    resource->map_size_x_ = static_cast<int32_t>(file->get_32());
    resource->map_size_y_ = static_cast<int32_t>(file->get_32());

    resource->map_3d_offset_x_ = file->get_float();
    resource->map_3d_offset_z_ = file->get_float();

    resource->initialize();

    for(int32_t i = 0; i < resource->map_size_y_; ++i) {
        for(int32_t j = 0; j < resource->map_size_x_; ++j) {
            W3eCell &cell_point = resource->get_cellpoint(j, i);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto *p_dst = reinterpret_cast<uint8_t*>(&cell_point);
            uint64_t bytes = file->get_buffer(p_dst, sizeof(W3eCell));
            if (bytes != sizeof(W3eCell)) {
                w3_log_error("W3eResourceLoader::load_w3e_file: Read error!");
                return godot::Error::ERR_FILE_CORRUPT;
            }
        }
    }
    return resource;
}

godot::Error
W3eResourceSaver::save_w3e_file(const W3eResource* resource, const godot::Ref<godot::FileAccess>& file)
{
    if (!file.is_valid()) {
        return godot::Error::ERR_FILE_CANT_OPEN;
    }

    if (!file->store_string(W3e::kW3eMagik)) {
        return godot::Error::ERR_FILE_CANT_WRITE;
    }

    file->store_32(W3e::kW3eVersion);

    file->store_8(resource->main_tileset_id_);
    file->store_32(resource->custom_tileset_flag_);

    file->store_32(resource->number_of_ground_tilesets_);
    for(size_t i = 0; i < resource->number_of_ground_tilesets_; ++i) {
        file->store_32(0);
    }

    file->store_32(resource->number_of_geo_tilesets_);
    for(size_t i = 0; i < resource->number_of_geo_tilesets_; ++i) {
        file->store_32(0);
    }

    file->store_32(resource->map_size_x_);
    file->store_32(resource->map_size_y_);

    file->store_float(resource->map_3d_offset_x_);
    file->store_float(resource->map_3d_offset_z_);

    for(int32_t i = 0; i < resource->map_size_y_; ++i) {
        for(int32_t j = 0; j < resource->map_size_x_; ++j) {
            const W3eCell &cell_point = resource->get_cellpoint(j, i);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const auto *p_src = reinterpret_cast<const uint8_t*>(&cell_point);
            file->store_buffer(p_src, sizeof(W3eCell));
        }
    }
    return godot::Error::OK;
}


}  // namespace w3terr

