cmake_minimum_required(VERSION 3.10)
project(text_editor)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# NFD
set(NFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog)

if(APPLE)
    add_library(nfd STATIC
        ${NFD_DIR}/src/nfd_common.c
        ${NFD_DIR}/src/nfd_cocoa.m
    )
    target_compile_options(nfd PRIVATE -x objective-c)
    target_link_libraries(nfd PRIVATE "-framework Cocoa")
elseif(UNIX)
    add_library(nfd STATIC
        ${NFD_DIR}/src/nfd_common.c
        ${NFD_DIR}/src/nfd_gtk.c
    )
    # Add GTK dependency
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(nfd PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(nfd PRIVATE ${GTK3_LIBRARIES})
endif()

target_include_directories(nfd PUBLIC 
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
)

file(COPY ${CMAKE_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR})

add_executable(text_editor
    main.cpp
    settings.cpp
    files.cpp
    editor.cpp
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(text_editor PRIVATE 
    imgui 
    imgui/backends 
    ${NFD_DIR}/src
    ${NFD_DIR}/src/include
)

include_directories(${PROJECT_SOURCE_DIR}/include)
file(COPY ${CMAKE_SOURCE_DIR}/icons DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(text_editor PRIVATE ${OPENGL_LIBRARIES} glfw nfd)

if(APPLE)
    target_link_libraries(text_editor PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif() 