#ifndef _W3MATH__H
#define _W3MATH__H

#include <algorithm>
#include <immintrin.h>

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/projection.hpp>

namespace w3terr::math {

template<typename T>
inline T w3_min4(T value0, T value1, T value2, T value3)
{
    return std::min(value0, std::min(value1, std::min(value2, value3)));
}

template<typename T>
inline T w3_max4(T value0, T value1, T value2, T value3)
{
    return std::max(value0, std::max(value1, std::max(value2, value3)));
}

inline float w3_lerp(float value0, float value1, float t_lerp)
{
    return std::lerp(value0, value1, t_lerp);
}

inline float w3_lerp_bi(float v00, float v01, float v10, float v11, float t_lerp, float s_lerp) {
    const float first = std::lerp(v00, v01, t_lerp);
    const float second = std::lerp(v10, v11, t_lerp);
    return std::lerp(first, second, s_lerp);
}

inline float w3_clamp01(float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

// NOLINTBEGIN(readability-identifier-naming, readability-identifier-length, hicpp-signed-bitwise)

using vector2 = godot::Vector2;
using vector4 = godot::Vector4;
using matrix44 = godot::Projection;

/*
 * 3D Vector adapter
 */
struct vector3: public godot::Vector3 {
    vector3() =  default;
    vector3(const vector3& v) =  default;
    vector3& operator=(const vector3& v) =  default;

    vector3(const godot::Vector3& gv): godot::Vector3(gv)  {}; // NOLINT(google-explicit-constructor, hicpp-explicit-conversions)
    vector3(float x, float y, float z): godot::Vector3(x, y, z) {};

    float get_x() const { return godot::Vector3::x; }
    float get_y() const { return godot::Vector3::y; }
    float get_z() const { return godot::Vector3::z; }
};

/*
 * 3D line
 */
struct line3 {
    line3(const vector3& origin, const vector3& direction)
        : origin(origin), direction(direction) {};

    const vector3& start() const { return origin; };
    vector3 end() const { return origin + direction; };

    vector3 origin;
    vector3 direction;
};

/*
 * 3D triangle
 */
struct triangle {
    vector3 b, e0, e1;

    triangle() = default;
    triangle(const triangle& v) =  default;
    triangle& operator=(const triangle& v) =  default;

    triangle(const vector3& v0, const vector3& v1, const vector3& v2)
        : b(v0), e0(v1 - v0), e1(v2 - v0) {};

    const vector3& get_v0() const { return b; }
    vector3 get_v1() const { return b + e0; }
    vector3 get_v2() const { return b + e1; }

    bool intersect(const line3& line, vector3& intersection_point) const
    {
        static constexpr float EPSILON = 1e-7F;

        // Calculate the determinant
        const vector3 h = line.direction.cross(e1);
        const float a = e0.dot(h);

        // If 'a' is nearly zero, the line is parallel to the triangle plane
        if (a > -EPSILON && a < EPSILON) {
            return false;
        }

        const float f = 1.0F / a;
        const vector3 s = line.origin - b;
        const float u = f * s.dot(h);

        // Check if the intersection lies outside the triangle (u coordinate)
        if (u < 0.0F || u > 1.0F) {
            return false;
        }

        const vector3 q = s.cross(e0);
        const float v = f * line.direction.dot(q);

        // Check if the intersection lies outside the triangle (v coordinate)
        if (v < 0.0F || u + v > 1.0F) {
            return false;
        }

        // Compute 't' to find where the intersection point is on the line
        const float t = f * e1.dot(q);

        if (t > EPSILON) { // Intersection occurs (t > 0 means it's ahead of the origin)
            intersection_point = line.origin + line.direction * t;
            return true;
        }

        // A negative 't' means there is a line intersection, but not a ray intersection
        return false;
    }
};

/*
 * Axis-aligned 3D bounding box adapter
 */
struct bbox3: public godot::AABB {

    // clip codes
    enum : uint8_t {
        ClipLeft   = (1U << 0U),
        ClipRight  = (1U << 1U),
        ClipBottom = (1U << 2U),
        ClipTop    = (1U << 3U),
        ClipNear   = (1U << 4U),
        ClipFar    = (1U << 5U),
    };

    // clip status
    enum ClipStatus : uint8_t {
        Outside,
        Inside,
        Clipped,
    };

    bbox3() = default;
    bbox3(const bbox3&) =  default;
    bbox3& operator=(const bbox3&) =  default;

    bbox3(const vector3 &min, const vector3 &max): godot::AABB(min, max - min) {}

    explicit bbox3(const godot::AABB& aabb): godot::AABB(aabb) {}

    static bbox3 from_pos_size(const vector3 &pos, const vector3 &size)
    {
        return bbox3(godot::AABB(pos, size));
    }

    vector3 get_min() const { return godot::AABB::get_position(); };
    vector3 get_max() const { return godot::AABB::get_end(); };

    vector3 get_center() const { return godot::AABB::get_center(); }
    vector3 get_size() const { return godot::AABB::get_size(); }
    vector3 get_extents() const { return { godot::AABB::get_size() * 0.5F }; }
    float get_diagonal_size() const { return get_size().length(); }

    void begin_extend()
    {
        set_size({-1000000.0F, -1000000.0F, -1000000.0F});
    }

    void extend(const vector3& v)
    {
        if (size.x < 0) {
            set_position(v);
            set_size(vector3(0.0, 0.0, 0.0));
        } else {
            expand_to(v);
        }
    }

    void extend(const bbox3& box)
    {
        if (size.x < 0) {
            set_position(box.position);
            set_size(box.size);
        } else {
            merge_with(box);
        }
    }

    /**
     * @brief Optimized clipstatus using 6 Frustum Planes.
     * @param planes Frustum planes in order NEAR, FAR, LEFT, RIGHT, TOP, BOTTOM
     */
    ClipStatus clipstatus(const godot::Plane planes[6]) const {
        const vector3 min = this->get_min();
        const vector3 max = this->get_max();

#if defined(__AVX__)
        // Prepare plane components in SoA (Structure of Arrays) format
        __m256 p_x = _mm256_setr_ps(
            planes[0].normal.x, planes[1].normal.x, planes[2].normal.x, planes[3].normal.x, 
            planes[4].normal.x, planes[5].normal.x, 0.0f, 0.0f
        );
        __m256 p_y = _mm256_setr_ps(
            planes[0].normal.y, planes[1].normal.y, planes[2].normal.y, planes[3].normal.y, 
            planes[4].normal.y, planes[5].normal.y, 0.0f, 0.0f
        );
        __m256 p_z = _mm256_setr_ps(
            planes[0].normal.z, planes[1].normal.z, planes[2].normal.z, planes[3].normal.z, 
            planes[4].normal.z, planes[5].normal.z, 0.0f, 0.0f
        );
        __m256 p_w = _mm256_setr_ps(
            planes[0].d, planes[1].d, planes[2].d, planes[3].d, 
            planes[4].d, planes[5].d, 0.0f, 0.0f
        );

        __m256 v_min_x = _mm256_set1_ps(min.x);
        __m256 v_min_y = _mm256_set1_ps(min.y);
        __m256 v_min_z = _mm256_set1_ps(min.z);

        __m256 v_max_x = _mm256_set1_ps(max.x);
        __m256 v_max_y = _mm256_set1_ps(max.y);
        __m256 v_max_z = _mm256_set1_ps(max.z);

        // Component signs masks (true if normal > 0)
        __m256 mask_x = _mm256_cmp_ps(p_x, _mm256_setzero_ps(), _CMP_GT_OS);
        __m256 mask_y = _mm256_cmp_ps(p_y, _mm256_setzero_ps(), _CMP_GT_OS);
        __m256 mask_z = _mm256_cmp_ps(p_z, _mm256_setzero_ps(), _CMP_GT_OS);

        // Find the "Nearest" vertex (N-vertex, lowest dot product)
        // If plane normal > 0, pick min, else pick max
        __m256 n_vx = _mm256_blendv_ps(v_max_x, v_min_x, mask_x);
        __m256 n_vy = _mm256_blendv_ps(v_max_y, v_min_y, mask_y);
        __m256 n_vz = _mm256_blendv_ps(v_max_z, v_min_z, mask_z);

        // Calculate dot product for N-vertex: (A*px + B*py + C*pz + D)
        __m256 dot_min = _mm256_add_ps(
            _mm256_add_ps(_mm256_mul_ps(p_x, n_vx), _mm256_mul_ps(p_y, n_vy)),
            _mm256_sub_ps(_mm256_mul_ps(p_z, n_vz), p_w));

        // If N-vertex is in the positive half-space (>0), the entire AABB is outside
        int out_mask = _mm256_movemask_ps(_mm256_cmp_ps(dot_min, _mm256_setzero_ps(), _CMP_GT_OS));
        if (out_mask & 0x3F) return Outside;

        // Find the "Farthest" vertex (P-vertex, highest dot product)
        // If P-vertex is > 0 while N-vertex was <= 0, the AABB intersects the plane
        __m256 p_vx = _mm256_blendv_ps(v_min_x, v_max_x, mask_x);
        __m256 p_vy = _mm256_blendv_ps(v_min_y, v_max_y, mask_y);
        __m256 p_vz = _mm256_blendv_ps(v_min_z, v_max_z, mask_z);

        // Calculate dot product for P-vertex: (A*px + B*py + C*pz + D)
        __m256 dot_max = _mm256_add_ps(
            _mm256_add_ps(_mm256_mul_ps(p_x, p_vx), _mm256_mul_ps(p_y, p_vy)),
            _mm256_sub_ps(_mm256_mul_ps(p_z, p_vz), p_w));

        // If P-vertex is in the positive half-space (>0), the AABB is clipped
        int clip_mask = _mm256_movemask_ps(_mm256_cmp_ps(dot_max, _mm256_setzero_ps(), _CMP_GT_OS));
        return (clip_mask & 0x3F) ? Clipped : Inside;

#else
        bool is_clipped = false;

        for (size_t i = 0; i < 6; ++i) {
            const auto& plane = planes[i];
            
            // P-vertex: farthest point in the direction of the plane normal
            vector3 p_vertex(
                (plane.normal.x > 0) ? max.x : min.x,
                (plane.normal.y > 0) ? max.y : min.y,
                (plane.normal.z > 0) ? max.z : min.z
            );

            // N-vertex: nearest point
            vector3 n_vertex(
                (plane.normal.x > 0) ? min.x : max.x,
                (plane.normal.y > 0) ? min.y : max.y,
                (plane.normal.z > 0) ? min.z : max.z
            );

            float dist_p = plane.distance_to(p_vertex);
            float dist_n = plane.distance_to(n_vertex);

            // If the nearest point is behind the plane, the AABB is completely Outside
            if (dist_n > 0) {
                return Outside;
            }

            // If the farthest point is also behind the plane, the AABB is Clipped
            if (dist_p > 0) {
                is_clipped = true;
            }        
        }
        return is_clipped ? Clipped : Inside;    
#endif
    }    

    bool test_intersection(const line3& line) const
    {
        return intersects_ray(line.origin, line.direction);
    }
};

/*
 * @brief Packs a vector3 into a 32-bit unsigned integer.
 * @param vector The vector3 to pack.
 * @return The packed 32-bit unsigned integer.
 */
inline
uint32_t
pack_vector3_to_32bit(const vector3& vector)
{
    const auto i_x = static_cast<uint32_t>(std::round((vector.x * 0.5F + 0.5F) * 1023.0F));
    const auto i_y = static_cast<uint32_t>(std::round((vector.y * 0.5F + 0.5F) * 1023.0F));
    const auto i_z = static_cast<uint32_t>(std::round((vector.z * 0.5F + 0.5F) * 4095.0F));

    // Pack into a 32-bit unsigned integer using bit shifts
    // Format: [ ZZZZ ZZZZ ZZZZ YYYY YYYY YY XXXX XXXX XX ]
    // or (IZ << 20) | (IY << 10) | IX
    return (i_z << 20U) | (i_y << 10U) | i_x;
}

/*
 * @brief Unpacks a 32-bit unsigned integer into a vector3.
 * @param packed_vector The 32-bit unsigned integer to unpack.
 * @return The unpacked vector3.
 */
inline
vector3
unpack_vector3_from_32bit(uint32_t packed_vector)
{
#if defined(__x86_64__) || defined(_M_X64)
    // Extract components into a 128-bit integer register (SSE2)
    // Layout: [X (10-bit), Y (10-bit), Z (12-bit), 0]
    __m128i i_components = _mm_setr_epi32(
        static_cast<int>(packed_vector & 0x3FFU),
        static_cast<int>((packed_vector >> 10U) & 0x3FFU),
        static_cast<int>((packed_vector >> 20U) & 0xFFFU),
        0
    );

    // Convert integer components to 32-bit floats
    __m128 f_components = _mm_cvtepi32_ps(i_components);

    // Pre-calculate (2.0f / max_int_val) to combine division and multiplication
    // X, Y max: 1023.0f | Z max: 4095.0f
    static const __m128 scale = _mm_setr_ps(
        2.0f / 1023.0f, 
        2.0f / 1023.0f, 
        2.0f / 4095.0f, 
        0.0f
    );
    static const __m128 offset = _mm_set1_ps(1.0f);

    // Perform math: (f * scale) - 1.0f
    f_components = _mm_mul_ps(f_components, scale);
    f_components = _mm_sub_ps(f_components, offset);

    // Store results back to a temporary array to avoid alignment issues with vector3 struct
    alignas(16) float result[4];
    _mm_store_ps(result, f_components);

    return { result[0], result[1], result[2] };
#else
    // Unpack integer components using bit shifts and masks
    const uint32_t i_x = packed_vector & 0x3FFU;          // Mask for lower 10 bits (00000000000000000000001111111111)
    const uint32_t i_y = (packed_vector >> 10U) & 0x3FFU; // Mask for next 10 bits  (00000000000011111111110000000000)
    const uint32_t i_z = (packed_vector >> 20U) & 0xFFFU; // Mask for upper 12 bits (11111111111100000000000000000000)

    // Convert back to float range [-1.0, 1.0]
    // Inverse operation: (unpacked_val / max_int_val) * 2.0f - 1.0f
    return {
        (static_cast<float>(i_x) / 1023.0F * 2.0F) - 1.0F,
        (static_cast<float>(i_y) / 1023.0F * 2.0F) - 1.0F,
        (static_cast<float>(i_z) / 4095.0F * 2.0F) - 1.0F
    };
#endif
}

/**
 * @brief Returns the next power of two for a given number.
 * @param n The number to find the next power of two for.
 * @return The next power of two.
 */
inline
int32_t
lower_bound_power_of_two(int32_t num)
{
    if (num <= 0) { return 1; }

    // If n is already a power of 2, return n.
    if ((num & (num - 1)) == 0) { return num; }

    // "Smear" the bits: set all bits to the right of the most significant bit
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;

    // Add 1 to get the next power of 2
    return num + 1;
}

// NOLINTEND(readability-identifier-naming, readability-identifier-length, hicpp-signed-bitwise)

}  // namespace w3terr::math

#endif // _W3MAP_MATH__H

