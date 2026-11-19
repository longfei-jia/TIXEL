# TIXEL C++ · 128×128 单色 UI 框架（ESP-IDF 组件）

以参考实现（C99，128×64）为基准重写为 C++17，面向 128×128 单色面板：
页面、控件、动效、示例页面全部保留，去掉 STM32 平台代码与 u8g2 依赖，
按 ESP-IDF 组件组织，可直接被其他工程引用。

---

## 1. 结论

| 项目 | 结果 |
| --- | --- |
| 组件 | `components/tixel`（库）+ `examples/apps`（示例页面，组件名 `apps`） |
| 面板 | 128×128，页格式帧缓冲，16 页 / 2048 B，脏页掩码 16 bit |
| 依赖 | 仅 libm；不依赖 u8g2（内置 u8g2 字体解码器，字体数据按需抽取） |
| 分配 | 零动态分配，全部静态存储；`tixel::Ui` 约 7 KB（含 3 个帧缓冲） |
| 体积 | `libtixel.a` 51.9 KB（53,130 B，含 16 KB 字体数据）、`libapps.a` 22.3 KB（+2.9 KB data / 3.7 KB bss）；示例固件 215 KB（220,192 B） |
| 一致性 | 与上游**逐字节**一致：`tests/parity` 329 项 + `tests/menu_parity` 15 帧，0 失败 |
| 版式 | 纵向 2×、字号取屏宽可容纳的最大值；逐页映射见 [docs/layout.md](docs/layout.md) |

## 2. 目录

```
components/tixel/          库：canvas / font / motion / input / settings / ui / overlay / widgets / menu / icons
examples/apps/             示例页面：boot、home、clock、music、monitor、settings、about（组件名 apps）
tests/                     与参考实现的逐字节对比测试（需要 res/LUMEN 源码）
tools/                     字体抽取、宿主抓帧预览、PGM→PNG
docs/                      接入指南（usage.md）、版式映射表（layout.md）、预览截图
res/LUMEN/                 参考实现源码（.gitignore 忽略；仅测试需要，自行 clone）
```

## 3. 在 IDF 工程中使用

> 完整步骤（含面板适配、按键映射、NVS 持久化、常见问题）见 **[docs/usage.md](docs/usage.md)**。
> 下面是三步速览。

1）让工程能找到组件目录：

```cmake
# 工程根 CMakeLists.txt
set(TIXEL_DIR "${CMAKE_CURRENT_LIST_DIR}/path/to/TIXEL")
set(EXTRA_COMPONENT_DIRS
    ${TIXEL_DIR}/components      # 库：tixel
    ${TIXEL_DIR}/examples)       # 示例页面：apps（不需要时去掉这行）
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_ui)
```

2）在 `main/CMakeLists.txt` 中引用：

```cmake
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "." REQUIRES tixel)
# 需要示例页面时再加 apps
```

3）实现 `tixel::Port`，把硬件接进来（自由函数指针，无 std::function）：

```cpp
static uint32_t ms(void*) { return esp_timer_get_time() / 1000; }
static uint8_t  keys(void*) { /* 位=1 表示按下 */ return mask; }
static void     flush(void*, const uint8_t* fb, uint16_t pages) { panel_send(fb, pages); }

tixel::Port port;
port.millis   = ms;
port.readKeys = keys;
port.flush    = flush;
port.keyLayout = tixel::KeyLayout::Three;   // 三键（UP/DOWN/ENTER），默认四键
// 可选：haptic / rtcGet / settingsLoad / settingsSave / setContrast / setPower
```

4）启动与主循环：

```cpp
static tixel::Ui ui;
ui.init(port, tixel::apps::kDefaults, tixel::apps::kSettingCount);
tixel::apps::start(ui);                 // 或 ui.start(myRootPage);

while (true) {
    ui.tick();                   // dt 由 port.millis 计算
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### 面板接入要求（摘要）

`flush` 收到的是 **SSD1306 页格式**：每字节 8 个纵向像素（LSB 在上），
`index = page * 128 + x`，`bit p = page p 有变化`。三点必须遵守：

1. **只发脏页**；静止时 `flush` 不会被调用（内部做整页哈希）。
2. `flush` 可以异步，但**必须自己拷贝**：缓冲下一帧就会被覆盖。
3. 面板若按行格式（如 Sharp Memory LCD），在 `flush` 里做页→行重排；
   若 `bit=1` 在你的面板上是白色，需要在 `flush` 里整体取反。

## 4. 128×128 版式规则

上游以 128×64 为基准。本库的映射规则：

- **纵向**（行高、间距、圆角、控件尺寸）：`scaled(v) = v × (H / 64)`，128 高即 2×；
  代码统一用 `tixel::scaled()`，把面板高度改回 64 时布局自动回到上游数值。
- **字号**：按屏宽能容纳的最大字号选取（不是简单 2×），
  例如正文 `7x13`、状态栏 `6x12`、数值 `logisoso28`、时钟 `logisoso42`、字标 `logisoso34`。
- **横向**：边距与留白随 2× 放大，但内容宽度受 128 px 限制，
  密集元素（频谱柱、图表点、秒标尺、对话框/底部面板）按可用宽度重排。

逐页数值对照（上游 → 本库、变更原因）见 [docs/layout.md](docs/layout.md)。

## 5. 构建验证（ESP-IDF v6.1 / esp32s3）

```
$ idf.py build
Successfully created ESP32-S3 image.
my_ui.bin binary size 0x35c20 bytes. Smallest app partition is 0x100000 bytes. 0xca3e0 bytes (79%) free.
```

组件以 `-Wall -Werror`、`-std=gnu++26`、`-fno-exceptions -fno-rtti` 编译通过，
无警告。只引用本框架的工程可用 `set(COMPONENTS main tixel apps)` 裁剪组件集
（`apps` 仅在需要示例页面时加入），不构建 mbedTLS/Wi-Fi 等无关部分。

## 6. 一致性验证

`tests/` 把库**按参考几何（128×64 + 参考字体）**编译，与 `res/LUMEN` 的 C 实现逐字节比较：

```bash
# 参考实现不随仓库分发，先准备源码
git clone https://github.com/Xulijun180/LUMEN res/LUMEN

