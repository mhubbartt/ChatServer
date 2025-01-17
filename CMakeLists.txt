# Minimum CMake version required
cmake_minimum_required(VERSION 3.17)

# Project Definition
project(ChatServer VERSION 1.0 LANGUAGES CXX)

# Compiler Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable folders in Visual Studio project organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Platform-Specific Configurations
if(WIN32)
    cmake_policy(SET CMP0156 NEW)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DGLFW_INCLUDE_NONE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux...")
    add_definitions(-DGLFW_INCLUDE_NONE)
endif()

# Include Directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/boost
    ${CMAKE_SOURCE_DIR}/include/spdlog/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/json/include
    ${CMAKE_SOURCE_DIR}/include/glfw/include
)

# Source Files
file(GLOB_RECURSE PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/source/*.cpp ${CMAKE_SOURCE_DIR}/source/*.h)
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/include/imgui/imgui_impl_opengl3.cpp
)
set(SOURCES ${PROJECT_SOURCES} ${IMGUI_SOURCES})

# Main Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Libraries and Linking
find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(PkgConfig)
    if(NOT PkgConfig_FOUND)
        message(WARNING "pkg-config not found! Attempting to install it...")
        execute_process(
            COMMAND bash -c "if [ -x \"$(command -v apt-get)\" ]; then \
                                  sudo apt-get update && \
                                  sudo apt-get install -y pkg-config; \
                              elif [ -x \"$(command -v yum)\" ]; then \
                                  sudo yum install -y pkg-config; \
                              else \
                                  echo \"Unsupported package manager. Please install pkg-config manually.\"; \
                                  exit 1; \
                              fi"
        )
        find_package(PkgConfig REQUIRED)
    endif()
endif()

if(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenGL::GL Threads::Threads)
    target_link_options(${PROJECT_NAME} PRIVATE /NODEFAULTLIB:MSVCRT)
elseif(UNIX AND NOT APPLE)
    find_package(OpenGL)
    if(NOT OPENGL_FOUND)
        message(WARNING "Attempting to load dependencies...")
        execute_process(
            COMMAND bash -c "if [ -x \"$(command -v apt-get)\" ]; then \
                                  sudo apt-get update && \
                                  sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev; \
                              elif [ -x \"$(command -v yum)\" ]; then \
                                  sudo yum groupinstall -y \"Development Tools\" && \
                                  sudo yum install -y mesa-libGL-devel mesa-libGLU-devel mesa-libGLES-devel; \
                              else \
                                  echo \"Unsupported package manager. Please install OpenGL manually.\"; \
                                  exit 1; \
                              fi"
        )
        set(OPENGL_gl_LIBRARY /usr/lib/x86_64-linux-gnu/libGL.so)
        set(OPENGL_glx_LIBRARY /usr/lib/x86_64-linux-gnu/libGLX.so)
        if(EXISTS ${OPENGL_gl_LIBRARY} AND EXISTS ${OPENGL_glx_LIBRARY})
            message(STATUS "Manually located OpenGL libraries.")
        else()
            message(FATAL_ERROR "OpenGL installation failed. Please check manually.")
        endif()
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_directories(${GLFW_LIBRARY_DIRS})
    add_definitions(${GLFW_CFLAGS_OTHER})
    target_link_libraries(${PROJECT_NAME}
        ${GLFW_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glx_LIBRARY}
        Threads::Threads
    )
endif()

# Output Directories
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    target_link_libraries(${PROJECT_NAME}
        "${CMAKE_BINARY_DIR}/lib/glfw/lib-vc2022/glfw3.lib"
        "${CMAKE_BINARY_DIR}/lib/glfw/lib-vc2022/glfw3_mt.lib"
    )
endif()

# Resource Copying
set(FONT_DIR "${CMAKE_SOURCE_DIR}/Fonts")
set(CONFIG_FILE "${CMAKE_SOURCE_DIR}/config.ini")
set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")
set(FONT_OUTPUT_DIR "${BIN_DIR}/Fonts")

add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${FONT_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONT_DIR} ${FONT_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CONFIG_FILE} ${BIN_DIR}
)
add_dependencies(${PROJECT_NAME} CopyResources)

# Suppress Batch Verbosity
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Compiler Flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
