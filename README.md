![WmaskEX](./screenshots/logo.png)

# WmaskEX

**为任意窗口提供动态桌面叠加效果**

英文文档 / English documentation: [README.en.md](./README.en.md)

[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows) [![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)

## ✨ 核心特性

- 🚀 **极致轻量**：基于纯 Win32 API 构建 GUI，启动迅速
- 🔒 **完全便携**：无 Hook、无注册表修改，100% 绿色
- ⚡ **低资源占用**：针对性能进行优化
- 🎭 **多版本 Spine 支持**：兼容 Spine 3.7、3.8、4.0、4.1、4.2
- 🎯 **智能应用匹配**：按应用名匹配并附着叠加层（例如 explorer.exe）
- 🎨 **动态响应布局**：随窗口尺寸变化自动调整位置
- 🔄 **自动随机轮播**：多个资源间随机切换

## ⚙️ 配置参数说明

![Configuration Interface](./screenshots/000.png)

| 参数       | 说明                 | 类型        | 详情                                                         |
| ---------- | -------------------- | ----------- | ------------------------------------------------------------ |
| name       | 配置名称             | String      | 配置的唯一标识                                               |
| parent     | 父窗口               | String      | 目标父窗口进程名（例如 `explorer.exe`）                      |
| assets     | 资源目录             | Path        | 存放叠加资源的目录                                           |
| preview    | 预览图               | Path        | 预览图片路径                                                 |
| size       | 尺寸适配模式         | Enum        | Fill / Fit / Follow Height / Follow Width / Fixed Size      |
| scale      | 缩放比例             | Number (%)  | 在尺寸适配后应用的缩放系数                                   |
| horizontal | 水平位置             | Number (%)  | 在父窗口中的水平位置（0-100+）<br/>0/100 = 左/右边缘对齐    |
| x shift    | 水平偏移             | Number (px) | 水平方向像素偏移                                             |
| vertical   | 垂直位置             | Number (%)  | 在父窗口中的垂直位置（0-100+）<br/>0/100 = 下/上边缘对齐    |
| y shift    | 垂直偏移             | Number (px) | 垂直方向像素偏移                                             |
| duration   | 轮播时长             | Time (sec)  | 资源切换间隔                                                 |
| opacity    | 透明度               | Number      | 显示透明度（0-255）                                          |
| pma        | PMA 默认值           | Checkbox    | 勾选表示默认使用 `true`，未勾选表示默认使用 `false`         |

## 📁 资源管理

### 🖼️ 图片资源

WmaskEX **仅在资源目录顶层**（不递归）查找图片文件，支持以下扩展名（不区分大小写）：
- **支持格式**：`png`、`jpg`、`jpeg`、`bmp`、`ico`、`tiff`、`exif`、`wmf`、`emf`

### 🎭 Spine 动画资源

WmaskEX 会通过检测 `.atlas` 文件并匹配对应骨骼文件，**递归查找** Spine 动画资源：

**Spine 资源识别所需文件：**
- ✅ `.atlas` 文件（纹理图集）
- ✅ 与其**同名**的 `.json` 或 `.skel` 文件（骨骼数据）

#### 自动解析行为

WmaskEX 会自动执行：
- 📊 **版本识别**：从 `.skel` 或 `.json` 中提取 Spine 版本
- 📐 **边界计算**：读取骨骼边界（x、y、width、height）
- 🎨 **PMA 检测**：从 `.atlas` 中解析预乘 Alpha 设置；若未检测到，则使用主界面的 PMA 默认值

## 🛠️ 安装与使用

### 系统要求
- Windows 7/8/10/11 (64-bit)
- Visual C++ Redistributable
- OpenGL support

### 快速开始
1. 下载最新发布版本
2. 解压到你希望放置的目录
3. 运行 `WmaskEX.exe`
4. 在 GUI 中配置叠加效果
5. 体验增强后的桌面效果

### 从源码构建

```powershell
# Install dependencies with vcpkg
vcpkg install nlohmann-json glbinding

# Clone and build
git clone https://github.com/wang606/WmaskEX.git
cd WmaskEX
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## 🎮 效果展示

<img src="./screenshots/001.gif" width="100%">

<img src="./screenshots/002.gif" width="100%">

<img src="./screenshots/003.png" width="100%">

<img src="./screenshots/004.gif" width="100%">

<img src="./screenshots/005.png" width="100%">

---

**⭐ 如果这个项目对你有帮助，欢迎点个 Star！⭐**

