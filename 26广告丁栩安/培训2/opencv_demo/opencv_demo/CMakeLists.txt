# 最低 CMake 版本
cmake_minimum_required(VERSION 3.16)

# 项目名称和语言
project(opencv_demo LANGUAGES CXX)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找 OpenCV。若 CMake 找不到，可在配置时传 -DOpenCV_DIR=...
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# 生成可执行程序
add_executable(opencv_demo main.cpp)

# 头文件目录和链接库都来自 find_package 的结果
target_include_directories(opencv_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(opencv_demo PRIVATE ${OpenCV_LIBS})

if(MSVC)
    # 源文件使用 UTF-8，避免中文注释在 GBK 代码页下被误读
    target_compile_options(opencv_demo PRIVATE /utf-8 /W4)
endif()

# 把 images 文件夹复制到 exe 旁边，保证从 build 目录运行也能找到图片
add_custom_command(TARGET opencv_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/images"
        "$<TARGET_FILE_DIR:opencv_demo>/images"
    COMMENT "Copying images to the executable directory"
)

# Windows 下 OpenCV 通常是动态库，把运行所需 DLL 复制到 exe 旁边
if(WIN32)
    file(GLOB OPENCV_RUNTIME_DLLS
        "${OpenCV_DIR}/x64/*/bin/opencv_*.dll"
    )
    # 本项目按 Release 构建，过滤掉带 d 后缀的 Debug DLL
    list(FILTER OPENCV_RUNTIME_DLLS EXCLUDE REGEX "d\\.dll$")
    if(OPENCV_RUNTIME_DLLS)
        add_custom_command(TARGET opencv_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OPENCV_RUNTIME_DLLS}
                "$<TARGET_FILE_DIR:opencv_demo>"
            COMMENT "Copying OpenCV runtime DLLs"
        )
    endif()
endif()
