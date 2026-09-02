# 战线突围 / Battlefront Breakout

《战线突围》是一款使用 C++17 与 raylib 制作的 8×8 回合制战术 Roguelite。
玩家在十个渐进关卡中观察敌人意图，在“移动”与“线性攻击”之间作出选择，
并在关卡间构筑自己的天赋路线。

## 主要特色

- 独立图形窗口与 1600×900 自适应霓虹 HUD，不再依赖控制台黑框。
- 9 类敌人、8 种 AI 行为、10 个关卡与可预测的敌方意图线。
- 鼠标和键盘双操作，包含教程、图鉴、暂停、重开与完整胜负结算。
- 三选一 Roguelite 天赋、关卡最佳时间和本地设置持久化。
- 射击、命中、治疗与胜负程序化音效，无额外音频版权依赖。
- 规则核心与 Raylib 表现层分离，可脱离图形环境运行自动测试。

## Windows 构建与运行

需要 Windows 10/11 x64、PowerShell 和 MinGW GCC 13 或更高版本。
脚本会优先使用系统 CMake；若未安装，会把经过 SHA-256 校验的 CMake 4.4.2
便携版下载到被 Git 忽略的 `.tools` 目录。Raylib 6.0 由 CMake FetchContent
固定版本下载并编译。

```powershell
# 构建并运行自动测试
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test

# 构建、测试并启动游戏
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1

# 生成确定性的视觉验收截图
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -QaScreenshot
```

构建结果位于 `build/bin/BattlefrontBreakout.exe`。运行时资源会自动复制到
同一目录下。首次构建需要访问 GitHub 下载 CMake（如缺失）和 Raylib。

## 操作

| 操作 | 鼠标 | 键盘 |
| --- | --- | --- |
| 选择格子 | 点击棋盘 | WASD / 方向键 |
| 移动模式 | 点击“移动” | `1` 或 `M` |
| 攻击模式 | 点击“线性攻击” | `2` 或 `F` |
| 确认 | 点击目标格 | `Enter` 或 `Space` |
| 暂停 | — | `Esc` |
| 重新开始本关 | — | `R` |
| 教程 | — | `F1` |

每个玩家回合只能移动或攻击一次。红色意图线表示敌人准备攻击，橙色意图线
表示敌人计划移动的位置，绿色意图线表示治疗行为。

## 架构

- `src/core`：无窗口、无控制台 I/O 的确定性规则、AI、内容和存档服务。
- `src/app`：Raylib 场景、输入、HUD、动画、粒子与程序化音频。
- `tests`：不链接 Raylib 的规则和存档测试。

用户设置与最佳时间保存在 `%LOCALAPPDATA%\BattlefrontBreakout\profile.json`。
第三方许可证见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
