cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="D:/Qt/5.15/qt5.15.16-msvc/lib/cmake"
cmake --build build --config Release -j 16