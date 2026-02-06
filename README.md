# File Manager Web GUI V1.0

<div align="center">

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)

一个基于 C++17 和现代 Web 技术构建的高性能目录扫描与文件树生成工具。
提供直观的图形界面来浏览本地文件系统结构、生成可定制的文本目录树，并支持导出功能。

[功能特性](#-功能特性) • [快速开始](#-快速开始) • [使用指南](#-使用指南) • [API 文档](#-api-文档)

</div>

---

## ✨ 功能特性

*   **📂 目录扫描**: 基于 C++ 本地文件系统 API，快速递归扫描大型目录。
*   **🌳 智能树状生成**: 自动生成美观的 ASCII/Unicode 文本格式文件树（类似 Linux `tree` 命令）。
*   **🎨 现代化 Web 界面**: 简洁响应式的 UI，无需安装额外的客户端软件。
*   **⚙️ 灵活的过滤配置**:
    *   **深度控制**: 可设置最大扫描深度。
    *   **排除模式**: 支持通配符排除不需要的文件或文件夹（如 `node_modules`, `.git`）。
    *   **显示选项**: 可选显示文件大小（支持 KB/MB/GB 自动格式化）。
*   **📋 便捷导出**:
    *   一键复制文件树文本到剪贴板。
    *   下载扫描结果为 `.txt` 文件。
*   **🚀 轻量级后端**: 单个可执行文件，基于 `cpp-httplib`，低内存占用。
*   **🌏 完整中文支持**: 针对 Windows 环境优化，完美支持中文路径和文件名的扫描与显示。

---

## 🚀 快速开始

### 1. 环境要求

*   **操作系统**: Windows 10/11, Linux, macOS。
*   **编译器**: 支持 C++17 的编译器 (GCC 8+, Clang, MSVC)。
    *   *Windows 用户推荐使用 MSYS2/MinGW64 环境*。
*   **构建工具**: CMake 3.15 或更高版本。

### 2. 获取源码

```bash
git clone https://github.com/CEQ151/fuzzy-palm-tree.git
cd fuzzy-palm-tree
```

### 3. 编译项目

在项目根目录下执行以下命令：

```bash
# 创建并进入构建目录
mkdir build
cd build

# 配置 CMake (自动检测环境)
cmake ..

# 开始编译
cmake --build .
```

*编译完成后，可执行文件 `filemanager` (或 `filemanager.exe`) 将位于 `build/bin/` 目录下。*

### 4. 运行服务

```bash
# 进入输出目录
cd bin

# 启动服务器 (默认端口 8080)
./filemanager

# 或者指定端口启动
./filemanager 9090
```

启动成功后，终端会显示：
> Server is running on port 8080
> Press Ctrl+C to stop the server

此时请在浏览器中访问：**http://localhost:8080**

---

## 📖 使用指南

### 🖥️ 扫描目录
1.  在页面顶部的 **"Directory Path"** 输入框中，输入您想要扫描的 **本地绝对路径**。
    *   *Windows 示例*: `D:\Projects\MyCode` 或 `C:/Users/Admin/Documents`
    *   *Linux/Mac 示例*: `/home/user/projects`
2.  点击 **"Scan Directory"** 按钮或直接按回车键。
3.  系统将快速扫描该目录，并在页面右侧显示生成的文件树预览。

### ⚙️ 自定义设置 (左侧面板)
*   **Show file sizes**: 勾选后，树状图中将显示每个文件的大小。
*   **Max depth**: 限制扫描的层级深度。输入 `-1` 表示无限制（递归所有子目录）。
*   **Exclude patterns**: 输入要忽略的文件或文件夹名称，使用逗号分隔。
    *   *示例*: `node_modules, .git, *.tmp, dist`

### 📤 导出结果
扫描完成后，您可以使用右侧顶部的工具栏：
*   **📋 Copy**: 将生成的文件树文本完整复制到剪贴板。
*   **💾 Download**: 将文件树保存为 `.txt` 文本文件到本地。
*   **🗑️ Clear**: 清空当前的显示结果。

---

## 🔌 API 文档

后端提供 RESTful API，可供自动化脚本或其他工具调用。

### 1. 获取服务器信息
*   **接口**: `GET /api/info`
*   **描述**: 检查服务器状态及版本。

### 2. 扫描目录
*   **接口**: `POST /api/scan`
*   **描述**: 扫描指定目录并返回 JSON 格式的文件列表。
*   **请求体 (JSON)**:
    ```json
    {
        "path": "C:/Projects/Demo",
        "max_depth": -1,
        "show_size": true,
        "exclude_patterns": ["node_modules", ".git"]
    }
    ```

### 3. 生成树文本
*   **接口**: `POST /api/tree`
*   **描述**: 直接返回格式化好的树状结构文本。
*   **请求体**: 同 `/api/scan`。
*   **响应 (JSON)**:
    ```json
    {
        "success": true,
        "tree_text": "📁 Demo/\n├── 📄 main.cpp (1.2 KB)\n└── ...",
        "file_count": 15
    }
    ```

---

## 📂 项目结构

```text
fuzzy-palm-tree/
├── CMakeLists.txt       # CMake 构建脚本
├── include/
│   └── httplib.h        # HTTP 服务器库头文件
├── src/
│   ├── backend/         # C++ 后端核心代码
│   │   ├── main.cpp     # 程序入口与参数解析
│   │   ├── webserver.*  # Web 服务器与 API 实现
│   │   └── filesystem.* # 文件扫描与树生成逻辑
│   └── frontend/        # Web 前端资源
│       ├── index.html   # 主界面
│       ├── script.js    # 前端交互逻辑
│       └── style.css    # 界面样式
└── build/               # 编译输出目录 (自动生成)
```

## ⚠️ 注意事项

1.  **安全提示**: 本工具设计用于本地受信任网络环境。它允许访问运行服务器的主机上的文件系统，**切勿**将其暴露在公共互联网上。
2.  **路径格式**: 在 Windows 上支持使用正斜杠 `/` 或反斜杠 `\`。
3.  **权限**: 确保运行程序的用户对目标扫描目录拥有读取权限。

## 📄 许可证

本项目采用 **MIT 许可证** 开源。

