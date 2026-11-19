# examples

`apps/` 是 TIXEL 的示例页面集合（ESP-IDF 组件名 `apps`），用来演示框架能做什么。
七个页面与参考实现的示例一一对应，全部不含平台代码 —— 时间、按键、显示都由
`tixel::Port` 注入。

文案支持中英双语：进入 **设置 → 语言** 切换，立即生效。字符串写成
`"English|中文"`，由 `Menu` 和 `apps::tr()` 按 `kSettingLanguage` 取用。

| 文件 | 页面 | 演示的能力 |
| --- | --- | --- |
| `app_boot.cpp` | Boot | CRT 闪白、字母沿贝塞尔抛入、下划线展开、打字机副标题、进度条、溶解转场 |
| `app_home.cpp` | Home | 图标轮播、橡皮筋边界回弹、缩放与淡出过渡、页点指示 |
| `app_clock.cpp` | Clock | 大号数字、秒表子视图、边框刻度尺、页面内状态切换 |
| `app_music.cpp` | Music | 唱片旋转、频谱柱、进度条、播放/暂停图标 morph |
| `app_monitor.cpp` | Monitor | 标签页、折线图、暂停徽标、数值滚动 |
| `app_settings.cpp` | Settings | 数据驱动菜单、开关/选项/子菜单/动作、底部滑条面板、确认框、toast |
| `app_about.cpp` | About | 可滚动列表、滚动条、字标与版本行 |

## 怎么用

工程根 `CMakeLists.txt` 把这个目录加进 `EXTRA_COMPONENT_DIRS`：

```cmake
set(EXTRA_COMPONENT_DIRS
    "path/to/TIXEL/components"    # 库
    "path/to/TIXEL/examples")     # 本目录（组件 apps）
```

`main` 里声明依赖，然后启动：

```cmake
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "." REQUIRES tixel apps)
```

```cpp
#include "apps/apps.hpp"

ui.init(port, tixel::apps::kDefaults, tixel::apps::kSettingCount);
tixel::apps::start(ui);          // boot splash -> home
```

这些页面只是起点：把 `tixel::apps::kDefaults` 换成自己的默认值、用 `Menu` 表驱动
替换设置页，就变成你自己的界面。完整接入步骤见 [../docs/usage.md](../docs/usage.md)。
