mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake .. -G "Visual Studio 17 2022" -A x64
cd ..