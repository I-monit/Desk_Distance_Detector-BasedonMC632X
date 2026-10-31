# MC632X 办公人员桌面测距仪 — PC 上位机（Qt）

Qt6 桌面应用，作为 MC632X 板端的 PC 侧监控界面：RTSP 实时画面 + 距离曲线/热力图/分布统计 + 标定控制与过近提醒，替代板端 LVGL 推流画面与物理按键。

## 环境要求

| 依赖 | 版本要求 | 说明 |
|---|---|---|
| Qt | **6.12+（MinGW 64-bit）** | 必须 MinGW 版且自带 FFmpeg 9.x（见下） |
| MinGW | 13.x 64-bit | 与 Qt 配套的编译工具链 |
| CMake | ≥ 3.21 | 可用 Qt 安装器自带版本 |
| Ninja | 任意近期版本 | 构建后端 |
| Python | 3.x | 可选，仅运行 `tests/mock_server.py` 模拟板端时需要 |

**Qt 在线安装器需勾选：**

- `Qt 6.12.x` → `MinGW 64-bit`
- `Developer and Designer Tools` → `MinGW 13.1.0 64-bit`、`CMake`、`Ninja`

**为什么限定 Qt 6.12+：** 视频解码直接链接 Qt 自带的 FFmpeg 9.0.1 DLL（`avutil-61 / avcodec-63 / avformat-63 / swscale-10`），仓库内 `third_party/ffmpeg/include` 的头文件与该版本 ABI 对齐。旧版 Qt（自带 FFmpeg 8 及以下）链接会失败。

## 首次配置

`build.bat` / `run.bat` 顶部写死了开发机路径，首次使用请将其改为本机安装路径：

```bat
set QT_ROOT=D:\Apps\Qt_GUI\QT_App\6.12.0\mingw_64   &rem 改成你的 Qt\mingw_64
set MINGW=D:\Apps\Qt_GUI\QT_App\Tools\mingw1310_64\bin
set CMAKE=D:\Apps\Qt_GUI\QT_App\Tools\CMake_64\bin
set NINJA=D:\Apps\Qt_GUI\QT_App\Tools\Ninja
```

改完后双击 `build.bat` 即完成 配置 → 编译 → windeployqt 部署。

也可手动构建（无需改脚本）：

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_PREFIX_PATH="<你的Qt路径>\mingw_64" `
    -DCMAKE_BUILD_TYPE=Release
cmake --build build
<你的Qt路径>\mingw_64\bin\windeployqt.exe --release --no-translations --no-system-d3d-compiler --no-opengl-sw build\qt_monitor.exe
```

## 运行

```powershell
# 1. 无板时: 启动模拟板端 (可选)
python tests/mock_server.py

# 2. 启动应用 (或直接双击 run.bat)
build\qt_monitor.exe
```

界面填写板端 IP（默认 `192.168.1.16`，mock 用 `127.0.0.1`）→ 点击"连接"。

## 协议契约

PC 与板端通过 **TCP 6000** 通信，字段与帧级配对规则见 [`resources/PROTOCOL.md`](resources/PROTOCOL.md)（与板端 `tcp_ctrl` 共同维护，任何字段变更需同步更新）。

## 目录结构

```
QT_GUI/
├── CMakeLists.txt
├── build.bat / run.bat        # 一键构建 / 增量编译并启动
├── src/
│   ├── main.cpp               # 入口, 日志重定向
│   ├── mainwindow.{h,cpp,ui}  # 主窗口装配, 帧级门控上屏
│   ├── algstate.h             # 协议数据结构 (与 PROTOCOL.md 对齐)
│   ├── boardlink.{h,cpp}      # TCP 数据通道客户端
│   ├── rtspstreamer.{h,cpp}   # FFmpeg 自主 RTSP 拉流解码
│   ├── videopanel.{h,cpp}     # 视频显示组件
│   ├── distanceplot.{h,cpp}   # 距离曲线
│   ├── heatmap.{h,cpp}        # 距离分布热力图与数据表
│   └── logpanel.{h,cpp}       # 可折叠日志面板
├── resources/
│   ├── PROTOCOL.md            # 板端-PC 端协议契约
│   └── icons/ + resources.qrc + qt_monitor.rc
├── third_party/ffmpeg/include # FFmpeg 9.0.1 官方头文件 (vendored, 与 Qt 自带 DLL ABI 对齐)
└── tests/
    └── mock_server.py         # 模拟板端 (无板开发调试)
```
