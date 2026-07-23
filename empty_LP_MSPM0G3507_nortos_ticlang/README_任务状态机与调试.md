# H题直线与转角代码说明

## 按钮和当前可运行范围

- PA18：第一问。A 点朝向 B 点放车，直行 100 cm，灰度板确认终点黑线或编码器达到目标后停车。
- PB22：第二问。从 A 到 B 的直线完成后停车在 `STATE_WAIT_ARC`，等待队友的右圆弧程序；圆弧报告完成后自动执行 C 到 D 的直线。
- PB23：第三问当前独立测试。把车放在 **B 点、车头沿 C 到 B 圆弧的出口切线方向**，按键后先前向差速转校准后的 40°，再直行约 128.06 cm 到 D。
- PB24：第四问完整状态机骨架。小车在 A 点沿水平切线摆放，先转校准后的 40° 再走 A 到 C；每个圆弧由队友程序完成并调用 `ArcInterface_markComplete()`，之后自动交替转角、走斜直线，累计 4 圈。

每条斜直线结束后还会反向转40°，把车头恢复到圆弧端点的水平切线方向，然后才向圆弧模块发出请求。因此队友接管时车头不是斜着切入圆弧。

圆弧尚未接入时，进入 `STATE_WAIT_ARC` 会保持制动，这是故意的安全行为，不是死机。

## SysConfig（当前文件已经满足，不要再新增外设）

- PA18：GPIO 输入、Pull Down；按下接 3.3 V。
- PB22/PB23/PB24：GPIO 输入、Pull Up；按下接 GND。
- PB8：灰度 CLK，GPIO 输出；PB9：灰度 DAT，GPIO 输入、Pull Up。
- PB25/PB21：左右编码器 A 相，上升沿 GPIO 中断；PA23/PB27：B 相普通输入。
- UART0：115200、8 data bits、no parity、1 stop bit、TX and RX、BUSCLK 32 MHz、RX `Receive` 中断；TX=PA10，RX=PA11；FIFO、DMA、硬件流控均关闭。
- TIMG12：Periodic Down Counting、1 ms、Start Timer；代码打开 ZERO EVENT 中断。
- TIMG0 CC1/PA13 和 TIMA1 CC0/PA15：20 kHz PWM，周期计数 1600。

当前代码依赖 SysConfig 中现有的组名 `START_KEY` 和 `MOTOR_TEST_KEYS` 生成的宏。可以在界面里把显示名称改得更漂亮，但改名后必须同步修改 `task_buttons.c` 中的 IOMUX 宏；暂时不改最稳妥。

## 接线

- 陀螺仪 VCC -> 5 V，GND -> GND，TX -> PA11/UART0 RX，RX -> PA10/UART0 TX。
- 灰度辅助板 5V -> 5 V，GND -> GND，CLK -> PB8，DAT -> PB9；串行方式不用连接 1-8、KEY、ERR。
- 所有模块、电机驱动板和 MCU 必须共地。

## 第三问角度来源

两个圆弧半径为 40 cm，因此上下端点相差 80 cm；左右端点水平相差 100 cm：

`斜线长度 = sqrt(100^2 + 80^2) = 128.06 cm`

`理论出口转角 = atan(80 / 100) = 38.66°`，当前实车校准值为40°。

B 到 D 与 A 到 C 的物理转向相反。转角控制启动后会根据最初 1° 的实际 Yaw 变化自动识别陀螺仪正负方向，不再依赖安装方向符号配置。

## 第一次实车调试

先架空驱动轮，在 CCS Expressions 观察：

- `gGyroValidFrameCount` 持续增加，`gGyroAgeMs < 500`；否则先核对波特率和 TX/RX 交叉接线。
- 手动转动车体并确认 `gGyroRawYaw` 会随转动变化。数值增加或减小都可以，代码会自动识别。
- 前推左右轮时 `gStraightLeftPulses`、`gStraightRightPulses` 都应增加。
- 白地时 `gStraightRawGray` 应接近 `0xFF`，压到黑线的探头位应变成 0。

机械参数、PI参数、终点确认时间、40°校准角和两种直线距离全部集中在 `app_config.h`。

## 队友接圆弧

队友读取 `ArcInterface_getRequest()`，按枚举执行对应半圆；到达下一个端点并完成制动后调用一次 `ArcInterface_markComplete()`。不要修改 `mission.c` 的直线和转角控制即可完成合并。
