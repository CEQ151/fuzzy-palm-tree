<div align="center">

**English** | [简体中文](./README_zh.md)

</div>

---

# File Manager Web GUI V1.0

<div align="center">
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)

A high-performance directory scanning and file tree generation tool built with C++17 and modern Web technologies.
Provides an intuitive graphical interface to browse local file system structures, generate customizable text-based directory trees, and export results.

[Features](#-features) • [Quick Start](#-quick-start) • [Usage Guide](#-usage-guide) • [API Documentation](#-api-documentation)

</div>

---

## ✨ Features

* **📂 Directory Scanning**: Fast recursive scanning of large directories powered by the C++ native filesystem API.
* **🌳 Smart Tree Generation**: Automatically generates beautiful ASCII/Unicode text file trees (similar to the Linux `tree` command).
* **🎨 Modern Web Interface**: Clean and responsive UI; no extra client-side software required.
* **⚙️ Flexible Filter Configuration**:
    * **Depth Control**: Set maximum scanning depth.
    * **Exclude Patterns**: Support for wildcard exclusions of unwanted files or folders (e.g., `node_modules`, `.git`).
    * **Display Options**: Optional file size display (supports automatic KB/MB/GB formatting).
* **📋 Convenient Export**:
    * One-click copy of the file tree text to the clipboard.
    * Download scanning results as a `.txt` file.
* **🚀 Lightweight Backend**: Single executable based on `cpp-httplib` with low memory footprint.
* **🌏 Full Unicode Support**: Optimized for Windows environments, perfectly supporting the scanning and display of Chinese paths and filenames.

---

## 🚀 Quick Start

### 1. Requirements

* **OS**: Windows 10/11, Linux, macOS.
* **Compiler**: A compiler supporting C++17 (GCC 8+, Clang, MSVC).
    * *Windows users are recommended to use MSYS2/MinGW64 environment*.
* **Build Tool**: CMake 3.15 or higher.

### 2. Get the Source Code

```bash
git clone [https://github.com/CEQ151/fuzzy-palm-tree.git](https://github.com/CEQ151/fuzzy-palm-tree.git)
cd fuzzy-palm-tree
```

### 3. Compile the Project

Execute the following commands in the project root directory:

```bash
# Create and enter the build directory
mkdir build
cd build

# Configure CMake (Auto-detect environment)
cmake ..

# Start compilation
cmake --build .
```

*Once compilation is complete, the executable `filemanager` (or `filemanager.exe`) will be located in the `build/bin/` directory.*

### 4. Run the Service

```bash
# Enter the output directory
cd bin

# Start the server (default port 8080)
./filemanager

# Or specify a port
./filemanager 9090
```

After a successful start, the terminal will display:

> Server is running on port 8080
>
> Press Ctrl+C to stop the server

Access the interface in your browser at: **http://localhost:8080**

------

## 📖 Usage Guide

### 🖥️ Scanning a Directory

1. Enter the **local absolute path** you wish to scan in the **"Directory Path"** input box at the top.
   - *Windows Example*: `D:\Projects\MyCode` or `C:/Users/Admin/Documents`
   - *Linux/Mac Example*: `/home/user/projects`
2. Click the **"Scan Directory"** button or press Enter.
3. The system will quickly scan the directory and display the generated file tree preview on the right side of the page.

### ⚙️ Custom Settings (Left Panel)

- **Show file sizes**: When checked, the size of each file will be shown in the tree diagram.
- **Max depth**: Limits the depth of recursive scanning. Enter `-1` for unlimited depth (all subdirectories).
- **Exclude patterns**: Enter file or folder names to ignore, separated by commas.
  - *Example*: `node_modules, .git, *.tmp, dist`

### 📤 Exporting Results

After scanning, you can use the toolbar at the top right:

- **📋 Copy**: Copy the entire generated file tree text to your clipboard.
- **💾 Download**: Save the file tree as a `.txt` file locally.
- **🗑️ Clear**: Clear the current display results.

------

## 🔌 API Documentation

The backend provides a RESTful API for automation scripts or other tools.

### 1. Get Server Info

- **Endpoint**: `GET /api/info`
- **Description**: Check server status and version.

### 2. Scan Directory

- **Endpoint**: `POST /api/scan`

- **Description**: Scans a specified directory and returns a file list in JSON format.

- **Request Body (JSON)**:

  ```json
  {
      "path": "C:/Projects/Demo",
      "max_depth": -1,
      "show_size": true,
      "exclude_patterns": ["node_modules", ".git"]
  }
  ```

### 3. Generate Tree Text

- **Endpoint**: `POST /api/tree`

- **Description**: Directly returns the formatted tree structure text.

- **Request Body**: Same as `/api/scan`.

- **Response (JSON)**:

  ```json
  {
      "success": true,
      "tree_text": "📁 Demo/\n├── 📄 main.cpp (1.2 KB)\n└── ...",
      "file_count": 15
  }
  ```

------

## 📂 Project Structure

```
fuzzy-palm-tree/
├── CMakeLists.txt        # CMake build script
├── include/
│   └── httplib.h         # Header for the HTTP server library
├── src/
│   ├── backend/          # C++ Backend core code
│   │   ├── main.cpp      # Entry point and argument parsing
│   │   ├── webserver.* # Web server and API implementation
│   │   └── filesystem.* # File scanning and tree generation logic
│   └── frontend/         # Web Frontend assets
│       ├── index.html    # Main interface
│       ├── script.js     # Frontend interaction logic
│       └── style.css     # Interface styling
└── build/                # Build output directory (auto-generated)
```

## ⚠️ Notes

1. **Security Warning**: This tool is designed for trusted local network environments. It allows access to the filesystem of the host running the server. **DO NOT** expose it to the public internet.
2. **Path Formats**: Supports both forward slashes `/` and backslashes `\` on Windows.
3. **Permissions**: Ensure the user running the program has read permissions for the target scanning directories.