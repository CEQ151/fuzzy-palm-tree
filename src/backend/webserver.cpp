#include "webserver.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <regex>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// 辅助函数：UTF-8 string 到 wstring (Windows)
static std::wstring utf8_to_wstring(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed <= 0) return std::wstring();
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
#else
    // 非 Windows 平台通常直接使用 UTF-8
    return std::wstring(str.begin(), str.end());
#endif
}

// UTF-8 友好的 JSON 字符串转义函数
static string escape_json_string(const string& input) {
    string output;
    for (size_t i = 0; i < input.size(); i++) {
        unsigned char c = input[i];
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b";  break;
            case '\f': output += "\\f";  break;
            case '\n': output += "\\n";  break;
            case '\r': output += "\\r";  break;
            case '\t': output += "\\t";  break;
            default:
                // 对于UTF-8字节，直接输出（包括多字节字符的所有字节）
                if (c < 0x20) {
                    // 只转义真正的控制字符
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    output += buf;
                } else {
                    output += c;
                }
        }
    }
    return output;
}

WebServer::WebServer() {
    // 创建上传目录
    try {
        if (!fs::exists(upload_dir_)) {
            fs::create_directory(upload_dir_);
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Failed to create upload directory: " << e.what() << endl;
    }
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start(int port) {
    if (running_) {
        cerr << "Server is already running" << endl;
        return false;
    }
    
    port_ = port;
    server_ = make_unique<httplib::Server>();
    
    // 设置路由
    setup_routes();
    
    // 设置静态文件服务（前端文件）
    server_->set_base_dir("./frontend");
    
    // 设置文件上传大小限制（100MB）
    server_->set_payload_max_length(100 * 1024 * 1024);
    
    // 启动服务器线程
    server_thread_ = make_unique<thread>([this]() {
        cout << "Starting web server on port " << port_ << "..." << endl;
        cout << "Open http://localhost:" << port_ << " in your browser" << endl;
        
        if (!server_->listen("0.0.0.0", port_)) {
            cerr << "Failed to start server on port " << port_ << endl;
            running_ = false;
        }
    });
    
    // 等待服务器启动
    this_thread::sleep_for(chrono::milliseconds(100));
    running_ = true;
    
    return true;
}

void WebServer::stop() {
    if (server_) {
        server_->stop();
    }
    
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    running_ = false;
    cout << "Server stopped" << endl;
}

void WebServer::setup_routes() {
    // 根路径 - 服务前端页面
    server_->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        handle_root(req, res);
    });
    
    // API端点
    server_->Post("/api/upload", [this](const httplib::Request& req, httplib::Response& res) {
        handle_upload(req, res);
    });
    
    server_->Post("/api/scan", [this](const httplib::Request& req, httplib::Response& res) {
        handle_scan(req, res);
    });
    
    server_->Post("/api/tree", [this](const httplib::Request& req, httplib::Response& res) {
        handle_tree(req, res);
    });
    
    server_->Get("/api/download/tree", [this](const httplib::Request& req, httplib::Response& res) {
        handle_download(req, res);
    });
    
    server_->Get("/api/info", [this](const httplib::Request& req, httplib::Response& res) {
        handle_api_info(req, res);
    });
}

void WebServer::handle_root(const httplib::Request& req, httplib::Response& res) {
    // 重定向到前端页面
    res.set_redirect("/index.html");
}

