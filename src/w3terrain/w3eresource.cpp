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


}  // namespace w3terr

