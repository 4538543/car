# 第一题：接线、SysConfig 与启动方法

> **本文件中的灰度 AD0/AD1/AD2/OUT 配置已经废止。** 你的灰度辅助板使用 CLK/DAT 串行输出，请以 [第一题SysConfig逐项配置.md](./第一题SysConfig逐项配置.md) 为准。旧版 `line_sensor.c` 也必须更换后才能烧录。

## 1. 软件行为

- 小车采用后轮左右双驱差速转向。
- Motor A 是左后轮，Motor B 是右后轮。
- 车放在 A 点，车头朝 B，黑线位于灰度板 CH4/CH5 中间。
- 按开发板 `KEY1`（PA18）一次启动。
- 小车沿 A->B 黑色直线循迹；持续检测到直线开始进入圆弧后，认为到达 B。
- 到 B 后 TB6612 短刹车 80 ms，随后关闭 STBY，同时蜂鸣并点亮蓝灯。
- 运行中再次按 `KEY1` 会立即急停。

## 2. TB6612 与双后轮接线

| TB6612 | MSPM0G3507/其他连接 | 说明 |
|---|---|---|
| PWMA | PA13 | TIMG0_C1，左后轮 PWM |
| AIN1 | PB4 | 左后轮方向 1 |
| AIN2 | PB5 | 左后轮方向 2 |
| AO1/AO2 | 左后轮电机两端 | 若前进方向反了，交换这两根电机线 |
| PWMB | PA15 | TIMA1_C0，右后轮 PWM |
| BIN1 | PB6 | 右后轮方向 1 |
| BIN2 | PB7 | 右后轮方向 2 |
| BO1/BO2 | 右后轮电机两端 | 若前进方向反了，交换这两根电机线 |
| STBY | PB12 | 高电平使能；不要悬空 |
| VCC | MCU 3.3 V | TB6612 逻辑电源允许 2.7~5.5 V |
| VM | 电机电池正极 | 必须符合电机和模块允许范围，TB6612 芯片手册范围约 4.5~13.5 V |
| GND | MCU GND、传感器 GND、电池负极 | 所有模块必须共地 |

首次测试请架空后轮。若只有一侧前进方向反了，只交换该侧 AO/BO 两根电机线，不要同时乱改 PWM 和方向代码。

## 3. 八路灰度与辅助板接线

传感器本体与辅助板按配套排针连接。辅助板/传感器 MCU 接口按下表连接：

| 灰度信号 | MSPM0G3507 | 说明 |
|---|---|---|
| AD0 | PB8 | 4051 地址位 0，GPIO 输出 |
| AD1 | PB9 | 4051 地址位 1，GPIO 输出 |
| AD2 | PB10 | 4051 地址位 2，GPIO 输出 |
| OUT | PA27 | ADC0 A0_0 模拟输入 |
| ERR | 暂不接 | 可悬空，后续再做故障输入 |
| EN | 悬空或接 GND | 低电平使能；不能接高电平 |
| +5V | 独立、稳定的 5 V | 不建议与电机、蜂鸣器等感性负载共用未经滤波的 5 V |
| GND | MCU GND | 必须共地 |

上电前用万用表确认 OUT 在黑/白表面下均不超过 3.3 V，再接 PA27。灰度板建议安装在车头，探头横向排列，CH1 在车的左侧、CH8 在车的右侧，先从约 15~20 mm 高度开始调。

## 4. 声光与按键

开发板已有资源，不需要额外接线：

- 启动/急停：板载 `KEY1`，对应 PA18，上拉、按下为低电平。
- 蓝色 LED：PA7，低电平点亮。
- 有源蜂鸣器：PB26，高电平鸣响。

注意：不要按 PB22/PB23/PB24 对应的另外三个按键。本版本只使用 `KEY1/PA18`。

## 5. SysConfig 配置

打开 `empty.syscfg`。保留已有 DEBUGSS、PA18 按键和 PA7 LED，然后添加以下配置。保存后让 CCS 自动重新生成 `ti_msp_dl_config.c/.h`。

### 5.1 系统时钟

当前 32 MHz 可以直接使用。代码读取生成的 `CPUCLK_FREQ`，不要在代码里另写 80 MHz。

### 5.2 GPIO 输出

