cmake_minimum_required(VERSION 3.20)


project(fsarcade)


# compile options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# code directories
set(FSARCADE_ROOT_DIR   "${CMAKE_CURRENT_LIST_DIR}/..")
set(FSARCADE_SRC_DIR    "${FSARCADE_ROOT_DIR}/src")
set(IMGUI_SRC_DIR       "${FSARCADE_ROOT_DIR}/external/imgui")


# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${FSARCADE_ROOT_DIR}/runtree)


# required packages
find_package(SDL3 REQUIRED)


# fsarcade
add_executable(fsarcade)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fsarcade PRIVATE -pedantic-errors -Wall -Weffc++ -Wconversion -Wsign-conversion -Wshadow -mavx2)
endif ()
target_sources(fsarcade PUBLIC
    ${FSARCADE_SRC_DIR}/main.cpp
    ${FSARCADE_SRC_DIR}/common/math.cpp
    ${FSARCADE_SRC_DIR}/common/Font.cpp
    ${FSARCADE_SRC_DIR}/common/MemoryManager.cpp
    ${FSARCADE_SRC_DIR}/common/Jobsys.cpp
    ${FSARCADE_SRC_DIR}/renderer/Renderer.cpp
    ${FSARCADE_SRC_DIR}/renderer/RSoftwareBackend.cpp
    ${FSARCADE_SRC_DIR}/games/Game.cpp
    ${FSARCADE_SRC_DIR}/games/fetris/Fetris.cpp
    ${FSARCADE_SRC_DIR}/games/fetris/Fetromino.cpp
    ${FSARCADE_SRC_DIR}/games/fetris/Board.cpp
    ${FSARCADE_SRC_DIR}/games/fnake/Fnake.cpp
    ${FSARCADE_SRC_DIR}/games/finesweeper/Finesweeper.cpp
    ${FSARCADE_SRC_DIR}/games/freakout/Freakout.cpp
    ${FSARCADE_SRC_DIR}/games/fasteroids/Fasteroids.cpp
)
target_include_directories(fsarcade PRIVATE
    ${FSARCADE_SRC_DIR}
)


# imgui
add_library(imgui OBJECT
    ${IMGUI_SRC_DIR}/imgui.cpp
    ${IMGUI_SRC_DIR}/imgui_draw.cpp
    ${IMGUI_SRC_DIR}/imgui_tables.cpp
    ${IMGUI_SRC_DIR}/imgui_widgets.cpp
    ${IMGUI_SRC_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_SRC_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_SRC_DIR} ${IMGUI_SRC_DIR}/backends)


# stb
add_library(stb_truetype STATIC ${FSARCADE_ROOT_DIR}/external/stb/stb_truetype.c)
target_include_directories(stb_truetype PUBLIC ${FSARCADE_ROOT_DIR}/external/stb)


target_link_libraries(fsarcade SDL3::SDL3 dl imgui stb_truetype)

