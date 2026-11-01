cmake_minimum_required(VERSION 3.16)

project(student_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json，供编辑器(IntelliSense)识别头文件路径
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 生成静态库 student
add_library(student STATIC
    tools/student.cpp
)

# 让编译 student 和链接 student 的目标都能找到 student.hpp
target_include_directories(student PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

# 生成可执行程序
add_executable(student_demo
    src/main.cpp
)

# 把静态库链接进可执行程序
target_link_libraries(student_demo PRIVATE student)

if(MSVC)
    target_compile_options(student PRIVATE /W4 /utf-8)
    target_compile_options(student_demo PRIVATE /W4 /utf-8)
else()
    target_compile_options(student PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(student_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
