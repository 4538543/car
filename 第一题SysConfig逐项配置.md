# 第一题 SysConfig 逐项配置（辅助板串行输出 + 双编码器后轮驱动）

本文档对应 MSPM0G3507、TB6612 双路电机驱动、后轮双驱差速、八路灰度辅助板 `CLK/DAT` 串行输出。

> 重要：SysConfig 中引脚内部的 `Name` 只是软件名称。比如把名字写成 `PIN_4`，并不代表选择了 PB4。必须在页面底部的 `Assigned Pin` 或 `PinMux` 下拉框中明确选中 `PB4`。如果底部仍显示 `Any(PB13/1)`，实际分配的是 PB13，而不是 PB4。

## 一、最终引脚表

| 功能 | 模块端引脚 | MCU 引脚 | SysConfig 类型 | 初始状态 |
|---|---|---|---|---|
| 左电机方向 1 | TB6612 AIN1 | PB4 | GPIO 输出 | Cleared/低 |
| 左电机方向 2 | TB6612 AIN2 | PB5 | GPIO 输出 | Cleared/低 |
| 右电机方向 1 | TB6612 BIN1 | PB6 | GPIO 输出 | Cleared/低 |
| 右电机方向 2 | TB6612 BIN2 | PB7 | GPIO 输出 | Cleared/低 |
| 驱动使能 | TB6612 STBY | PB12 | GPIO 输出 | Cleared/低 |
| 左电机 PWM | TB6612 PWMA | PA13/TIMG0_C1 | TIMER-PWM | 20 kHz、0% |
| 右电机 PWM | TB6612 PWMB | PA15/TIMA1_C0 | TIMER-PWM | 20 kHz、0% |
| 灰度时钟 | 辅助板 CLK | PB8 | GPIO 输出 | Cleared/低 |
| 灰度数据 | 辅助板 DAT | PB9 | GPIO 输入 | Pull-Up |
| 左编码器 A 相 | E1A | PB17 | GPIO 输入、中断 | Rising Edge |
| 左编码器 B 相 | E1B | PB20 | GPIO 输入 | 无中断 |
| 右编码器 A 相 | E2A | PB21 | GPIO 输入、中断 | Rising Edge |
| 右编码器 B 相 | E2B | PB27 | GPIO 输入 | 无中断 |
| 启动/急停键 | 板载 KEY1 | PA18 | GPIO 输入 | Pull-Up、轮询 |
| 蓝灯 | 板载蓝 LED | PA7 | GPIO 输出 | Set/高，默认熄灭 |
| 蜂鸣器 | 板载蜂鸣器 | PB26 | GPIO 输出 | Cleared/低 |

辅助板的 `KEY、ERR、1~8` 第一题串行模式均不接 MCU，只接 `5V、GND、CLK、DAT`。辅助板 `KEY` 不是车辆启动键；车辆启动键是板载 `KEY1/PA18`。

## 二、截图页面中每个公共选项怎样填写

所有 GPIO 实例都在左侧 `GPIO` 模块中添加。一个实例只能统一选择一个 `Port` 和 `Port Segment`，因此按下文分成六组。

普通 GPIO 输出统一填写：

- `Direction`：Output
- `Initial Value`：按表选择 Cleared 或 Set
- `IO Structure`：Any（不要选 Open Drain）
- `Internal Resistor`：No Resistor
- `Invert`：Disabled
- `Drive Strength Control`：Low
- `High-Impedance`：Disabled
- `Event Subscribing Channel`：Disabled (0)
- `Output Policy`：Bit will be Set
- `LaunchPad-Specific Pin`：No Shortcut Used
- `PinMux`：必须选下文指定的确切 PA/PB 引脚，不能保留 Any

普通 GPIO 输入统一填写：

- `Direction`：Input
- `IO Structure`：Any
- `Internal Resistor`：按下文选择 Pull-Up 或 No Resistor
- `Invert`：Disabled；高低有效在代码中处理
- `Event Subscribing Channel`：Disabled (0)
- `LaunchPad-Specific Pin`：No Shortcut Used
- `PinMux`：选择确切引脚

输入引脚上的 `Initial Value`、`Drive Strength` 和 `Output Policy` 没有实际作用，保持默认即可。

## 三、六个 GPIO 实例逐项填写

### 1. MOTOR_CTRL

顶部：

- `Name`：MOTOR_CTRL
- `Port`：PORTB
- `Port Segment`：Lower

点五次 `ADD`，加入以下 Group Pins：

