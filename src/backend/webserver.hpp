#pragma once

#include "filesystem.hpp"
#include "httplib.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <map>

class WebServer {
public:
    WebServer();
    ~WebServer();
    
    // 启动HTTP服务器
    bool start(int port = 8080);
    
    // 停止HTTP服务器
    void stop();
    
    // 检查服务器是否正在运行
    bool is_running() const { return running_; }
    
    // 获取服务器端口
    int get_port() const { return port_; }
    
private:
    // 设置路由
    void setup_routes();
    
    // HTTP请求处理函数
    void handle_root(const httplib::Request& req, httplib::Response& res);
    void handle_upload(const httplib::Request& req, httplib::Response& res);
    void handle_scan(const httplib::Request& req, httplib::Response& res);
    void handle_tree(const httplib::Request& req, httplib::Response& res);
    void handle_download(const httplib::Request& req, httplib::Response& res);
    void handle_api_info(const httplib::Request& req, httplib::Response& res);
    
    // 解析JSON请求（简化版）
    FileTreeOptions parse_tree_options(const std::string& json_str);
    
    // 解析简单JSON
    std::map<std::string, std::string> parse_simple_json(const std::string& json_str);
    
    // 生成JSON响应
    std::string generate_json_response(bool success, 
                                      const std::string& message = "", 
                                      const std::string& data = "");
    
    // 服务器实例
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<std::thread> server_thread_;
    
    // 服务器状态
    std::atomic<bool> running_{false};
    int port_{8080};
    
    // 上传文件存储目录
    std::string upload_dir_{"uploads"};
    
    // 当前扫描的目录信息
    struct {
        std::string path;
        std::vector<FileInfo> files;
        FileTreeOptions options;
    } current_scan_;
};