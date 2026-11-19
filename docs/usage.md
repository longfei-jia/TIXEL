# 在其他 ESP-IDF 工程中使用 TIXEL C++

本文是从零接入的完整步骤：引入组件、实现 port、接面板与按键、跑起来。
框架本体（`components/tixel`）不含任何平台代码，全部硬件差异集中在
`tixel::Port` 的 3~8 个函数指针里。

---

## 0. 前提

| 项目 | 要求 |
| --- | --- |
| ESP-IDF | v5.x / v6.x（组件按 C++17 编写，IDF v6 默认 `gnu++26` 亦可） |
| 面板 | 128×128 单色（1 bit），或能接收页格式缓冲的自定义面板 |
| 内存 | `tixel::Ui` 约 7 KB（含 3 个 2048 B 帧缓冲），建议静态分配 |
| 依赖 | 仅 libm；不需要 u8g2、不需要 LVGL |

## 1. 把组件引入工程

三种方式任选其一。

### 方式 A：放进工程的 components/ 目录（最简单）

```bash
your_project/
├── CMakeLists.txt
├── components/
│   ├── tixel/          # 从本仓库复制
│   └── tixel_demo/     # 可选：示例页面
└── main/
```

ESP-IDF 会自动发现 `components/` 下的组件，无需改 CMake。

### 方式 B：外部目录（推荐，组件单独维护）

```cmake
# your_project/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "E:/path/to/TIXEL/components")   # 指向本仓库 components

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_ui)
```

### 方式 C：git submodule

```bash
git submodule add <本仓库地址> components/TIXEL
# CMakeLists.txt 中：
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/components/TIXEL/components")
```

### 在 main 里声明依赖

```cmake
# your_project/main/CMakeLists.txt
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES tixel            # 只用库
    # REQUIRES tixel tixel_demo  # 需要示例页面时
)
```

> IDS v6 若在工程根用了 `set(COMPONENTS ...)` 裁剪组件集，记得把
> `tixel` / `tixel_demo` 和 port 里用到的组件（`esp_timer`、`esp_driver_gpio` 等）
> 一并列出。

## 2. 最小可运行程序

```cpp
// main/main.cpp
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tixel/ui.hpp"
#include "tixel_demo/apps.hpp"      // 只用库时可删

static tixel::Ui sUi;

extern "C" void app_main() {
    tixel::Port port;
    port.millis   = [](void*) -> uint32_t { return (uint32_t)(esp_timer_get_time() / 1000); };
    port.readKeys = [](void*) -> uint8_t  { return 0; };        // 见第 4 节
    port.flush    = [](void*, const uint8_t* fb, uint16_t pages) { /* 见第 3 节 */ };

    sUi.init(port, demo::kDefaults, demo::kSettingCount);       // 自己写页面时传 nullptr, 0
    demo::start(sUi);                                            // 或 sUi.start(&myRootPage);

    while (true) {
        sUi.tick();                        // dt 由 port.millis 计算
        vTaskDelay(pdMS_TO_TICKS(10));     // 10 ms 节拍 ≈ 100 fps 上限
    }
}
```

自定义页面只需继承 `tixel::Page`：

```cpp
class HelloPage : public tixel::Page {
public:
    void enter() override { title = "Hello"; }
    void render(tixel::Canvas& g) override {
        g.setFont(tixel::theme::kUi);
        g.textCenter(tixel::kWidth / 2.f, 40, "HELLO");
        g.rbox(10, 60, 108, 24, 6);
    }
};
static HelloPage sHello;
```

## 3. 面板输出：实现 `flush`

`flush(ctx, fb, pages)` 收到的是 **SSD1306 页格式**：

```
一页 = 128 字节，对应屏幕 8 行；
字节 bit0 = 该页第 1 行，bit7 = 第 8 行；
index = page * 128 + x；
pages 的 bit p = 1 表示第 p 页有变化（16 页 → uint16_t）。
```

三条硬性约定：

