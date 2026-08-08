# JSON 阅读器 / JSON Viewer

[🇬🇧 English](#-english) | [🇨🇳 中文](#-中文)

---

> 粘贴 JSON 即可展开/收拢查看的轻量级桌面工具，基于 **Qt 6**（C++）。
> A lightweight desktop JSON viewer built with **Qt 6** (C++).

## 🇨🇳 中文

### 简介

JSON 阅读器是一个树形结构查看 JSON / HAR 文件的桌面工具：左侧粘贴或打开文件，右侧实时渲染彩色树形结构，支持搜索、格式化、压缩、主题切换。

**v2.0 重大变更**：从 Electron 33 完全重写为 Qt 6 Widgets（C++/CMake），内存占用从 **724 MB 降至 88 MB**（约省 8 倍），二进制从 186 MB 缩至 275 KB。

### 功能

| 功能 | 说明 | 快捷键 |
|------|------|--------|
| 自动解析 | 左侧粘贴 JSON，右侧实时渲染树形结构 | — |
| 导入文件 | 打开本地 .json / .har / .txt / .jsonc 文件 | `Ctrl+O` |
| 拖拽导入 | 拖拽文件到窗口直接打开 | — |
| 命令行打开 | `json-viewer-qt myfile.json` 或双击文件关联 | — |
| HAR 识别 | 自动识别 HAR 格式，显示请求数/域名/方法统计 | — |
| 展开/收拢 | 点击节点前箭头折叠或展开 | — |
| 全部展开/收拢 | 工具栏一键操作 | — |
| 格式化 | 美化缩进 JSON | `Ctrl+Enter` |
| 压缩 | 压缩为一行 | `Ctrl+Shift+C` |
| 搜索 | 搜索键名或值，智能展开 + 上下导航 | `Ctrl+F` |
| 复制 | 右键复制值/键值对/整块/路径 | — |
| 花括号匹配 | 点击右侧树节点，左侧自动定位对应 JSON 块 | — |
| 面板折叠 | 左右面板可独立收起，小屏友好 | `Ctrl+B` |
| 主题切换 | 浅色/深色一键切换，自动记忆偏好 | — |
| 语法高亮 | 字符串/数字/布尔/null/键名分色显示 | — |
| 大文件 | >50KB 直接解析树（不进编辑框），>1000 子项截断 | — |
| 单实例 | 重复启动自动转发文件到已有窗口 | — |

### 快速开始

**Linux (Fedora/RHEL)**：

```bash
# 依赖：qt6-qtbase >= 6.5
sudo dnf install -y qt6-qtbase cmake gcc-c++
git clone https://github.com/raymond202202/json-viewer.git
cd json-viewer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/json-viewer-qt
```

**Windows / macOS**：使用 GitHub Actions Release 或自行 `cmake` 构建（Qt 6 跨平台）。

### 性能对比（实测）

| 版本 | 空闲内存 | 进程数 | 二进制 |
|------|---------|--------|--------|
| Electron 33（v1.x） | 724 MB | 10 | 186 MB |
| Qt 6（v2.0） | **88 MB** | 1 | **275 KB** |

### 项目结构

```
json-viewer/
├── CMakeLists.txt          # CMake 构建（Qt6 Widgets + Network）
├── src/
│   ├── main.cpp            # 入口 + 单实例 + 命令行参数
│   ├── MainWindow.h/.cpp   # 主窗口：工具栏/搜索/主题/拖放/联动
│   ├── JsonTreeModel.h/.cpp    # 懒加载树模型（按需 fetchMore）
│   └── JsonTreeDelegate.h/.cpp # 彩色渲染 + 高亮
├── resources/
│   ├── icon.png
│   └── styles/ light.qss dark.qss
├── packaging/              # RPM spec + .desktop
├── tests/                  # 模型自测
└── .github/workflows/      # CI + Release（tag v* 触发）
```

### 许可证

MIT License

> 本项目由 Hermes Agent (AI) 辅助开发，Electron v1.x 历史版本保留在 git 历史（tag v1.1.0）。

---

## 🇬🇧 English

### Overview

A lightweight desktop tool for viewing JSON / HAR files in a tree structure. Paste JSON on the left or open a file — the right side renders a color-coded tree in real time. Supports search, format, compress, and theme switching.

**v2.0 major change**: Fully rewritten from Electron 33 to Qt 6 Widgets (C++/CMake). Memory usage dropped from **724 MB to 88 MB** (~8x saving), binary from 186 MB to 275 KB.

### Features

| Feature | Description | Shortcut |
|---------|-------------|----------|
| Auto-parse | Paste JSON, tree renders instantly | — |
| Open file | .json / .har / .txt / .jsonc | `Ctrl+O` |
| Drag & drop | Drop a file onto the window | — |
| CLI open | `json-viewer-qt myfile.json` | — |
| HAR detect | Request/domain/method stats | — |
| Expand/collapse | Click node arrows | — |
| Expand/collapse all | Toolbar buttons | — |
| Format | Pretty-print JSON | `Ctrl+Enter` |
| Compress | Minify to one line | `Ctrl+Shift+C` |
| Search | Key or value, smart expand, prev/next | `Ctrl+F` |
| Copy | Value / key-value / block / path (context menu) | — |
| Brace match | Click tree node, left editor locates the block | — |
| Panel collapse | Collapse left/right panels (mutually exclusive) | `Ctrl+B` |
| Theme | Light/dark toggle, remembered | — |
| Syntax colors | String/number/bool/null/key colors | — |
| Large files | >50KB parsed directly, >1000 children truncated | — |
| Single instance | Second launch forwards file to existing window | — |

### Quick Start

**Linux (Fedora/RHEL)**:

```bash
# deps: qt6-qtbase >= 6.5
sudo dnf install -y qt6-qtbase cmake gcc-c++
git clone https://github.com/raymond202202/json-viewer.git
cd json-viewer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/json-viewer-qt
```

**Windows / macOS**: Build with CMake (Qt 6 is cross-platform), or grab a Release artifact.

### Measured Performance

| Version | Idle memory | Processes | Binary |
|---------|-------------|-----------|--------|
| Electron 33 (v1.x) | 724 MB | 10 | 186 MB |
| Qt 6 (v2.0) | **88 MB** | 1 | **275 KB** |

### License

MIT License

> Developed with assistance from Hermes Agent (AI). The Electron v1.x codebase remains in git history (tag v1.1.0).
