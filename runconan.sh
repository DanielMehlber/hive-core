rm build -r
conan install --output-folder=build .
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
# cmake --build .