1. **只发脏页**。静止时 `flush` 不会被调用；全屏变化时 16 页全脏。
2. **必须自己拷贝**。`fb` 下一帧就被覆盖，异步 DMA 要用自己的缓冲。
3. **极性自己处理**。框架恒定 `1 = 内容`；若面板 `1 = 白`，在这里整体取反。

### 情况 1：SSD1306 / SH1107 等原生页格式（I2C/SPI）

直接按页下发即可（SH1107 128×128 的列地址从 0 开始；SSD1306 只有 8 页，
128 高的屏一般是 SH1107/SSD1327，注意页数）：

```cpp
static void flush_pages(void*, const uint8_t* fb, uint16_t pages) {
    for (int p = 0; p < 16; p++) {
        if (!(pages & (1u << p))) continue;
        panel_write_page(p, fb + p * 128);   // 你的驱动：设置页地址 + 写 128 字节
    }
}
```

### 情况 2：Sharp Memory LCD（行格式，需要重排）

Sharp 的串行接口是**行格式 + LSB-first + 1=白**，与页格式不同。适配思路：

```cpp
static void flush_sharp(void*, const uint8_t* fb, uint16_t pages) {
    static uint8_t rows[128][16];
    // 1) 页格式 -> 行格式（128 行 × 16 字节，MSB = 最左像素）
    for (int y = 0; y < 128; y++) {
        const uint8_t* src = fb + (y / 8) * 128;
        for (int xb = 0; xb < 16; xb++) {
            uint8_t v = 0;
            for (int b = 0; b < 8; b++)
                v |= (uint8_t)(((src[xb * 8 + b] >> (y % 8)) & 1u) << (7 - b));
            rows[y][xb] = (uint8_t)~v;    // 面板 1=白，框架 1=内容 -> 取反
        }
    }
    // 2) 只发行脏的页对应的行（pages 第 p 页 = 行 p*8 .. p*8+7）
    panel_send_rows(rows, pages);          // 你的驱动：SCS + SPI 一次性发送
}
```

若已使用现成的 Sharp 传输层（如 `screen_present(fb, dirty_pages)` 这种签名），
直接把 `flush` 转发过去即可，签名一致：

```cpp
port.flush = [](void*, const uint8_t* fb, uint16_t pages) { screen_present(fb, pages); };
```

> Sharp Memory LCD 必须周期性翻转 VCOM（建议 20 ms 一次，独立任务），
> 否则直流累积会损坏像素。框架不涉及这一点，由面板驱动负责。

### 情况 3：没有 1-bit 屏，只有 RGB/灰度屏

在 `flush` 里把页格式展开成你屏的行缓冲，再做二值映射或抖动即可；
或者实现一个 `Canvas` 的替代输出后端（`Canvas::screen()` 就是原始位图）。

## 4. 按键：实现 `readKeys`

返回位掩码，**1 = 按下**，框架内部做 12 ms 消抖、长按与重复识别：

| 位 | 键 | 宏 |
| --- | --- | --- |
| 0 | 上一个 / 上 | `tixel::keyBit(tixel::kKeyUp)` |
| 1 | 下一个 / 下 | `tixel::keyBit(tixel::kKeyDown)` |
| 2 | 确认 | `tixel::keyBit(tixel::kKeyEnter)` |
| 3 | 返回（4 键布局才有） | `tixel::keyBit(tixel::Key::Back)` |

两种布局（`port.keyLayout`）：

| 布局 | 值 | 手势 |
| --- | --- | --- |
| 四键 | `KeyLayout::Four` | PREV/NEXT 按下即触发（长按加速重复）；OK/BACK 短按/长按 |
| 三键 | `KeyLayout::Three` | UP/DOWN 同上；ENTER 松开时按按住时长判定：<0.4 s = OK，0.4~1.2 s = 返回，≥1.2 s = 回主页（主页上再长按 = 息屏） |

按键驱动已有时，直接映射：

```cpp
port.readKeys = [](void*) -> uint8_t {
    uint8_t m = 0;
    if (button_is_pressed(KEY_UP))   m |= tixel::keyBit(tixel::kKeyUp);
    if (button_is_pressed(KEY_DOWN)) m |= tixel::keyBit(tixel::kKeyDown);
    if (button_is_pressed(KEY_OK))   m |= tixel::keyBit(tixel::kKeyEnter);
    return m;
};
port.keyLayout = tixel::KeyLayout::Three;
```

