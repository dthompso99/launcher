
CENTAURI_API="http://api.centauri.world"
CENTAURI_API_KEY="ZHRob21wc29uOmhvbWVicjN3"
BUILD_TYPE=Release
APP_NAME=SubAtomicLauncher
read Build < buildnum.txt

rm -rf cmake_build_linux_$BUILD_TYPE
mkdir cmake_build_linux_$BUILD_TYPE
cd cmake_build_linux_$BUILD_TYPE

cmake --build . --target clean --config $BUILD_TYPE
cmake .. -DCMAKE_CXX_COMPILER=clang -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCENTAURI_BUILDNUM=$Build -DCENTAURI_API="$CENTAURI_API" -DCENTAURI_API_KEY="$CENTAURI_API_KEY" -DEXENAME=$APP_NAME
cmake --build . --target custom-install --config $BUILD_TYPE
cd ..


