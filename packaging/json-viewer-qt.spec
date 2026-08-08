Name:           json-viewer-qt
Version:        2.0.2
Release:        1%{?dist}
Summary:        轻量级 JSON 阅读器（Qt 版）— 粘贴/打开 JSON 以树形结构查看

License:        MIT
URL:            https://github.com/raymond202202/json-viewer
Source0:        %{name}-%{version}.tar.xz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
Requires:       qt6-qtbase >= 6.5

%description
JSON 阅读器 Qt 版：粘贴 JSON 或打开 .json/.har/.txt/.jsonc 文件，
右侧以彩色树形结构展示，支持搜索/高亮/格式化/压缩/主题切换。
基于 Qt 6 Widgets，内存占用约 90MB（Electron 版约 724MB）。

%prep
%setup -q

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%{_bindir}/json-viewer-qt
%{_datadir}/json-viewer-qt/icon.png
%{_datadir}/applications/json-viewer-qt.desktop

%changelog
* Sat Aug 08 2026 raymond202202 <raymond202202@github> - 1.1.0-1
- Qt 版初版：功能对齐 Electron 版 1.1.0（树形/搜索/HAR/格式化/主题）
