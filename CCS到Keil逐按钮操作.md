# MSPM0G3507：CCS 生成配置，Keil 编译和烧录

本文按以下环境编写：

- Code Composer Studio Theia 当前版本；
- MSPM0 SDK 2.x；
- Keil MDK / µVision 5.38a 或更高版本，Arm Compiler 6；
- MSPM0G3507 64-LQFP；
- XDS110，使用 CMSIS-DAP/SWD 烧录。

不同版本的个别菜单文字可能略有不同，但项目结构和操作顺序相同。

---

## 一、安装所需软件

### 1. 安装 MSPM0 SDK

1. 运行 MSPM0 SDK 安装程序。
2. 连续点击 `Next`。
3. 安装路径建议保持默认，例如：

```text
C:\ti\mspm0_sdk_2_11_00_07
```

4. 点击 `Install`。
5. 安装完成后点击 `Finish`。
6. 记住实际安装路径，后文用 `<SDK目录>` 表示它。

### 2. 在 Keil 安装器件包

1. 启动 `Keil µVision`。
2. 点击顶部菜单 `Project`。
3. 点击 `Manage`。
4. 点击 `Pack Installer...`。
5. 等待右下角 Pack 索引更新完成。
6. 在左上角搜索框输入：

```text
MSPM0G3507
```

7. 选择搜索结果中的 `MSPM0G3507`。
8. 在右侧找到：

```text
TexasInstruments::MSPM0G1X0X_G3X0X_DFP
```

9. 点击右侧的 `Install`。
10. 如果弹出许可证窗口，勾选同意并点击 `OK`。
11. 等状态变为 `Up to date` 或 `Installed` 后关闭 Pack Installer。

如果只能看到旧的 `MSPM0G_DFP`，先点击 Pack Installer 的刷新按钮；旧包
已经被新名称替代。

---

## 二、在 CCS 建立 SysConfig 工程

### 1. 创建空 DriverLib 工程

1. 启动 `Code Composer Studio`。
2. 在欢迎页点击 `Create a new project`。
   - 如果没有欢迎页：点击 `Help -> Getting Started`；
   - 然后点击 `Create a new project with Code Composer Studio IDE`。
3. 在 Device/Board 搜索框输入：

```text
MSPM0G3507
```

4. 选择 `MSPM0G3507` 或 `LP-MSPM0G3507`。
5. 在 Example/Template 搜索框输入：

```text
empty_driverlib_src
```

6. 选择 `Empty DriverLib Source`。
7. Compiler 选择 `TI Arm Clang`。
8. RTOS 选择 `No RTOS`。
9. Project Name 输入，例如：

```text
motor_car_ccs
```

10. 点击 `Create`。
11. 等待 CCS 下载/载入 SDK 和建立工程。

如果 CCS 提示没有安装 MSPM0 SDK：

1. 点击提示框中的 `Install`；
2. 等安装完成；
3. 回到工程向导重新选择 `empty_driverlib_src`。

### 2. 打开 SysConfig

1. 在左侧 `Project Explorer` 展开工程。
2. 找到扩展名为 `.syscfg` 的文件。
3. 双击 `.syscfg`。
4. 等待 SysConfig Device View 打开。
5. 检查右上角或 Device 设置：
   - Device：`MSPM0G3507`；
   - Package：`PM / 64-LQFP`。

如果当前工程是 LaunchPad 工程，Device 通常已经正确，但 Package 仍要核对。

---

## 三、在 SysConfig 中设置时钟

1. 在 SysConfig 左侧搜索框输入 `SYSCTL`。
2. 点击 `SYSCTL` 模块。
3. 展开 `Clock Configuration` 或 `Clock Tree`。
4. 设置系统主频为 `80 MHz`。
5. 查看界面顶部：
   - 黄色三角是警告；
   - 红色叉号是错误，必须先清除才能生成。

代码中的 1 ms 软件延时按照 80 MHz 编写，所以不要把 CPU 时钟改成其他值。

---

## 四、配置 PA13 的电机 A PWM

1. 在左侧搜索框输入：

```text
TIMER-PWM
```

2. 找到 `TIMER-PWM`。
3. 点击模块右侧的 `+`，添加第一个 PWM 实例。
4. 将 `Name` 改为：

```text
PWM_A
```

5. 在 `Basic Configuration` 中设置：
   - PWM Mode：`Edge-aligned`；
   - PWM Frequency：`20000 Hz`；
   - Initial Duty Cycle：`0%`；
   - Count Direction：`Up`；
   - Start Timer：勾选。
6. 展开底部 `PinMux` 或 `PinMux - Peripheral and Pin Configuration`。
7. Peripheral/Timer 选择：

```text
TIMG0
```

8. Capture/Compare Channel 选择：

```text
CCP1 / Channel 1
```

