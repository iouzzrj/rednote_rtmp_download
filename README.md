# rednote_rtmp_download

使用 MSVC + vcpkg 的示例项目，用于根据配置文件请求小红书直播状态并在检测到直播时调用 PowerShell 运行 `rtmpdump` 下载直播流。

## 构建

```powershell
# 假设已安装 vcpkg 并配置为全局集成，或使用 CMakePresets 配置 toolchain
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

## 配置

请编辑根目录的 `config.json`，其中已经预填了抓包得到的请求头和默认的 rtmpdump 路径。

- `request.headers` 中 `required: true` 的条目表示在抓包验证时必需的请求头。
- `rtmp.auto_download` 设置为 `false` 时，程序只会输出 PowerShell 命令，不会自动执行。

## 运行

```powershell
./build/Release/rednote_rtmp_download.exe
```

程序会输出 HTTP 响应内容。当检测到直播间时，会根据配置生成下载文件夹（例如 `downloads/20251017/569971301260745429_rtmp.flv`），并调用 PowerShell 执行 `rtmpdump`。如果文件已存在，会自动附加序号避免覆盖。
