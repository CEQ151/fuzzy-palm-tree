#include "filesystem.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

#ifdef _WIN32
static std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed <= 0) return std::wstring();
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return std::string();
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#endif

vector<FileInfo> FileSystemScanner::scan_directory(const string& path, 
                                                  const FileTreeOptions& options) {
    vector<FileInfo> result;
    
    if (!is_path_safe(path)) {
        cerr << "Error: Path is not safe to access: " << path << endl;
        return result;
    }
    
    try {
#ifdef _WIN32
        fs::path root_path = utf8_to_wstring(path);
#else
        fs::path root_path(path);
#endif

        if (!fs::exists(root_path)) {
            cerr << "Error: Path does not exist: " << path << endl;
            return result;
        }
        
        if (!fs::is_directory(root_path)) {
            cerr << "Error: Path is not a directory: " << path << endl;
            return result;
        }
        
        scan_recursive(root_path, result, options, 0);
    } catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "Error scanning directory: " << e.what() << endl;
    }
    
    return result;
}

void FileSystemScanner::scan_recursive(const fs::path& path, 
                                      vector<FileInfo>& result, 
                                      const FileTreeOptions& options,
                                      int depth) {
    // 检查深度限制
    if (options.max_depth >= 0 && depth > options.max_depth) {
        return;
    }
    
    try {
        // 先添加当前目录（如果深度大于0，表示不是根目录）
        if (depth > 0) {
            FileInfo dir_info;
#ifdef _WIN32
            dir_info.name = wstring_to_utf8(path.filename().wstring());
            dir_info.path = wstring_to_utf8(path.wstring());
#else
            dir_info.name = path.filename().string();
            dir_info.path = path.string();
#endif
            dir_info.is_directory = true;
            dir_info.size = calculate_directory_size(path);
            dir_info.last_modified = fs::last_write_time(path);
            dir_info.depth = depth;
            
            result.push_back(dir_info);
        }
        
        // 收集所有条目以便排序
        vector<fs::directory_entry> entries;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!should_exclude(entry.path(), options.exclude_patterns)) {
                entries.push_back(entry);
            }
        }
        
        // 排序：目录优先，然后按字母顺序
        sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            bool a_is_dir = a.is_directory();
            bool b_is_dir = b.is_directory();
            
            if (a_is_dir != b_is_dir) {
                return a_is_dir > b_is_dir; // 目录排在前面
            }
            
            return a.path().filename() < b.path().filename();
        });
        
        // 处理排序后的条目
        for (const auto& entry : entries) {
            const auto& entry_path = entry.path();
            
            try {
                if (entry.is_directory()) {
                    // 递归扫描子目录
                    scan_recursive(entry_path, result, options, depth + 1);
                } else {
                    FileInfo info;
#ifdef _WIN32
                    info.name = wstring_to_utf8(entry_path.filename().wstring());
                    info.path = wstring_to_utf8(entry_path.wstring());
#else
                    info.name = entry_path.filename().string();
                    info.path = entry_path.string();
#endif
                    info.is_directory = false;
                    info.depth = depth + 1;
                    info.last_modified = fs::last_write_time(entry_path);
                    info.size = entry.file_size();
                    result.push_back(info);
                }
            } catch (const fs::filesystem_error& e) {
                cerr << "Warning: Cannot access " << entry_path << ": " << e.what() << endl;
                continue;
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Error accessing " << path << ": " << e.what() << endl;
    }
}