添加 GPIO 输出组，全部为 Push-Pull、无反相：

| 名称 | 引脚 | 初始值 |
|---|---|---|
| MOTOR_A_IN1 | PB4 | Low |
| MOTOR_A_IN2 | PB5 | Low |
| MOTOR_B_IN1 | PB6 | Low |
| MOTOR_B_IN2 | PB7 | Low |
| MOTOR_STBY | PB12 | Low |
| LINE_AD0 | PB8 | Low |
| LINE_AD1 | PB9 | Low |
| LINE_AD2 | PB10 | Low |
| BUZZER | PB26 | Low |

把已有 PA7 `BLUE_LED` 的初始输出设为 High，使上电默认熄灭。

已有 PA18 `KEY1` 保持 Digital Input、Pull-Up、Falling/低有效，不需要 GPIO 中断。

### 5.3 左后轮 PWM

添加 `TIMER-PWM`：

- Peripheral：TIMG0
- PWM pin：PA13 / TIMG0_C1
- Channel：CCP1 / Channel 1
- Mode：Edge-aligned，向上计数
- Frequency：20 kHz
- Initial duty：0%
- ZERO action：Set
- CC-UP action：Clear
- Output inversion：Disabled

### 5.4 右后轮 PWM

再添加 `TIMER-PWM`：

- Peripheral：TIMA1
- PWM pin：PA15 / TIMA1_C0
- Channel：CCP0 / Channel 0
- Mode：Edge-aligned，向上计数
- Frequency：20 kHz
- Initial duty：0%
- ZERO action：Set
- CC-UP action：Clear
- Output inversion：Disabled

### 5.5 灰度 ADC

添加 `ADC12`：

- Peripheral：ADC0
- Analog input pin：PA27 / A0_0
- Resolution：12 bit
- Conversion mode：Single conversion
- Trigger：Software
- Memory：MEM0
- MEM0 channel：A0_0
- Voltage reference：VDDA/VSSA（MCU 3.3 V）
- Result loaded flag：MEM0 result loaded
- SysConfig 初始化时使能 ADC conversions

本程序轮询 ADC 的 raw interrupt flag，不需要开启 ADC 的 NVIC 中断。

## 6. 把新文件加入/确认在 CCS 工程中

代码已经放在 CCS 工程根目录。回到 Project Explorer：

1. 右键工程，选择 `Refresh`。
2. 确认能看到 `app_q1.c`、`button.c`、`indicator.c`、`line_control.c`、`line_sensor.c`、`motor.c`。
3. `empty.c` 是唯一的 `main()`；不要再把工程目录之外的其他 `main.c` 加进工程。
4. 保存 `empty.syscfg`，等待生成文件更新。
5. 选择 `Project -> Clean Project`，再点 `Build Project`。

## 7. 首次测试步骤

1. 不接电机 VM，只给 MCU 和逻辑侧供电；检查 STBY 上电为低、PA13/PA15 为 0% PWM。
2. 接传感器，在白纸和黑线上分别测 OUT，确认黑色 ADC 值小、白色 ADC 值大。
3. 架空左右后轮，按 KEY1；确认两轮都向前。再按一次确认立即停转。
4. 落地低速测试，把 A 点黑线放在 CH4/CH5 中间，车头朝 B。
5. 按 `KEY1/PA18` 启动，手随时放在电源开关附近。

## 8. 首次必调参数

参数集中在 `app_config.h`：

- 小车朝黑线反方向修正：把 `APP_STEERING_SIGN` 从 `1` 改成 `-1`。
- 直线摆动：先减小 `LINE_KP`；响应太慢再逐步增加。
- 抖动明显：适当增大 `LINE_KD`；噪声放大则减小。
- 直线中途误判 B：增大 `Q1_CURVE_ERROR_MIN` 或 `Q1_CURVE_CONFIRM_MS`。
- 已进入圆弧仍不停：减小上述两个阈值。
- 速度：先保持 `APP_BASE_SPEED_PERMILLE=350`，稳定后再加。
- 黑白值：实测后把 `LINE_BLACK_RAW_VALUES`、`LINE_WHITE_RAW_VALUES` 的 CH1~CH8 分别替换为每路标定值；正式比赛不应长期使用手册默认值。