9. Pin 选择：

```text
PA13
```

10. 确认旁边显示的复用功能是：

```text
TIMG0_C1
```

11. 在 Output Action/CCP Action 中确认：
   - ZERO event：`Set`；
   - CC-UP event：`Clear`；
   - Output inversion：`Disabled`。

如果 SysConfig 使用“PWM starts high/low”的简化选项，选择 `Starts High`，
其含义必须是 ZERO 后为高、比较匹配后为低。

---

## 五、配置 PA15 的电机 B PWM

1. 再次点击 `TIMER-PWM` 右侧的 `+`。
2. 将 `Name` 改为：

```text
PWM_B
```

3. 设置：
   - PWM Mode：`Edge-aligned`；
   - PWM Frequency：`20000 Hz`；
   - Initial Duty Cycle：`0%`；
   - Count Direction：`Up`；
   - Start Timer：勾选。
4. 展开 `PinMux`。
5. Peripheral/Timer 选择：

```text
TIMA1
```

6. Channel 选择：

```text
CCP0 / Channel 0
```

7. Pin 选择：

```text
PA15
```

8. 确认复用功能显示：

```text
TIMA1_C0
```

9. 输出动作同样设置为 ZERO 置高、CC-UP 清低、禁止反相。

---

## 六、配置 PB4～PB7 方向输出

1. 在 SysConfig 左侧搜索框输入 `GPIO`。
2. 找到 GPIO 模块并点击右侧 `+`。
3. Name 输入：

```text
MOTOR_DIR
```

4. 选择 `Digital Output`。
5. 点击 `Add`、`+` 或 `Add GPIO`，依次建立四个输出：

| Name | Pin | Initial Output |
|---|---|---|
| AIN1 | PB4 | Low |
| AIN2 | PB5 | Low |
| BIN1 | PB6 | Low |
| BIN2 | PB7 | Low |

6. 四个引脚都设置：
   - Direction：`Output`；
   - Output：`Low`；
   - Invert：`Disabled`；
   - Open Drain：`Disabled`。

如果当前 SysConfig 版本要求每个 GPIO 单独添加四次模块，也可以分别创建
`AIN1`、`AIN2`、`BIN1`、`BIN2`，程序直接使用 GPIOB 寄存器，不依赖
模块名称。

---

## 七、配置 PB22～PB24 按钮输入

1. 再次点击 GPIO 模块右侧 `+`。
2. Name 输入：

```text
BUTTONS
```

3. 选择 `Digital Input`。
4. 依次添加：

| Name | Pin | Pull |
|---|---|---|
| KEY_EQUAL | PB22 | Pull-down |
| KEY_A_FAST | PB23 | Pull-down |
| KEY_B_FAST | PB24 | Pull-down |

5. 三个输入都设置：
   - Direction：`Input`；
   - Internal Resistor：`Pull-down`；
   - Hysteresis：保持默认或 Enabled；
   - Invert：`Disabled`；
   - Interrupt：`Disabled`。

6. 按钮另一端接 `3.3 V`。
7. 不要把按钮另一端接 5 V。

程序采用轮询和 20 ms 消抖，因此不需要打开 GPIO 中断。

---

## 八、检查并让 CCS 生成配置代码

1. 点击 SysConfig 编辑器顶部或右上角的 `Save`，也可以按 `Ctrl+S`。
2. 查看顶部 Problems/Issues：
   - 必须没有红色 Error；
   - 黄色 Warning 要逐个确认。
3. 切回 CCS 主界面。
4. 在左侧 `Project Explorer` 中右键工程名称。
5. 点击 `Build Project`；也可以点击顶部锤子图标。
6. 等 Console 最后显示：

```text
Build Finished
```

或 `0 errors`。

7. 在 Project Explorer 中寻找：

```text
Generated Source
  └─ SysConfig
      ├─ ti_msp_dl_config.c
      └─ ti_msp_dl_config.h
```

某些 CCS 版本显示为：

```text
Debug
  └─ syscfg
      ├─ ti_msp_dl_config.c
      └─ ti_msp_dl_config.h
```

8. 如果看不到，按 `Ctrl+P`，输入 `ti_msp_dl_config.c` 搜索。
9. 右键该文件，点击 `Reveal in File Explorer`、`Show in Explorer`
   或 `Open Containing Folder`。
10. 记下 `ti_msp_dl_config.c/.h` 所在位置。

不要只打开文件后复制文本；应复制完整文件。

---

## 九、用 SDK 模板建立 Keil 工程

### 1. 复制空工程

1. 打开 Windows 文件资源管理器。
2. 进入：

```text
<SDK目录>\examples\nortos\LP_MSPM0G3507\
driverlib\empty_driverlib_src
```

