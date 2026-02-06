cmake_minimum_required(VERSION 3.15)
project(FileManagerWebGUI VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows 版本要求（httplib 需要 Windows 10+）
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加可执行文件
add_executable(filemanager
    src/backend/main.cpp
    src/backend/filesystem.cpp
    src/backend/webserver.cpp
)

# 包含目录
target_include_directories(filemanager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库 - Windows Socket 库
if(WIN32)
    target_link_libraries(filemanager PRIVATE ws2_32)
endif()
# 复制前端文件到构建目录
add_custom_command(TARGET filemanager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend
        $<TARGET_FILE_DIR:filemanager>/frontend
)

# 安装目标
install(TARGETS filemanager DESTINATION bin)
install(DIRECTORY src/frontend/ DESTINATION share/filemanager/frontend)