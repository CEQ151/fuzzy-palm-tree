#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <optional>

namespace fs = std::filesystem;

struct FileInfo {
    std::string name;
    std::string path;
    bool is_directory;
    uintmax_t size;  // 文件大小（字节）
    fs::file_time_type last_modified;  // 使用filesystem的时间类型
    int depth;  // 在文件树中的深度
};

struct FileTreeOptions {
    bool show_size = false;  // 是否显示文件大小
    bool human_readable = true;  // 是否使用人类可读的格式（KB, MB等）
    int max_depth = -1;  // 最大深度，-1表示无限制
    std::vector<std::string> exclude_patterns;  // 排除模式
};

class FileSystemScanner {
public:
    // 扫描目录并返回文件树
    static std::vector<FileInfo> scan_directory(const std::string& path, 
                                               const FileTreeOptions& options = {});
    
    // 生成制表符格式的文件树字符串
    static std::string generate_tree_text(const std::vector<FileInfo>& files, 
                                         const FileTreeOptions& options = {});
    
    // 计算目录总大小
    static uintmax_t calculate_directory_size(const fs::path& path);
    
    // 格式化文件大小为人类可读的字符串
    static std::string format_file_size(uintmax_t size, bool human_readable = true);
    
    // 验证路径是否安全可访问
    static bool is_path_safe(const fs::path& path);
    
private:
    // 递归扫描目录
    static void scan_recursive(const fs::path& path, 
                              std::vector<FileInfo>& result, 
                              const FileTreeOptions& options,
                              int depth = 0);
    
    // 检查文件是否应该被排除
    static bool should_exclude(const fs::path& path, 
                              const std::vector<std::string>& patterns);
};