void WebServer::handle_upload(const httplib::Request& req, httplib::Response& res) {
    try {
        string target_path_utf8;
        
        // 尝试从 multipart form data 获取路径
        if (req.form.has_field("path")) {
            target_path_utf8 = req.form.get_field("path");
        } else if (req.has_param("path")) {
            // 后备：尝试从 URL 参数获取
            target_path_utf8 = req.get_param_value("path");
        } else {
            res.set_content(generate_json_response(false, "Missing path parameter"), 
                           "application/json");
            return;
        }
        
        cout << "Upload request for path: " << target_path_utf8 << endl;

        // 基本安全检查
        if (target_path_utf8.find("..") != string::npos) {
            res.set_content(generate_json_response(false, "Invalid path"), 
                           "application/json");
            return;
        }

        // 转换路径为宽字符以支持中文 (Windows)
        fs::path target_path;
        try {
#ifdef _WIN32
            target_path = utf8_to_wstring(target_path_utf8);
#else
            target_path = target_path_utf8;
#endif
        } catch (const exception& e) {
             cerr << "Path conversion error: " << e.what() << endl;
             res.set_content(generate_json_response(false, "Path encoding error"), 
                           "application/json");
             return;
        }

        // 创建目标目录（如果不存在）
        try {
            if (!fs::exists(target_path)) {
                fs::create_directories(target_path);
            }
        } catch (const fs::filesystem_error& e) {
            cerr << "Directory creation error: " << e.what() << endl;
            res.set_content(generate_json_response(false, "Failed to create directory"), 
                           "application/json");
            return;
        }
        
        // 处理文件
        int files_uploaded = 0;
        vector<string> saved_files;
        
        if (req.form.has_file("files")) {
            auto files = req.form.get_files("files");
            cout << "Processing " << files.size() << " files..." << endl;
            
            for (const auto& file : files) {
                if (file.filename.empty()) continue;
                
                // 文件名也是 UTF-8，需要转换
                fs::path filename;
#ifdef _WIN32
                filename = utf8_to_wstring(file.filename);
#else
                filename = file.filename;
#endif
                if (filename.empty()) {
                    cerr << "Skipping file with empty or invalid filename" << endl;
                    continue;
                }

                fs::path filepath = target_path / filename.filename();
                
                try {
                    // 使用宽字符路径打开文件流
                    ofstream out(filepath, ios::binary);
                    if (out) {
                        out.write(file.content.data(), file.content.size());
                        out.close();
                        files_uploaded++;
                        // 保存日志用 UTF-8 文件名
                        saved_files.push_back(file.filename);
                        cout << "Saved file: " << file.filename << " (" << file.content.size() << " bytes)" << endl;
                    } else {
                        cerr << "Failed to save file: " << file.filename << endl;
                    }
                } catch (const exception& e) {
                    cerr << "Error saving file: " << e.what() << endl;
                }
            }
        } else {
            cout << "No files found in request" << endl;
        }
        
        // 返回成功响应（UTF-8 兼容）
        string escaped_path = escape_json_string(target_path_utf8);
        
        ostringstream response_ss;
        response_ss << R"({
    "success": true,
    "message": "Files uploaded successfully",
    "path": ")" << escaped_path << R"(",
    "files_uploaded": )" << files_uploaded << R"(
})";

        res.set_content(response_ss.str(), "application/json; charset=utf-8");
        
    } catch (const exception& e) {
        cerr << "Upload handler exception: " << e.what() << endl;
        res.set_content(generate_json_response(false, string("Upload error: ") + e.what()), 
                       "application/json");
    } catch (...) {
        cerr << "Unknown upload handler exception" << endl;
        res.set_content(generate_json_response(false, "Unknown server error during upload"), 
                       "application/json");
    }
}