3. 复制整个 `empty_driverlib_src` 文件夹。
4. 粘贴到自己的工程目录，例如：

```text
F:\电赛小车\keil_motor_car
```

5. 进入复制后的：

```text
F:\电赛小车\keil_motor_car\keil
```

6. 双击其中的 `.uvprojx` 文件。

不要直接修改 `C:\ti` 下的 SDK 原始示例，否则重新安装 SDK 后容易丢失。

### 2. 删除模板的 main.c

1. 在 Keil 左侧 `Project` 窗口展开 Target。
2. 展开 `Source`、`Application` 或 `Source Group 1`。
3. 找到模板自带的 `main.c`。
4. 右键 `main.c`。
5. 点击 `Remove File from Group`。
6. 弹出确认时点击 `Yes`。

这只从 Keil 工程移除文件，一般不会删除磁盘文件。

### 3. 复制业务代码

1. 用 Windows 文件资源管理器打开：

```text
F:\电赛小车
```

2. 复制：

```text
main.c
motor.c
motor.h
buttons.c
buttons.h
```

3. 粘贴到 Keil 工程的源码目录，建议：

```text
F:\电赛小车\keil_motor_car
```

4. 回到 Keil。
5. 右键 `Source Group 1` 或 `Application`。
6. 点击：

```text
Add Existing Files to Group...
```

7. 文件类型选择 `C Source file (*.c)`。
8. 按住 Ctrl，选中：
   - `main.c`；
   - `motor.c`；
   - `buttons.c`。
9. 点击 `Add`。
10. 点击 `Close`。
11. 需要在工程树显示头文件时，再右键该组：
    - 点击 `Add Existing Files to Group...`；
    - 文件类型改为 `Header file (*.h)` 或 `All files (*.*)`；
    - 添加 `motor.h`、`buttons.h`。

### 4. 替换 SysConfig 生成文件

1. 把 CCS 生成的：

```text
ti_msp_dl_config.c
ti_msp_dl_config.h
```

复制到 Keil 工程源码目录。

2. 如果 Windows 提示同名文件：
   - 点击 `Replace the files in the destination`。
3. 回到 Keil。
4. 在工程树中找到旧的 `ti_msp_dl_config.c`。
5. 如果旧文件路径不是刚复制的位置：
   - 右键旧文件；
   - 点击 `Remove File from Group`；
   - 右键 Source Group；
   - 点击 `Add Existing Files to Group...`；
   - 添加刚复制的 `ti_msp_dl_config.c`。
6. `ti_msp_dl_config.h` 可以加入 Header Group，也可以只放在 include 路径中。

如果 CCS 还生成了 `ti_msp_config.c/.h`、NONMAIN 配置文件或其他 `.c`
文件，也必须用同样方式加入 Keil。普通 GPIO/PWM 工程通常只需要
`ti_msp_dl_config.c/.h`。

---

## 十、检查 Keil 目标和编译器

1. 在 Keil 顶部点击“魔术棒”图标：

```text
Options for Target
```

也可以点击 `Project -> Options for Target...`。

2. 打开 `Device` 标签。
3. 确认显示：

```text
Texas Instruments MSPM0G3507
```

4. 打开 `Target` 标签。
5. 检查 Xtal/Clock 显示接近 `80.0 MHz`。这里主要供调试器计算使用，
   实际系统时钟由 SysConfig 代码设置。
6. 打开 `Output` 标签。
7. 勾选：

```text
Create Executable
```

8. 如果需要给其他烧录软件使用，勾选：

```text
Create HEX File
```

9. 打开 `C/C++ (AC6)` 标签。
10. 确认 Compiler 为 Arm Compiler 6。
11. 点击 `Include Paths` 右侧的 `...`。
12. 如果业务代码和 `ti_msp_dl_config.h` 不在同一目录，点击 `New`，
    添加它们所在目录。
13. 保留 SDK 模板原有的 DriverLib/CMSIS include paths，不要删除。
14. 点击 `OK` 保存。

不要把 CCS 的 `.cmd` 文件、CCS 启动文件或 TI Clang 目标文件加入 Keil。

---

## 十一、第一次编译

1. 点击顶部菜单 `Project`。
2. 点击 `Rebuild all target files`。
   - 快捷方式一般是 `F7`；
   - 也可以点击工具栏的 Rebuild 图标。
3. 查看下方 `Build Output`。
4. 正常结束应看到：

```text
0 Error(s)
```

常见错误：

### 找不到 ti_msp_dl_config.h

1. 点击 `Options for Target` 魔术棒。
2. 打开 `C/C++ (AC6)`。
3. 点击 `Include Paths` 右侧 `...`。
4. 添加 `ti_msp_dl_config.h` 所在目录。
5. 点击 `OK`。
6. 再次点击 `Rebuild`。

### main 被重复定义