cmake -S tests -B build/tests -G Ninja        # 或 -DREF_DIR=/path/to/checkout
cmake --build build/tests
./build/tests/parity.exe        # 329 checks, 0 failures
./build/tests/menu_parity.exe   # 15 checks, 0 failures
```

覆盖范围：

| 测试 | 内容 |
| --- | --- |
| `parity.cpp` | 5 种字体角色的全部字符 × 7 个 y 位置 / 边界 x；`hline/vline/box/frame/rbox/rframe/disc/ring/arc/line/capsule/poly/dotted`；裁剪与平移栈；SET/CLEAR/XOR；5 档抖动 α；`bufFade/bufBlit/bufBlitRegion`；弹簧收敛与补间中点 |
| `menu_parity.cpp` | 菜单页 15 帧逐帧一致：进入动画、逐行下移、边界回卷、开关、选项循环、空闲 |

> 一致性是**按上游几何**证明的。128×128 下改的是版式常量，不是渲染语义，
> 所以像素级行为仍由同一套栅格化/运动代码决定。

## 7. 宿主预览

无需硬件即可看版式：

```bash
cmake -S tools/preview -B build/preview -G Ninja && cmake --build build/preview
./build/preview/preview.exe build/preview/out      # 26 帧 PGM
python tools/pgm2png.py build/preview/out build/preview/png 2
```

截图见 `docs/preview/`。预览工具用虚拟时钟与虚拟按键跑完整导航脚本，
并校验每一步停留在预期页面。

## 8. API 速查

| 类型 | 作用 |
| --- | --- |
| `tixel::Ui` | 页面栈、转场、电源/亮度/反色、事件分发、整帧输出 |
| `tixel::Port` | 硬件注入：`millis / readKeys / haptic / rtcGet / settingsLoad / settingsSave / flush / setContrast / setPower / keyLayout` |
| `tixel::Canvas` | 帧缓冲 + 浮点图元 + 裁剪/平移 + SET/CLEAR/XOR + 抖动 α + 文本 |
| `tixel::Font` | u8g2 格式字体解码（字宽、行高、绘制） |
| `tixel::Page` | 页面基类：`enter/resume/leave/key/update/render/focus/probe` |
| `tixel::Overlay` | 覆盖层基类；内置 `toast / dialog / sheet` |
| `tixel::Menu` | 数据驱动菜单：`TIXEL_ITEM_TOGGLE/OPTION/SHEET/SUBMENU/ACTION` |
| `tixel::Spring` / `Tween` | 二阶系统 / 贝塞尔补间；`kMotions[]` 为统一动效 token |
| `tixel::Keys` | 4 键 / 3 键手势状态机（3 键：ENTER 短按=OK、0.4s=返回、1.2s=回主页） |
| `tixel::theme::kUi / kTiny / kMid / kBig / kLogo` | 字体角色 |
| `tixel::scaled(v)` | 参考像素 → 当前面板像素 |

## 9. 已知限制

- **设置不持久化**：示例 port 的 `settingsLoad` 返回 `false`，接 NVS/Flash 即可。
- **文本按字节绘制**：与上游 `u8g2_DrawStr` 行为一致；中文需改用 UTF-8 入口并换带
  unicode 索引的字体（`Font` 已实现 unicode 查找，文本迭代目前是字节语义）。
- **面板尺寸为编译期常量**：`TIXEL_PANEL_WIDTH/HEIGHT` 可覆盖，但更换高度后需按
  `docs/layout.md` 复核版式；当前只在 128×128（默认）与 128×64（测试）上验证。
- **示例页面为英文文案**：与上游一致，未做多语言。
- **上游 `examples/`（29 MB STM32 工程）未纳入仓库**，只在本地作为参考。

## 10. 许可

- 本仓库的 C++ 实现：跟随参考实现的 AGPL-3.0（见 `res/LUMEN/LICENSE`）。
- 字体数据来自 u8g2（BSD-2-Clause，版权 Olikraus），仅抽取本库使用的 10 个字体数组，
  原样保留字节内容，见 `components/tixel/src/fonts_data.cpp` 与 `tools/extract_fonts.py`。