string FileSystemScanner::generate_tree_text(const vector<FileInfo>& files, 
                                           const FileTreeOptions& options) {
    ostringstream tree_stream;
    
    if (files.empty()) {
        return "No files found.";
    }
    
    // 移除错误的根目录显示，因为 files[0] 是第一个子文件而不是根目录
    
    // 跟踪每个深度的"是否是最后一个子项"的状态
    vector<bool> is_last_at_depth(256, false);
    
    for (size_t i = 0; i < files.size(); i++) {
        const auto& file = files[i];
        
        // 如果文件深度为0（理论上scan_recursive没有添加depth=0的项，但以防万一），跳过缩进处理直接显示
        if (file.depth == 0) {
            if (file.is_directory) {
                tree_stream << "📁 " << file.name << endl;
            } else {
                tree_stream << "📄 " << file.name << endl;
            }
            continue;
        }
        
        // 记录当前项是否是该深度的最后一个
        bool is_last = true;
        if (i + 1 < files.size()) {
            const auto& next_file = files[i + 1];
            if (next_file.depth > file.depth) {
                is_last = false;
            } else if (next_file.depth == file.depth) {
                is_last = false;
            }
        }
        is_last_at_depth[file.depth] = is_last;
        
        // 绘制树状结构
        for (int d = 1; d < file.depth; d++) {
            if (is_last_at_depth[d]) {
                tree_stream << "    ";
            } else {
                tree_stream << "│   ";
            }
        }
        
        // 添加分支符号
        if (is_last) {
            tree_stream << "└── ";
        } else {
            tree_stream << "├── ";
        }
        
        // 添加图标和文件名
        if (file.is_directory) {
            tree_stream << "📁 " << file.name;
        } else {
            tree_stream << "📄 " << file.name;
        }
        
        // 添加文件大小（如果启用）
        if (options.show_size) {
            tree_stream << " (" << format_file_size(file.size, options.human_readable) << ")";
        }
        
        tree_stream << endl;
    }
    
    return tree_stream.str();
}

uintmax_t FileSystemScanner::calculate_directory_size(const fs::path& path) {
    uintmax_t total_size = 0;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                try {
                    total_size += entry.file_size();
                } catch (const fs::filesystem_error&) {
                    // 忽略无法访问的文件
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        // 忽略无法访问的目录
    }
    
    return total_size;
}

string FileSystemScanner::format_file_size(uintmax_t size, bool human_readable) {
    if (!human_readable) {
        return to_string(size) + " B";
    }
    
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double formatted_size = static_cast<double>(size);
    
    while (formatted_size >= 1024.0 && unit_index < 4) {
        formatted_size /= 1024.0;
        unit_index++;
    }
    
    ostringstream stream;
    stream << fixed << setprecision(2) << formatted_size << " " << units[unit_index];
    return stream.str();
}

bool FileSystemScanner::is_path_safe(const fs::path& path) {
    // 安全检查：确保路径在允许的范围内
    // 这里可以添加更多的安全检查逻辑
    
    try {
        // 检查是否为绝对路径
        if (path.is_absolute()) {
            // 可以在这里添加对特定目录的限制
            // 例如，只允许访问用户指定的目录
            return true;
        }
        
        // 相对路径也需要检查
        fs::path canonical_path = fs::canonical(path);
        // 可以添加更多的安全检查
        
        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

bool FileSystemScanner::should_exclude(const fs::path& path, 
                                      const vector<string>& patterns) {
    string filename;
#ifdef _WIN32
    filename = wstring_to_utf8(path.filename().wstring());
#else
    filename = path.filename().string();
#endif

    // 默认排除规则
    // 默认不排除任何文件，除非用户指定
    /*
    // 排除以点开头的隐藏文件/目录（如 .git, .vscode, .DS_Store）
    if (filename.empty() || filename[0] == '.') {
        return true;
    }
    
    // 排除常见构建和依赖目录
    static const vector<string> default_excludes = {
        "build", "bin", "lib", "node_modules", "vendor", "__pycache__", "target", "dist", "out",
        "Debug", "Release", "x64", "x86", "obj", "CMakeFiles", "uploads"
    };
    
    for (const auto& exclude : default_excludes) {
        if (filename == exclude) {
            return true;
        }
    }
    */

    if (patterns.empty()) {
        return false;
    }
    
    for (const auto& pattern : patterns) {
        try {
            regex re(pattern, regex::icase);
            if (regex_match(filename, re)) {
                return true;
            }
        } catch (const regex_error&) {
            // 无效的正则表达式，尝试简单匹配
            if (filename.find(pattern) != string::npos) {
                return true;
            }
        }
    }
    
    return false;
}