工程里还有两个 `main.c`：

1. 在工程树查找所有 `main.c`；
2. 保留本项目的 `main.c`；
3. 右键模板的 `main.c`；
4. 点击 `Remove File from Group`。

### DriverLib 函数或宏不存在

通常是 CCS 与 Keil 使用了不同版本 MSPM0 SDK：

1. 查看 CCS 工程的 SDK 版本；
2. 查看 Keil 模板路径中的 SDK 版本；
3. 用同一版本 SDK 的 Keil empty_driverlib_src 重新建立工程；
4. 重新复制 `.syscfg` 生成文件。

---

## 十二、配置 XDS110 和烧录算法

### 1. 硬件连接

LP-MSPM0G3507 使用板载 XDS110 时，只需连接 LaunchPad 的 Debug USB。

自制板连接外部 XDS110：

| XDS110 | MSPM0G3507 |
|---|---|
| SWDIO | PA19 |
| SWCLK | PA20 |
| nRESET | NRST |
| GND | GND |
| VTref | 目标板 3.3 V |

先给目标板供电，再插调试器。VTref 是电平参考，不要让两个电源互相反灌。

### 2. 选择 CMSIS-DAP

1. 在 Keil 点击 `Options for Target` 魔术棒。
2. 点击 `Debug` 标签。
3. 选择右侧单选框：

```text
Use:
```

4. 下拉框选择：

```text
CMSIS-DAP Debugger
```

5. 点击右侧 `Settings`。
6. 在 `Debug` 标签中：
   - Port：选择 `SW`；
   - Clock：第一次建议选择 `1 MHz` 或更低；
   - 如果能看到调试器序列号，说明 XDS110 已被识别。
7. 点击 `Flash Download` 标签。

### 3. 添加 Flash 算法

1. 查看 Programming Algorithm 列表。
2. 如果已经有名称包含以下文字的算法，就不必添加：

```text
MSPM0G3507 MAIN
```

或：

```text
MSPM0G350x MAIN
```

3. 如果列表为空，点击 `Add`。
4. 选择 MSPM0G3507/MSPM0G350x 的 `MAIN` On-chip Flash。
5. 点击 `Add` 或双击该算法。
6. 勾选：
   - `Erase Sectors`；
   - `Program`；
   - `Verify`；
   - `Reset and Run`。
7. 点击 `OK` 关闭 Settings。
8. 再点击 `OK` 关闭 Options for Target。

---

## 十三、烧录并运行

1. 先点击 `Project -> Build Target` 或按 `F7`。
2. 确认 `0 Error(s)`。
3. 点击顶部菜单 `Flash`。
4. 点击：

```text
Download
```

通常快捷键是 `F8`，工具栏图标是向下箭头/Load。

5. 查看下方 Output，应出现：

```text
Erase Done
Programming Done
Verify OK
```

6. 如果勾选了 `Reset and Run`，程序会立即运行。
7. 否则按一下开发板 RESET。
8. 上电后电机应保持停止。
9. 按 PB22 对应按钮：两路 PWM 都变为 80%。
10. 按 PB23：A=80%，B≈52.6%。
11. 按 PB24：A≈52.6%，B=80%。

第一次测试建议先断开电机电源，只给 MCU 和电机驱动逻辑供电，用示波器测
PA13、PA15，再接电机。

---

## 十四、进入单步调试

1. 点击 `Debug` 菜单。
2. 点击：

```text
Start/Stop Debug Session
```

通常快捷键是 `Ctrl+F5`。

3. Keil 会下载程序并停在 `main()` 附近。
4. 点击：
   - `Run`：全速运行；
   - `Step`：单步进入；
   - `Step Over`：单步越过函数。
5. 退出时再次点击 `Debug -> Start/Stop Debug Session`。

---

## 十五、推荐的长期使用方式

第一次可以从 CCS 复制 `ti_msp_dl_config.c/.h`。以后建议让 Keil 直接
调用同一个 `.syscfg`：

1. 编辑 `<SDK目录>\tools\keil\syscfg.bat`；
2. 检查 `SYSCFG_PATH` 指向独立安装的 SysConfig；
3. 在 Keil 点击 `Tools -> Customize Tools Menu...`；
4. 点击 `Import`；
5. 选择：

```text
<SDK目录>\tools\keil\MSPM0_SDK_syscfg_menu_import.cfg
```

6. 点击 `OK`；
7. 在 Keil 中选中 `.syscfg`；
8. 从 `Tools` 菜单运行导入的 MSPM0 SysConfig 工具；
9. 保存后重新生成配置文件；
10. 回到 Keil 点击 `Rebuild`。

这样就不需要每次从 CCS 手工复制生成文件，但 CCS 与 Keil 仍然可以共享
同一个 `.syscfg`。
