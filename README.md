# JSON 阅读器

> 粘贴 JSON 即可展开/收拢查看的轻量级桌面工具。

---

[English](#english) | [中文](#中文)

---

## 中文

### 功能

| 功能 | 说明 | 快捷键 |
|------|------|--------|
| 自动解析 | 左侧粘贴 JSON，右侧实时渲染树形结构 | — |
| 导入文件 | 打开本地 .json / .har / .txt / .jsonc 文件 | `Ctrl+O` |
| 拖拽导入 | 拖拽文件到窗口直接打开 | — |
| HAR 识别 | 自动识别 HAR 格式，显示请求数/域名统计 | — |
| 命令行打开 | 支持 `JSON阅读器.exe myfile.json` 或双击关联 | — |
| 展开/收拢 | 点击节点前 `▼` 箭头折叠或展开 | — |
| 全部展开/收拢 | 工具栏一键操作 | — |
| 格式化 | 美化缩进 JSON | `Ctrl+Enter` |
| 压缩 | 压缩为一行 | — |
| 搜索 | 搜索键名或值，上下导航 | `Ctrl+F` |
| 复制键值对 | 悬停任意行点 📋，复制完整 `"key": value` | — |
| 复制整块 | 悬停对象/数组行点 📋块，复制整个子树 JSON | — |
| 花括号匹配 | 点击右侧树节点，左侧自动定位对应 JSON 块 | — |
| 面板折叠 | 左右面板可独立收起，小屏友好 | `Ctrl+B` |
| 主题切换 | 浅色/深色一键切换，自动记忆偏好 | — |
| 语法高亮 | 字符串/数字/布尔/null/键名分色显示 | — |

## 项目结构

```
json-viewer/
├── index.html          # 主界面（单文件全功能）
├── main.js             # Electron 主进程（文件关联/命令行支持）
├── preload.js          # 预加载脚本（IPC 桥接）
├── package.json        # npm 配置与依赖
├── icon.png            # 应用图标 PNG（256×256）
├── icon.ico            # 应用图标 ICO（多尺寸）
├── .gitignore          # Git 忽略规则
├── 启动JSON阅读器.bat   # Windows 备用启动脚本
└── README.md           # 本文件
```

## 技术栈

- **前端**：纯 HTML/CSS/JS，零框架依赖，单文件即可运行
- **桌面壳**：Electron 33
- **打包**：electron-builder（可选，生成独立 .exe）

## 快速开始

### 方式一：浏览器直接打开（推荐，无需安装）

双击 `index.html` 即可在浏览器中使用，功能完整。

### 方式二：Electron 桌面应用

```bash
# 1. 安装依赖（仅首次）
npm install

# 2. 启动
npm start
```

### 方式三：打包为独立 EXE

```bash
# 安装打包工具
npm install --save-dev electron-builder

# 打包为便携版 EXE
npm run build
# 输出在 dist/JSON阅读器.exe
```

## 移植到其他系统

### Windows → macOS

```bash
# 1. 复制整个 json-viewer 文件夹到 macOS
# 2. 安装依赖
cd json-viewer
npm install

# 3. 启动
npm start

# 4. 打包为 macOS 应用（可选）
# 修改 package.json 中 build.win 为 build.mac
npm run build
```

### Windows → Linux

```bash
# 同上，打包时改为 build.linux
```

### 纯浏览器模式（跨平台零依赖）

直接复制 `index.html` 到任意设备，浏览器打开即用。

## 分发方式

| 方式 | 适用场景 | 文件 |
|------|----------|------|
| 源码分发 | 开发者、需要定制 | 整个文件夹（不含 node_modules） |
| EXE 分发 | Windows 普通用户 | `dist/JSON阅读器.exe` |
| 网页分发 | 任何平台、无需安装 | `index.html` 单个文件 |

## 自定义

### 修改默认主题

编辑 `index.html` 第 2 行：

```html
<!-- 默认浅色 -->
<html lang="zh-CN" data-theme="light">

<!-- 改为默认深色 -->
<html lang="zh-CN" data-theme="dark">
```

### 修改应用名称/图标

1. 替换 `icon.png`（保持 64×64 以上）
2. 修改 `package.json` 中 `productName` 和 `appId`

## 依赖

- **运行时**：仅 Electron（桌面模式），浏览器模式零依赖
- **开发**：Node.js ≥ 18，npm ≥ 9

## 许可

MIT


---

## English

A lightweight desktop tool for viewing and navigating JSON data with a tree structure.

**Features:**
- **Auto-parsing** — Paste JSON on the left, rendered tree on the right
- **Collapse/Expand** — Click `▼` arrows to fold/unfold nodes
- **Format & Minify** — Beautify or compress JSON
- **Search** — `Ctrl+F` to search keys/values with up/down navigation
- **Copy** — Copy key-value pairs or entire subtrees with one click
- **Brace Matching** — Click tree nodes to locate corresponding JSON blocks
- **Panel Collapse** — `Ctrl+B` to hide panels independently
- **Theme Switch** — Light/Dark with auto-saved preference
- **Syntax Highlighting** — Color-coded strings, numbers, booleans, nulls, keys

### Quick Start
```bash
# Open in browser (no install needed)
open index.html

# Or as Electron app
npm install && npm start
```

---

## 🤖 AI 辅助声明

本项目由 AI 智能体（Hermes Agent by Nous Research）辅助开发。从需求分析、UI 设计到代码实现、文档撰写均由 AI 在人类指导下完成。人工负责需求提出、代码审查和最终发布。

## 🤖 AI Assistance Declaration

This project was developed with assistance from an AI agent (Hermes Agent by Nous Research). From requirements analysis and UI design to code implementation and documentation, all work was done by AI under human guidance. Human contributors were responsible for requirements definition, code review, and final release.
