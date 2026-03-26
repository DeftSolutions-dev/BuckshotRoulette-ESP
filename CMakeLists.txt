cmake_minimum_required(VERSION 3.15)
project(BuckshotESP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(BuckshotESP
    src/main.cxx
    src/config.cxx
    src/patcher.cxx
    src/esp.cxx
    src/overlay.cxx
)

target_include_directories(BuckshotESP PRIVATE src)
target_compile_definitions(BuckshotESP PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(BuckshotESP PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MT /W3 /O2>)
target_link_libraries(BuckshotESP PRIVATE
    user32 gdi32 shell32 ole32 comdlg32 advapi32)