没有驱动时，`gpio_config()` 配成输入 + 上拉，`gpio_get_level()==0` 即按下。

## 5. 时间、RTC 与设置持久化

```cpp
// 单调时钟（必须）
port.millis = [](void*) -> uint32_t { return (uint32_t)(esp_timer_get_time() / 1000); };

// 真实时间（可选）：没有就返回 false，框架用内置时钟从 12:59 起走
port.rtcGet = [](void*, tixel::DateTime* dt) -> bool {
    if (!timeSynced()) return false;        // SNTP 同步前返回 false
    // 填 dt：year/month/day/wday(0=周一)/hour/min/sec/ms
    return true;
};

// 设置持久化（可选）：返回 true 表示成功读到；不实现就一直用默认值
port.settingsLoad = [](void*, int32_t* v, int n) -> bool {
    // nvs_get_blob("tixel", v, &len) == ESP_OK
    return false;
};
port.settingsSave = [](void*, const int32_t* v, int n) {
    // 变化后约 1 s 才调用一次（框架做了防抖），直接 nvs_set_blob + commit
};
```

## 6. 亮度、息屏与振动

```cpp
port.setPower    = [](void*, bool on)  { screen_set_power(on); };   // 息屏/唤醒
port.setContrast = [](void*, uint8_t)  { /* OLED 对比度 0..255；Sharp 无此寄存器 */ };
port.haptic     = [](void*, uint16_t ms) { /* 振动马达，可选 */ };
```

亮度设置项（`kSettingBrightness`）在框架内部还会参与**软件调光**：
输出前按亮度做有序抖动降亮，因此即使面板没有对比度寄存器也有效。

## 7. 任务模型与省电

- UI 必须跑在**任务**里，不要在 ISR 里 `tick()`；ISR 只适合 `ui.post(key, kind)` 投事件。
- 推荐 10 ms 节拍；一帧 128×128 的光栅化在 ESP32-S3 上约 2~4 ms。
- `ui.idle()` 为 true 表示连续 30 帧无像素变化且无转场，可据此降频或进入 light sleep。
- `ui.post(...)` 线程安全（单生产者单消费者环形队列）。

## 8. 常见问题

| 现象 | 原因与处理 |
| --- | --- |
| 找不到 `tixel/ui.hpp` | `EXTRA_COMPONENT_DIRS` 没指向 `components`，或 `main` 没写 `REQUIRES tixel` |
| 花屏 / 行错位 | 面板位序（Sharp 是 LSB-first）；页→行重排时 MSB 必须是最左像素 |
| 整屏反色 | 面板 `1 = 白`，在 `flush` 里取反 |
| 文字全空 | 字体角色被改成不含字母的 `_tn` 字体（只含数字）；换 `_tr` |
| 中文显示为乱码/空白 | 当前文本按字节绘制（与上游一致）；中文需带 unicode 索引的字体 + UTF-8 文本入口 |
| 编译报 `volatile` 错误 | GCC 15 对 `volatile` 链式赋值报错；本库已修，若自行改代码注意拆成两条语句 |
| 只有第一帧 | `flush` 没拷贝缓冲，或用完释放了 `fb` |
| 闪烁 / 撕裂 | `flush` 阻塞太久；改 DMA 或减少每帧发送页数 |

## 9. 从示例到产品

示例（`components/tixel_demo`）刻意保持最小：英文文案、无持久化、无网络。
落地时通常只需要：

1. 把 `demo::kDefaults` 换成自己的默认值，`S_*` 设置项按需扩展（从 `kSettingUser` 起）。
2. 用 `Menu` 表驱动替换设置页的表；页面数量不受限制（页面实例静态分配）。
3. 接入 NVS 的 `settingsLoad/Save`，重启后保留亮度、语言等。
4. 需要中文时替换 `theme::kUi/kTiny` 为带 unicode 索引的字体，并把文本入口改为 UTF-8。
