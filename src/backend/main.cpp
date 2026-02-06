#include "webserver.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <filesystem>
#include <cstdlib>

using namespace std;
namespace fs = std::filesystem;

atomic<bool> running(true);

// 信号处理函数
void signal_handler(int signal) {
    cout << "\nReceived signal " << signal << ", shutting down..." << endl;
    running = false;
}

void print_help() {
    cout << "File Manager Web GUI" << endl;
    cout << "=====================" << endl;
    cout << "Usage:" << endl;
    cout << "  ./filemanager [port]" << endl;
    cout << endl;
    cout << "Arguments:" << endl;
    cout << "  port      Port number for the web server (default: 8080)" << endl;
    cout << endl;
    cout << "Features:" << endl;
    cout << "  • Modern web-based GUI" << endl;
    cout << "  • Folder upload and scanning" << endl;
    cout << "  • File tree generation with tab formatting" << endl;
    cout << "  • Optional file size display" << endl;
    cout << "  • Downloadable file tree output" << endl;
    cout << endl;
    cout << "Once started, open http://localhost:<port> in your browser" << endl;
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 解析命令行参数
    int port = 8080;
    
    if (argc > 1) {
        string arg1 = argv[1];
        if (arg1 == "--help" || arg1 == "-h") {
            print_help();
            return 0;
        }
        
        try {
            port = stoi(arg1);
            if (port < 1 || port > 65535) {
                cerr << "Error: Port must be between 1 and 65535" << endl;
                return 1;
            }
        } catch (const exception&) {
            cerr << "Error: Invalid port number" << endl;
            print_help();
            return 1;
        }
    }
    
    cout << "Starting File Manager Web GUI..." << endl;
    
    // 获取可执行文件所在目录并更改工作目录
    try {
        // argv[0] 包含程序的路径
        fs::path exe_path(argv[0]);
        fs::path exe_dir = exe_path.parent_path();
        
        // 如果是相对路径，需要转换为绝对路径
        if (!exe_dir.is_absolute()) {
            exe_dir = fs::absolute(exe_dir);
        }
        
        // 更改工作目录到可执行文件所在目录
        fs::current_path(exe_dir);
        cout << "Working directory: " << fs::current_path().string() << endl;
    } catch (const exception& e) {
        cerr << "Warning: Could not set working directory: " << e.what() << endl;
    }
    
    // 创建并启动Web服务器
    WebServer server;
    
    if (!server.start(port)) {
        cerr << "Failed to start web server" << endl;
        return 1;
    }
    
    cout << "Server is running on port " << port << endl;
    cout << "Press Ctrl+C to stop the server" << endl;
    cout << endl;
    
    // 主循环
    while (running && server.is_running()) {
        // 可以在这里添加其他后台任务
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    // 停止服务器
    server.stop();
    
    cout << "File Manager Web GUI stopped" << endl;
    return 0;
}