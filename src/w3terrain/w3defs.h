#ifndef _W3_DEFS__H
#define _W3_DEFS__H

#include <cassert>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/material.hpp>

namespace w3terr {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

using W3String = godot::String;
using W3Mesh = godot::Mesh;
using W3Texture = godot::Texture2D;
using W3Marerial = godot::Material;
using W3Color = godot::Color;

template <typename T>
using W3Ref = godot::Ref<T>;

template <typename Key, typename Tp>
using W3HashMap = std::unordered_map<Key,Tp>;

template <typename T1, typename T2>
using W3Pair = std::pair<T1, T2>;

template <typename T>
using W3Array = std::vector<T>;

using W3UInt8Pair = W3Pair<uint8_t, uint8_t>;

inline void w3_assert(bool p_condition) {
    assert(p_condition);
}

template <typename... Args>
inline void w3_log_error(const char* p_format, Args... p_args) {
    if (godot::gdextension_interface::get_godot_version != nullptr) { // check gdextension initialized
        godot::UtilityFunctions::printerr(godot::vformat(p_format, p_args...));
    }
}

template <typename... Args>
inline void w3_log_info(const char* p_format, Args... p_args) {
    if (godot::gdextension_interface::get_godot_version != nullptr) { // check gdextension initialized
        godot::UtilityFunctions::print(godot::vformat(p_format, p_args...));
    }
}

template <typename... Args>
inline void w3_log_debug(const char* p_format, Args... p_args) {
#ifdef DEBUG_ENABLED
    if (godot::gdextension_interface::get_godot_version != nullptr) { // check gdextension initialized
        godot::UtilityFunctions::print(godot::vformat(p_format, p_args...));
    }
#endif
}

struct Delta2D {
    int32_t dx, dy;
};

struct Coord2D {
    // NOLINTNEXTLINE(readability-identifier-length)
    Coord2D(int32_t x, int32_t y) : x(x), y(y) {}
    explicit Coord2D(const godot::Vector2i& coords) : x(coords.x), y(coords.y) {}
    Coord2D operator + (const Delta2D& delta) const { return { x + delta.dx, y + delta.dy }; }
    auto operator <=> (const Coord2D& other) const = default;
    int32_t x, y;
};

template <class T>
struct GodotObjectDeleter {
    void operator()(T* obj) const {
        if (obj) {
            memdelete(obj);
        }
    }
};

#if defined(_WIN32) || defined(__CYGWIN__)
  // Настройки для Windows (MSVC, MinGW)
  #ifdef W3_PROJECT_BUILD_DLL
    #define W3_API __declspec(dllexport)
  #else
    #define W3_API __declspec(dllimport)
  #endif
#else
  // Настройки для Unix-подобных систем (GCC, Clang)
  #if __GNUC__ >= 4
    #define W3_API __attribute__ ((visibility ("default")))
  #else
    #define W3_API
  #endif
#endif

}  // namespace w3terr

#endif // _W3_DEFS__H

