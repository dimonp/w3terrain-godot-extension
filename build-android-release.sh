cmake -S . -B ./out/build/android-release \
  -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Android \
  -DVCPKG_TARGET_TRIPLET=arm64-android \
  -DANDROID_PLATFORM=android-21 \
  -DANDROID_ABI=arm64-v8a \
  -DGODOTCPP_TARGET=template_release \
  -Dgdextension_w3terrain_ENABLE_IPO=OFF

cmake --build ./out/build/android-release
