# 2026 年电赛 H 题循迹小车 - 第一阶段引脚配置

本工程已删除原有业务代码、旧 SysConfig 备份和旧 Debug 构建产物，只保留 CCS 工程骨架，并从零创建新的引脚配置和模块化控制代码。

MCU：MSPM0G3507，LQFP-64。  
SDK：MSPM0 SDK 2.11.00.07。  
LCD：严格按本次给出的接线配置成 GPIO 软件 SPI。  
电机：两路 PWM 使用同一个 TIMA0，上电占空比 0%，避免上电误转。  
当前按键功能：PA18循迹一圈，忽略起步后第一次启停横线，第二次检测到启停线后立即停止计时并停车；PB22执行A→B，进入B端圆弧后继续行驶一个350mm车长，整车通过B点时停止计时，再沿圆弧行驶150mm后减速停车；PB23循迹一圈，整车通过A点时停止计时，再沿直线行驶150mm后减速停车；PB24在任何时刻立即急停。LCD显示陀螺仪Z轴角速度、任务计时、状态、距离和双轮平均实时速度。

## 当前源码模块

| 文件 | 作用 |
|---|---|
| `motor.c/.h` | TB6612 双路 PWM、正反转和 STBY 控制，上电安全停机 |
| `encoder.c/.h` | 两组 A/B 相四倍频解码、平均里程和毫米换算 |
| `gray_sensor.c/.h` | 感为辅助板CLK/DAT采样、黑线位置和有效性判断 |
| `buttons.c/.h` | 四个板载按钮的 20ms 消抖 |
| `straight_control.c/.h` | 灰度循迹PID、启动渐入、弯道纠偏和编码器堵转监测 |
| `ab_mission.c/.h` | 三种按键任务、起始横线基准、整车通过计时、延后停车和故障保护 |
| `app_time.c/.h` | 任务计时；启动时清零，停车时冻结 |
| `gyro.c/.h` | UART0 接收、5字节帧校验和角速度/角度换算 |
| `lcd.c/.h` | ILI9341 GPIO软件SPI、图形和5×7字符驱动 |
| `display.c/.h` | 角速度、计时、任务状态和距离界面 |
| `app_config.h` | 机械参数、目标计数、PWM和方向可调参数 |
| `empty.c` | 1ms调度、GPIO中断和程序入口 |

## 完整引脚表

| 功能模块 | 外设端子/信号 | MSPM0G3507 引脚 | LQFP-64 管脚号 | SysConfig 模式 | 上电/中断设置 |
|---|---|---:|---:|---|---|
| LCD | BLK | PA16 | 9 | GPIO 输出 | 初始低，背光关闭 |
| LCD | D/C | PB15 | 3 | GPIO 输出 | 初始低 |
| LCD | SDO | PB20 | 19 | GPIO 输入 | 下拉；不用读屏时可不接 |
| LCD | CS | PB17 | 14 | GPIO 输出 | 初始高，片选无效 |
| LCD | CLK | PA17 | 10 | GPIO 输出 | 初始低 |
| LCD | SDI | PB16 | 4 | GPIO 输出 | 初始低 |
| 按钮 | 一圈，到线立即停车 | PA18 | 11 | GPIO 输入 | 下拉、按下高、上升沿中断 |
| 按钮 | A→B，整车通过后计时停止 | PB22 | 21 | GPIO 输入 | 上拉、按下低、下降沿中断 |
| 按钮 | 一圈，整车通过后计时停止 | PB23 | 22 | GPIO 输入 | 上拉、按下低、下降沿中断 |
| 按钮 | 任何时刻立即停车 | PB24 | 23 | GPIO 输入 | 上拉、按下低、下降沿中断 |
| TB6612 | PWMA | PB8 | 60 | TIMA0_C0 PWM | 约20kHz；待机时由ODIS强制低 |
| TB6612 | PWMB | PB9 | 61 | TIMA0_C1 PWM | 约20kHz；待机时由ODIS强制低 |
| TB6612 | AIN1 | PB10 | 62 | GPIO 输出 | 初始低，A路方向控制 |
| TB6612 | AIN2 | PB11 | 63 | GPIO 输出 | 初始低，A路方向控制 |
| TB6612 | BIN1 | PB12 | 64 | GPIO 输出 | 初始低，B路方向控制 |
| TB6612 | BIN2 | PB13 | 1 | GPIO 输出 | 初始低，B路方向控制 |
| TB6612 | STBY | PB14 | 2 | GPIO 输出 | 初始低；运行时才拉高 |
| 编码器 1 | E1A | PA21 | 17 | GPIO 输入 | 双边沿中断 |
| 编码器 1 | E1B | PA22 | 18 | GPIO 输入 | 双边沿中断 |
| 编码器 2 | E2A | PA23 | 24 | GPIO 输入 | 双边沿中断 |
| 编码器 2 | E2B | PA24 | 25 | GPIO 输入 | 双边沿中断 |
| 灰度辅助板 | CLK | PB2 | 50 | GPIO 输出 | 初始低 |
| 灰度辅助板 | DAT | PB3 | 51 | GPIO 输入 | 下拉 |
| 单轴陀螺仪 | MCU TX → 模块 RX | PA10 | 56 | UART0_TX | 115200, 8-N-1 |
| 单轴陀螺仪 | MCU RX ← 模块 TX | PA11 | 57 | UART0_RX | 115200, RX 中断源 |
| STM32 预留 | MSP TX → STM32 RX | PB4 | 52 | UART1_TX | 115200, 8-N-1，P6 接口 |
| STM32 预留 | MSP RX ← STM32 TX | PA9 | 55 | UART1_RX | 115200, RX 中断源，P6 接口 |
| 调试 | SWDIO | PA19 | 12 | DEBUGSS | 固定保留 |
| 调试 | SWCLK | PA20 | 13 | DEBUGSS | 固定保留 |

