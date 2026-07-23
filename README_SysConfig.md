# MSPM0G3507 双电机驱动：CCS / SysConfig 设置

## 1. 硬件前提

本引脚表要求 **MSPM0G3507 64-LQFP（PM）封装**。PB22、PB23、PA29
在较小封装中没有全部引出。

本程序假定电机驱动器的接口是 `PWMA/PWMB + IN1/IN2`（例如 TB6612
一类逻辑），PWM 高电平为有效。驱动器地和 MCU 地必须共地。若驱动板有
`STBY`/`ENABLE` 引脚，还必须把它拉高或由另一个 GPIO 拉高。

## 2. 建立 CCS 工程

推荐从 MSPM0 SDK 的
`examples/nortos/LP_MSPM0G3507/driverlib/empty_driverlib_src`
导入一个 TI Arm Clang 工程，再把 `main.c`、`motor.c/.h`、
`buttons.c/.h` 加入工程。打开工程中的 `.syscfg` 文件进行以下设置。

Device 选择：

- Device：MSPM0G3507
- Package：64-PM (LQFP)
- SYSOSC / MCLK：80 MHz

## 3. PWM A：PA13

在左侧 **Timers -> TIMER-PWM** 添加一个实例：

- Name：`PWM_A`（名字不影响本程序）
- Timer instance：`TIMG0`
- PWM mode：Edge-aligned
- PWM frequency：20 kHz（10~20 kHz 均可）
- Timer count direction：Up
- PWM channel / CCP：Channel 1
- Output pin：`PA13`，功能应显示 `TIMG0_C1`
- Initial duty：0%
- 输出动作必须是：计数到 ZERO 时置高，CC-UP 匹配时清低
- Start timer：可以启用；程序也会调用 start，重复调用无害

如果时钟为 80 MHz、timer divider 为 1，20 kHz 的 period count 应为
4000，LOAD 为 3999。

## 4. PWM B：PA15

再添加一个 **TIMER-PWM** 实例：

- Name：`PWM_B`
- Timer instance：`TIMA1`
- PWM mode：Edge-aligned
- PWM frequency：20 kHz
- Timer count direction：Up
- PWM channel / CCP：Channel 0
- Output pin：`PA15`，功能应显示 `TIMA1_C0`
- Initial duty：0%
- 输出动作同样为 ZERO 置高、CC-UP 清低

## 5. 方向 GPIO

在 **GPIO** 中添加输出组（可命名为 `MOTOR_DIR`），全部设置：
Digital Output、初始 Low、无反相。

| 信号 | MCU 引脚 |
|---|---|
| AIN1 | PB4 |
| AIN2 | PB5 |
| BIN1 | PB6 |
| BIN2 | PB7 |

程序定义的正转逻辑是 IN1=1、IN2=0。如果某侧车轮方向相反，交换该电机
两根线，或修改 `Motor_writeDirectionPins()` 中该侧的逻辑。

## 6. 按钮 GPIO

在 **GPIO** 中添加输入组（可命名为 `BUTTONS`）：

| 功能 | MCU 引脚 | 设置 |
|---|---|---|
| 两电机 80% | PB22 | Digital Input，内部下拉 |
| A=80%，B=80%/1.52 | PB23 | Digital Input，内部下拉 |
| B=80%，A=80%/1.52 | PB24 | Digital Input，内部下拉 |

按钮另一端接 3.3 V，所以按下为高。不要接 5 V。程序每 1 ms 采样并做
20 ms 消抖；有效按下一次就切换并保持对应模式，松开不会停车。

## 7. 其余引脚

- PB8/PB9、PB10/PB11 编码器本版程序未使用。以后要闭环测速时再分别
  配置 QEI；当前所谓“80%速度”实际是 **80% PWM 占空比**，负载变化时
  真实转速不会严格保持 80%。
- PA29 电流 ADC 本版程序未使用；在 64-PM 封装中 PA29 是 `A0_1`，
  后续应在 ADC12 中选择 ADC0、通道 A0_1。输入电压必须处于 ADC 允许
  范围，不能直接采电机电流。

## 8. 编译前检查

保存 `.syscfg` 后，CCS 会重新生成 `ti_msp_dl_config.c/.h`。确认 PinMux
摘要中没有冲突，并检查：

- PA13 = TIMG0_C1
- PA15 = TIMA1_C0
- PB4~PB7 = GPIO output
- PB22~PB24 = GPIO input with pull-down

然后 Clean Project、Build Project、Debug。第一次上电建议先不接电机，
用示波器检查 PA13/PA15：启动时应为 0%，按三个按钮后应分别看到
80%/80%、80%/约 52.6%、约 52.6%/80%。

## 9. 在 CCS 配置、在 Keil 编译和烧录

### 9.1 安装环境

安装以下软件，并尽量让 CCS 和 Keil 使用同一个 MSPM0 SDK 版本：