void WebServer::handle_scan(const httplib::Request& req, httplib::Response& res) {
    try {
        // 解析请求参数
        auto params = parse_simple_json(req.body);
        
        if (params.find("path") == params.end() || params["path"].empty()) {
            res.set_content(generate_json_response(false, "Missing path parameter"), 
                           "application/json");
            return;
        }
        
        string path_utf8 = params["path"];
        
        // 解析选项
        FileTreeOptions options = parse_tree_options(req.body);
        
        // 扫描目录
        // 注意：这里需要确保 scan_directory 能够处理中文路径
        // 我们不在这里修改 scan_directory，因为这涉及到整个 FileSystemScanner 类的修改
        // 但是我们可以暂时直接传递 utf8 string，然后在 scan_directory 内部处理（如果它已经处理了）
        // 或者我们在这里进行一些预处理。
        // 由于这是一个较大的改动，我们首先修复上传。
        
        // 但为了完整性，我们尝试在这里也使用宽字符路径转换
        // 然而 FileSystemScanner::scan_directory 接受 string 参数
        // 最好是在 main.cpp 或 filesystem.cpp 统一处理，但现在先修复上传的 json 错误。
        // 上传错误是因为 handle_upload 里的路径处理。
        
        vector<FileInfo> files = FileSystemScanner::scan_directory(path_utf8, options);
        
        // 保存扫描结果（即使为空也保存）
        current_scan_.path = path_utf8;
        current_scan_.files = files;
        current_scan_.options = options;
        
        // 生成响应
        string escaped_path = escape_json_string(path_utf8);
        
        ostringstream response_stream;
        response_stream << R"({)" << endl;
        response_stream << R"(    "success": true,)" << endl;
        response_stream << R"(    "message": "Directory scanned successfully",)" << endl;
        response_stream << R"(    "path": ")" << escaped_path << R"(",)" << endl;
        response_stream << R"(    "file_count": )" << files.size() << "," << endl;
        response_stream << R"(    "files": [)" << endl;
        
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& file = files[i];
            response_stream << R"(        {)" << endl;
            // 转义文件名
            response_stream << R"(            "name": ")" << escape_json_string(file.name) << R"(",)" << endl;
            response_stream << R"(            "is_directory": )" << (file.is_directory ? "true" : "false") << "," << endl;
            response_stream << R"(            "depth": )" << file.depth << "," << endl;
            response_stream << R"(            "size": )" << file.size << "," << endl;
            response_stream << R"(            "size_formatted": ")" << FileSystemScanner::format_file_size(file.size, options.human_readable) << R"(")" << endl;
            response_stream << "        }";
            if (i < files.size() - 1) response_stream << ",";
            response_stream << endl;
        }
        
        response_stream << R"(    ])" << endl;
        response_stream << R"(})";
        
        res.set_content(response_stream.str(), "application/json; charset=utf-8");
        
    } catch (const exception& e) {
        res.set_content(generate_json_response(false, "Scan error: " + string(e.what())), 
                       "application/json");
    }
}

void WebServer::handle_tree(const httplib::Request& req, httplib::Response& res) {
    try {
        if (current_scan_.files.empty()) {
            res.set_content(generate_json_response(false, "No scan data available. Please scan a directory first."), 
                           "application/json");
            return;
        }
        
        // 生成文件树文本
        string tree_text = FileSystemScanner::generate_tree_text(current_scan_.files, current_scan_.options);
        
        // 转义字符串中的特殊字符用于JSON
        string escaped_tree = tree_text;
        // 简单的转义：只转义引号和换行
        size_t pos = 0;
        while ((pos = escaped_tree.find('\\', pos)) != string::npos) {
            escaped_tree.replace(pos, 1, "\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped_tree.find('"', pos)) != string::npos) {
            escaped_tree.replace(pos, 1, "\\\"");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped_tree.find('\n', pos)) != string::npos) {
            escaped_tree.replace(pos, 1, "\\n");
            pos += 2;
        }
        
        ostringstream response_stream;
        response_stream << R"({)" << endl;
        response_stream << R"(    "success": true,)" << endl;
        response_stream << R"(    "tree_text": ")" << escaped_tree << R"(",)" << endl;
        response_stream << R"(    "path": ")" << current_scan_.path << R"(",)" << endl;
        response_stream << R"(    "file_count": )" << current_scan_.files.size() << endl;
        response_stream << R"(})";
        
        res.set_content(response_stream.str(), "application/json");
        
    } catch (const exception& e) {
        res.set_content(generate_json_response(false, "Tree generation error: " + string(e.what())), 
                       "application/json");
    }
}