## 重要接线说明

1. 所有外设必须与 MSPM0 开发板共地。
2. LCD 逻辑电源和 IO 使用 2.8V~3.3V，不要给 LCD IO 输入 5V。
3. 灰度主板按资料使用独立、稳定的 5V 供电；辅助板的 CLK/DAT 是 3.3V 逻辑。接线为辅助板`CLK→PB2`、`DAT→PB3`并共地。
4. 编码器 E1A/E1B/E2A/E2B 必须确认输出不超过 3.3V。若是 5V 推挽输出，先加电平转换或分压。
5. 陀螺仪模块 TX 接 PA11，RX 接 PA10；串口必须交叉接。
6. STM32 TX 接 PA9，STM32 RX 接 PB4；两块主控共地，IO 都使用 3.3V。
7. PA19/PA20 是下载调试口，严禁再接按钮、电机或传感器。
8. 开发板上 PA3/PA4 接 32.768kHz 晶振、PA5/PA6 接 40MHz 晶振、PA2 接 ROSC 电阻，均不拿来做业务 IO。
9. TB6612 的 `VCC` 接 MSPM0 的 `3.3V`，`VM` 接电机电源正极，两个 `GND` 必须和 MSPM0、电机电源共地。
10. 左电机接 `AO1/AO2`，右电机接 `BO1/BO2`；编码器信号仍接 PA21～PA24，不能接到电机输出端。

## LCD 接口特别提醒

当前 `empty.syscfg` 完全按照本次给出的接线：

`BLK=PA16, D/C=PB15, SDO=PB20, CS=PB17, CLK=PA17, SDI=PB16`

这和资料中开发板自带 P1 TFT 插座的网络顺序不同。因此：

- 如果 LCD 是用杜邦线单独接到这些引脚，可以直接使用本配置。
- 如果 LCD 是直接插在开发板 P1 TFT 插座上，不能照本配置上电；需要改回原理图的 P1 网络定义，并补上 LCD RST。
- 当前六根信号线没有单独分配 `RST`。代码上电时发送 ILI9341 软件复位命令，因此液晶模块的 `RST` 必须已在模块上拉高；若屏幕始终白屏，先检查这一点。

## LCD显示与陀螺仪协议

界面每100ms取得一次新数据，并把变化的字符分散刷新，避免GPIO软件SPI整屏刷新长时间阻塞电机控制。

| 显示项 | 含义 |
|---|---|
| `GYRO WZ` | Z轴角速度，单位 `DPS`（度/秒）；超过500ms没有有效角速度帧时显示 `NO DATA` |
| `TIME` | 从任务启动到规定计时终点；PB22/PB23冻结计时后车辆仍会继续行驶150mm |
| `STATE` | 包含`AB RUN / LAP NOW / LAP PASS / PASSING / POST RUN / DECEL / DONE / ABORT / FAULT` |
| `DIST` | 双轮编码器平均里程，单位mm |
| `SPEED` | 双轮编码器每10ms增量计算并低通滤波后的平均速度，单位mm/s |

当前按资料解析5字节帧：

```text
5A  AA  WzL  WzH  SUM
```

`SUM`为前4字节相加的低8位，`WzH:WzL`按有符号16位解析，角速度换算为 `raw / 32768 × 2000 dps`。UART0当前设置为115200、8-N-1。若接线正确但一直显示`NO DATA`，用串口工具确认模块实际波特率；部分资料的出厂参数页写成9600，确认后再在`empty.syscfg`中修改UART0，而不要直接编辑`Debug/ti_msp_dl_config.*`。

## 在 CCS 中检查

1. 在 Project Explorer 中双击 `empty.syscfg`。
2. 等待 SysConfig 图形界面加载完成。
3. 点左侧 `PinMux`，确认右侧没有红色冲突。
4. 展开 `GPIO`，检查 `LCD_GPIO`、`KEY_GPIO`、`MOTOR_CTRL_GPIO`、`ENCODER_GPIO`、`GRAY_GPIO`。
5. 展开 `PWM`，确认 `MOTOR_PWM` 为 `TIMA0`，输出为 `PB8/PB9`，两个通道 Duty Cycle 都是 `0`。
6. 展开两个 `UART`，确认陀螺仪为 `UART0 PA10/PA11`，STM32 为 `UART1 PB4/PA9`。
7. 按 `Ctrl+S` 保存。
8. 右击工程，点 `Clean Project`，再点锤子图标 `Build Project`。

三种任务的首次上车步骤见`直线测试说明.md`。起步后第一次启停横线会被记录为一圈距离基准，不会触发完成；第二次横线相对第一次横线判断，因此探头放在启停线后方的距离不会混入一圈长度。横线允许连续3～8个探头见黑，但完成判定要求此前200ms内见过正常1～2灯纵向轨迹，可排除持续全黑故障。PA18/PB22/PB23都由灰度PID负责方向，编码器用于里程、速度、350mm车长补偿和堵转检测。
