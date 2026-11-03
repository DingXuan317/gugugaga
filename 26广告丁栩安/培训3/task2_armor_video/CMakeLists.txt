cmake_minimum_required(VERSION 3.16)

project(task2_armor_video LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

add_executable(task2_armor_video
    src/main.cpp
    src/armor_detector.cpp
)

target_include_directories(task2_armor_video PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(task2_armor_video PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(task2_armor_video PRIVATE /utf-8 /W4)
endif()

add_custom_command(TARGET task2_armor_video POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:task2_armor_video>/assets"
    COMMENT "Copying test video"
)

if(WIN32)
    file(GLOB OPENCV_RUNTIME_DLLS
        "${OpenCV_DIR}/x64/*/bin/opencv_*.dll"
    )
    list(FILTER OPENCV_RUNTIME_DLLS EXCLUDE REGEX "d\\.dll$")
    if(OPENCV_RUNTIME_DLLS)
        add_custom_command(TARGET task2_armor_video POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OPENCV_RUNTIME_DLLS}
                "$<TARGET_FILE_DIR:task2_armor_video>"
            COMMENT "Copying OpenCV runtime DLLs"
        )
    endif()
endif()
