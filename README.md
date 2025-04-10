<span style="font-size:48px;"><span style="font-size:14px;"><span style="color:#e60000;">******<span style="font-size:36px;">请务必先用JavSP刮削器下载封面图后再用本工具导入"整理完成" 的视频文件夹

  JavSP下载地址
  
  https://github.com/Yuukiy/JavSP/releases/tag/v1.8</span>*</span>****</span></span></span>


# JavArk

一个针对Windows系统下，管理AV视频的本地视频管理库应用程序。

## 功能特点

- 记录用户添加的视频库路径
- 扫描并显示视频库中的所有视频
- 使用视频文件夹中的poster.jpg和fanart.jpg作为视频封面
- 点击视频封面，使用系统默认播放器播放视频
- **异步加载视频封面，加快初始扫描速度**
- **支持海报模式和背景图模式切换**
- **自定义调整封面图大小**
- **自适应网格布局，确保所有封面完整显示**
- **视频文件按创建时间、修改时间或文件名排序**
- **左下角显示扫描进度条，改善用户体验**
- **内置现代深色主题，减轻眼睛疲劳**
- **支持自定义应用程序图标**
- 多线程扫描提高性能
- 针对Windows系统优化

## 系统要求

- Windows 10或更高版本
- Qt 6.0或更高版本
- CMake 3.16或更高版本
- C++17兼容的编译器(MSVC 2019或更高版本)

## 构建步骤

1. 安装Qt 6
2. 安装CMake
3. 安装Visual Studio 2022或更高版本(包含MSVC编译器)
4. 克隆或下载本项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake项目
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:\Qt\6.5.0\msvc2019_64 ..

# 构建项目
cmake --build . --config Release
```

注意：请将`CMAKE_PREFIX_PATH`调整为您的Qt安装路径。

## 使用说明

1. 运行程序
2. 点击"添加目录"按钮，选择包含视频的文件夹
3. 应用程序将自动扫描所选文件夹及其子文件夹中的视频
4. 点击视频缩略图可以使用系统默认播放器播放视频
5. 使用"扫描媒体库"按钮重新扫描媒体库
6. 使用"移除目录"按钮从媒体库中移除目录
7. **使用"切换封面"按钮在海报模式和背景图模式之间切换**
8. **使用"+"和"-"按钮调整封面图大小**






## 视频封面图片

应用程序会在视频所在文件夹中查找以下图片文件：

- `poster.jpg` - 用作视频的海报缩略图
- `fanart.jpg` - 用作视频的背景图片

如果这些文件不存在，应用程序将使用默认图片。

## 界面特性

- **自适应网格布局** - 根据窗口大小和封面图尺寸自动调整每行显示的视频数量
- **动态缩略图大小调整** - 用户可以自由调整封面图大小
- **海报/背景图切换** - 可以在垂直海报图和水平背景图之间切换
- **鼠标悬停效果** - 在视频缩略图上悬停鼠标时显示播放按钮
- **排序功能** - 支持按创建时间、修改时间或文件名排序视频
- **扫描进度条** - 位于左下角的进度条直观显示扫描状态
- **深色主题** - 灰黑色的界面配色，更加美观且减轻眼睛疲劳

## 性能优化

本应用采用了以下性能优化措施：

1. 多线程扫描视频文件夹
2. **异步生成缺失的封面图，避免阻塞UI线程**
3. 使用Windows原生API播放视频
4. 应用程序启动时设置高优先级
5. 延迟加载图片以减少内存使用
6. **动态调整布局，提升不同屏幕尺寸下的显示效果**



## 许可

MIT许可证 