void WebServer::handle_download(const httplib::Request& req, httplib::Response& res) {
    try {
        if (current_scan_.files.empty()) {
            res.set_content("No scan data available. Please scan a directory first.", 
                           "text/plain");
            return;
        }
        
        // 生成文件树文本
        string tree_text = FileSystemScanner::generate_tree_text(current_scan_.files, current_scan_.options);
        
        // 设置下载头
        string filename = "file_tree_" + to_string(time(nullptr)) + ".txt";
        res.set_header("Content-Type", "text/plain");
        res.set_header("Content-Disposition", "attachment; filename=" + filename);
        
        res.set_content(tree_text, "text/plain");
        
    } catch (const exception& e) {
        res.set_content("Error generating download: " + string(e.what()), "text/plain");
    }
}

void WebServer::handle_api_info(const httplib::Request& req, httplib::Response& res) {
    string status = running_ ? "running" : "stopped";
    
    string info = R"({
    "name": "File Manager Web GUI",
    "version": "1.0.0",
    "description": "A web-based file manager with file tree generation",
    "endpoints": [
        {"method": "GET", "path": "/", "description": "Frontend interface"},
        {"method": "POST", "path": "/api/upload", "description": "Upload files/folders"},
        {"method": "POST", "path": "/api/scan", "description": "Scan directory"},
        {"method": "POST", "path": "/api/tree", "description": "Generate file tree"},
        {"method": "GET", "path": "/api/download/tree", "description": "Download file tree as text"},
        {"method": "GET", "path": "/api/info", "description": "API information"}
    ],
    "status": ")" + status + R"(",
    "port": )" + to_string(port_) + R"(
})";
    
    res.set_content(info, "application/json");
}

FileTreeOptions WebServer::parse_tree_options(const std::string& json_str) {
    FileTreeOptions options;
    
    try {
        auto params = parse_simple_json(json_str);
        
        // 解析布尔值
        if (params.find("show_size") != params.end()) {
            options.show_size = (params["show_size"] == "true" || params["show_size"] == "1");
        }
        
        if (params.find("human_readable") != params.end()) {
            options.human_readable = (params["human_readable"] == "true" || params["human_readable"] == "1");
        }
        
        if (params.find("max_depth") != params.end()) {
            try {
                options.max_depth = stoi(params["max_depth"]);
            } catch (...) {
                // 使用默认值
            }
        }
        
        // 解析排除模式（简化版，假设单个值）
        if (params.find("exclude_patterns") != params.end()) {
            options.exclude_patterns.push_back(params["exclude_patterns"]);
        }
        
    } catch (...) {
        // 使用默认选项
    }
    
    return options;
}

std::string WebServer::generate_json_response(bool success, 
                                             const std::string& message, 
                                             const std::string& data) {
    ostringstream response;
    string escaped_message = escape_json_string(message);
    string escaped_data = escape_json_string(data);
    
    response << R"({)" << endl;
    response << R"(    "success": )" << (success ? "true" : "false") << "," << endl;
    response << R"(    "message": ")" << escaped_message << "\"";
    
    if (!data.empty()) {
        response << "," << endl << R"(    "data": ")" << escaped_data << "\"";
    }
    
    response << endl << R"(})";
    return response.str();
}

std::map<std::string, std::string> WebServer::parse_simple_json(const std::string& json_str) {
    std::map<std::string, std::string> result;
    
    if (json_str.empty() || json_str[0] != '{') {
        return result;
    }
    
    // 简单的JSON解析器 - 只处理字符串值的键值对
    string content = json_str.substr(1);  // 跳过开始的 {
    
    // 查找所有的 "key": "value" 对
    regex pattern("\\\"([^\\\"]+)\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    
    smatch match;
    string::const_iterator searchStart(content.cbegin());
    while (regex_search(searchStart, content.cend(), match, pattern)) {
        result[match[1].str()] = match[2].str();
        searchStart = match.suffix().first;
    }
    
    // 也支持布尔值和数字
    regex bool_pattern("\\\"([^\\\"]+)\\\"\\s*:\\s*(true|false|null|-?\\d+\\.?\\d*)");
    searchStart = content.cbegin();
    while (regex_search(searchStart, content.cend(), match, bool_pattern)) {
        result[match[1].str()] = match[2].str();
        searchStart = match.suffix().first;
    }
    
    return result;
}