| Pin Name | Direction | Initial Value | Internal Resistor | Interrupt | Assigned Pin / PinMux |
|---|---|---|---|---|---|
| MOTOR_A_IN1 | Output | Cleared | No Resistor | Disabled | 4 / PB4 |
| MOTOR_A_IN2 | Output | Cleared | No Resistor | Disabled | 5 / PB5 |
| MOTOR_B_IN1 | Output | Cleared | No Resistor | Disabled | 6 / PB6 |
| MOTOR_B_IN2 | Output | Cleared | No Resistor | Disabled | 7 / PB7 |
| MOTOR_STBY | Output | Cleared | No Resistor | Disabled | 12 / PB12 |

这五个脚上电全为低，使 TB6612 保持安全关闭。不要给 STBY 配上拉。

### 2. GRAY_SERIAL

顶部：

- `Name`：GRAY_SERIAL
- `Port`：PORTB
- `Port Segment`：Lower

加入两个 Group Pins：

| Pin Name | Direction | Initial Value | Internal Resistor | Interrupt | Assigned Pin / PinMux |
|---|---|---|---|---|---|
| GRAY_CLK | Output | Cleared | No Resistor | Disabled | 8 / PB8 |
| GRAY_DAT | Input | 不适用 | Pull-Up | Disabled | 9 / PB9 |

两脚的 `Invert` 都选 Disabled。串行协议由软件拉低/拉高 CLK 并读取 DAT，不要把它们配置成 I2C、SPI 或 UART。

### 3. ENCODER_IO

顶部：

- `Name`：ENCODER_IO
- `Port`：PORTB
- `Port Segment`：Upper

加入四个 Group Pins：

| Pin Name | Direction | Internal Resistor | GPIO Interrupt | Polarity | Assigned Pin / PinMux |
|---|---|---|---|---|---|
| ENCODER_E1A | Input | No Resistor | Enabled | Rising Edge | 17 / PB17 |
| ENCODER_E1B | Input | No Resistor | Disabled | 不适用 | 20 / PB20 |
| ENCODER_E2A | Input | No Resistor | Enabled | Rising Edge | 21 / PB21 |
| ENCODER_E2B | Input | No Resistor | Disabled | 不适用 | 27 / PB27 |

只对两路 A 相开上升沿中断；中断到来时读取对应 B 相判断方向。不要同时打开下降沿，否则每圈计数倍数会改变，速度 PID 参数也必须重标。

编码器接口电平必须先测量：如果 E1A/E1B/E2A/E2B 输出高电平接近 5 V，不能直接进入 MSPM0。使用电平转换，或每路用 10 kΩ 串联在信号端、20 kΩ 从 MCU 输入端接 GND，将 5 V 分压到约 3.33 V。若编码器是开集电极输出，则不要用这种分压，改为把信号上拉到 3.3 V。

### 4. START_KEY

顶部：

- `Name`：START_KEY
- `Port`：PORTA
- `Port Segment`：Upper

唯一引脚：

- `Pin Name`：KEY1
- `Direction`：Input
- `Internal Resistor`：Pull-Up
- `Invert`：Disabled
- `GPIO Interrupt`：Disabled（程序每 1 ms 轮询并消抖）
- `Assigned Pin / PinMux`：18 / PA18

如果模板中已有 PA18 的 `KEY_PORT/KEY1`，直接修改/保留它，不要重复添加，否则会引脚冲突。

### 5. BLUE_LED

顶部：

- `Name`：BLUE_LED_PORT
- `Port`：PORTA
- `Port Segment`：Lower

唯一引脚：

- `Pin Name`：BLUE_LED
- `Direction`：Output
- `Initial Value`：Set
- `Internal Resistor`：No Resistor
- `Invert`：Disabled
- `Assigned Pin / PinMux`：7 / PA7

板载蓝灯低电平亮，因此上电先输出高电平让它熄灭。如果模板中已有 PA7，修改已有实例，不要重复添加。

### 6. BUZZER

顶部：

- `Name`：BUZZER_PORT
- `Port`：PORTB
- `Port Segment`：Upper

唯一引脚：

- `Pin Name`：BUZZER
- `Direction`：Output
- `Initial Value`：Cleared
- `Internal Resistor`：No Resistor
- `Invert`：Disabled
- `Assigned Pin / PinMux`：26 / PB26

## 四、两路电机 PWM 定时器

左侧添加两个 `TIMER-PWM` 实例。PWM 只接 TB6612 的 PWMA/PWMB，不接编码器。

### 1. PWM_LEFT

- `Name`：PWM_LEFT
- `Peripheral`：TIMG0
- `Clock Source`：BUSCLK（或 SysConfig 默认时钟）
- `PWM Mode`：Edge-Aligned
- `Counting Mode`：Up
- `PWM Period/Frequency`：50 us / 20 kHz
- `Channel`：CC1/CCP1
- `Duty Cycle`：0%
- `Zero Event Action`：Set
- `Compare Up Event Action`：Clear
- `Output Inversion`：Disabled
- `PinMux`：PA13 / TIMG0_C1
- `Timer Start`：代码启动；若界面只有 `Start Timer` 选项，也可 Enabled
- `Interrupts`：全部 Disabled
- `Events/DMA`：全部 Disabled