1. MSPM0 SDK。
2. Keil MDK-Arm / uVision 5.38a 或更高版本。
3. 在 Keil 的 `Project -> Manage -> Pack Installer` 中安装 Texas
   Instruments MSPM0G3507 Device Pack。

版本一致非常重要：如果 CCS 的 `.syscfg` 使用 SDK 2.xx 元数据，而 Keil
工程引用另一版 DriverLib，生成代码可能出现宏或函数不匹配。

### 9.2 建立 Keil 工程模板

不要在 Keil 中新建一个完全空白的 Cortex-M 工程。打开 SDK 自带工程：

```text
C:\ti\mspm0_sdk_<版本>\examples\nortos\LP_MSPM0G3507\
driverlib\empty_driverlib_src\keil\*.uvprojx
```

把这个示例文件夹复制为自己的工程后再修改。该模板已经包含：

- MSPM0G3507 启动文件和中断向量表；
- Keil/ArmClang 对应的 Scatter 链接配置；
- DriverLib、CMSIS 和器件头文件路径；
- Flash 下载算法和调试器所需的基础配置。

删除或排除模板原来的 `main.c`，再把本项目的下列文件复制到 Keil 工程
目录，并在 Project 窗口右键 Source Group，选择
`Add Existing Files to Group`：

```text
main.c
motor.c
motor.h
buttons.c
buttons.h
```

### 9.3 传递 SysConfig 配置

有两种方式，推荐方式 A。

#### 方式 A：在 Keil 中重新生成

1. 把 CCS 工程中的 `.syscfg` 文件复制到 Keil 工程目录并加入工程。
2. 打开 `<MSPM0_SDK>\tools\keil\syscfg.bat`，确认其中
   `SYSCFG_PATH` 指向已安装的独立 SysConfig。
3. 在 Keil 中选择 `Tools -> Customize Tools Menu -> Import`。
4. 导入：

```text
<MSPM0_SDK>\tools\keil\MSPM0_SDK_syscfg_menu_import.cfg
```

5. 在 Keil 中选中或打开 `.syscfg`，从 Tools 菜单运行 MSPM0 SysConfig。
6. 保存配置。生成的 `ti_msp_dl_config.c` 和
   `ti_msp_dl_config.h` 应加入 Keil 工程。

这样以后修改 `.syscfg` 后，可以直接在 Keil 中重新生成。

#### 方式 B：复制 CCS 已生成的文件

在 CCS 中保存 `.syscfg` 并 Build 一次，然后在 CCS 工程的 Generated
Source / `syscfg` 目录找到：

```text
ti_msp_dl_config.c
ti_msp_dl_config.h
```

把它们复制到 Keil 工程并加入 Source Group。若所用 SysConfig 版本还生成
了 `ti_msp_config.c/.h`、NONMAIN 或其他配置文件，也要一并加入。

不要从 CCS 复制以下内容：

- TI Clang 使用的 `.cmd` 链接文件；
- CCS 工程元数据；
- CCS 的启动汇编文件；
- CCS 的 Debug/Release 目标文件。

Keil 工程只能保留一套启动文件和一个 `main()`。

### 9.4 Keil 编译设置

打开 `Options for Target` 并检查：

- Device：Texas Instruments -> MSPM0G3507；
- Compiler：Arm Compiler 6；
- C/C++ 中保留模板已有的 MSPM0 SDK include paths；
- 本项目源码路径已加入 Include Paths；
- Output 中可勾选 `Create HEX File`；
- 不要再添加 CCS 的 `.cmd` 文件。

按 `F7` Build。如果提示找不到 `ti_msp_dl_config.h`，把该文件所在目录加入
`C/C++ -> Include Paths`。如果出现 DriverLib 函数未定义，优先检查
SysConfig 和 Keil 工程使用的 SDK 是否为同一版本。

### 9.5 使用 XDS110 烧录

若使用 LP-MSPM0G3507 板载 XDS110：

1. `Options for Target -> Debug`；
2. Debugger 选择 `CMSIS-DAP Debugger`；
3. 进入 `Settings`，接口选择 SWD；
4. 打开 `Flash Download`；
5. 确认存在 MSPM0G3507 MAIN/On-chip Flash 下载算法，没有就点 Add；
6. 勾选 `Erase Sectors`、`Program`、`Verify`、`Reset and Run`；
7. Build 后点击工具栏 `Load/Download` 烧录。

自制板使用外部 XDS110 时至少连接：

| 调试器 | MSPM0G3507 |
|---|---|
| SWDIO | PA19 |
| SWCLK | PA20 |
| nRESET | NRST |
| GND | GND |
| VTref | 板上 3.3 V |

VTref 是目标电平参考。若开发板已经由外部电源供电，不要再让两个电源互相
反灌。首次下载失败时，降低 SWD Clock，并检查 NRST、供电和公共地。
