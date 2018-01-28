SET MANIFEST_BASE="https://api.centauri.world/patcher"
SET BUILD_TYPE=Release
SET APP_NAME="CentauriLauncher"
SET PATCHER_TITLE="Centauri Engine Patcher"
SET EXECUTABLE="start SubAtomic_d.exe &"

if not exist buildnum.txt > buildnum.txt echo 1
set /p Build=<buildnum.txt
set /a Build=%Build%+1
echo %Build%>buildnum.txt

mkdir cmake_build_win_%BUILD_TYPE%
cd cmake_build_win_%BUILD_TYPE%

cmake --build . --target clean --config %BUILD_TYPE%

cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_GENERATOR="Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DLAUNCHER_BUILDNUM=%Build% -DEXENAME=%APP_NAME% -DMANIFEST_BASE=%MANIFEST_BASE% -DPATCHER_TITLE=%PATCHER_TITLE% -DEXECUTABLE=%EXECUTABLE%

cmake --build . --target custom-install --config %BUILD_TYPE% 

cd ..

