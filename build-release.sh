cmake --fresh -S . -B ./out/build/release \
	-DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
	-DGODOTCPP_TARGET=template_release \
	-DCMAKE_BUILD_TYPE=Release

cmake --build ./out/build/release