### 2. PWM_RIGHT

- `Name`：PWM_RIGHT
- `Peripheral`：TIMA1
- `Clock Source`：BUSCLK（或 SysConfig 默认时钟）
- `PWM Mode`：Edge-Aligned
- `Counting Mode`：Up
- `PWM Period/Frequency`：50 us / 20 kHz
- `Channel`：CC0/CCP0
- `Duty Cycle`：0%
- `Zero Event Action`：Set
- `Compare Up Event Action`：Clear
- `Output Inversion`：Disabled
- `PinMux`：PA15 / TIMA1_C0
- `Timer Start`：代码启动；若界面只有 `Start Timer` 选项，也可 Enabled
- `Interrupts`：全部 Disabled
- `Events/DMA`：全部 Disabled

如果界面只让填 `Timer Count` 而不是频率，用 SysConfig 显示的 timer clock 除以 20000。例如 timer clock 为 32 MHz，周期计数填 1600。以界面最终显示 20.000 kHz 为准。

## 五、1 ms 控制定时器

添加一个普通 `TIMER` 实例：

- `Name`：CONTROL_TIMER
- `Peripheral`：TIMG12
- `Timer Mode`：Periodic
- `Repeat Mode`：Continuous/Repeat
- `Timer Period`：1 ms
- `Clock Source`：BUSCLK（保持默认分频也可以，SysConfig 会计算装载值）
- `Start Timer`：Enabled
- `Interrupts`：只勾选 ZERO
- `Interrupt Priority`：默认即可
- `Event Publisher/Subscriber`：Disabled
- `DMA Trigger`：Disabled
- `PinMux`：无；周期定时器不向外输出

这个中断每 1 ms 运行按键消抖、状态机时基；每 5 ms 读取灰度并进行循迹；每 10 ms 采样编码器并运行左右轮速度 PI。不要再另外添加 SysTick 或第二个周期定时器。

## 六、ADC、UART、DMA 第一题怎样配置

### ADC12：不添加

第一题的灰度辅助板走 `CLK/DAT` 数字串行接口，不再使用传感器本体的 `AD0/AD1/AD2/OUT`，因此不需要灰度 ADC。

TB6612 模块引出的 `ADC` 是电池电压 1:11 分压检测，不是电机电流，也不是编码器信号。第一题“直线行驶并在 B 点停车”不依赖电池电压检测，所以 `ADC` 脚先悬空，SysConfig 中不添加 ADC12。后续若要低电压报警，再单独加入 PA25/A0_2、ADC0、MEM0、12-bit、软件触发、VDDA/VSSA、单次转换；换算约为 `raw / 4095 * 3.3 * 11`。

### UART：不添加

第一题控制不需要串口。陀螺仪也不参与第一题，因此不要添加 UART。PA10/PA11 预留给后续陀螺仪串口，不在本题配置。

### DMA：不添加

灰度串行只有 8 bit，GPIO 读取即可；编码器由 GPIO 中断计数；PWM 由定时器硬件输出；没有大块高速数据搬运需求，所以不添加 DMA。

### I2C、SPI、DAC、COMP：不添加

辅助板虽然可能支持其他通信模式，本方案使用 CLK/DAT 串行方式，因此这些模块均不需要。

## 七、电源与接线注意事项

- 灰度辅助板：`5V -> 稳定 5 V`，`GND -> MCU GND`，`CLK -> PB8`，`DAT -> PB9`。
- TB6612：逻辑 VCC 接 3.3 V；VM 接电机电源；所有 GND 与电池负极共地。
- 左后轮接 AO1/AO2，右后轮接 BO1/BO2；若单侧方向反了，交换该侧两根电机线。
- 第一次测试必须架空后轮，并准备随时断开 VM 电源。

## 八、保存前逐项检查

1. 页面右上角不能有红色错误或黄色引脚冲突。
2. 每个引脚底部都显示确切的 `PAxx/PBxx`，不能显示 `Any(...)`。
3. PA13 必须是 TIMG0_C1，PA15 必须是 TIMA1_C0。
4. PB17/PB21 只开 Rising Edge；PB20/PB27 不开中断。
5. STBY、四个方向脚、两个 PWM、蜂鸣器上电均为关闭状态。
6. 保存 `empty.syscfg` 后等待 `ti_msp_dl_config.c/.h` 自动重新生成，再 Build Project。

当前工程已经换成辅助板 `CLK/DAT` 串行驱动，并加入编码器直线修正。新增的 `encoder.c`、`speed_balance.c` 需要在 CCS 中右键工程执行 `Refresh` 后才会进入构建清单。
