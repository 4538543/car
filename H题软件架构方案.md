# 2024 H 题自动行驶小车软件架构方案

## 1. 题目与硬件结论

题目场地由两条 100 cm 直线和两个半径 40 cm 的半圆组成。小车只能前进，经过 A、B、C、D 时需要声光提示。任务 3/4 还包含 A->C、B->D 两段没有黑线的开放场地行驶。

当前硬件适合采用以下控制方式：

- MSPM0G3507 负责 1 kHz 调度、ADC 采样、UART 接收、状态机和控制计算。
- 八路灰度通过 74HC4051 读取，不需要 8 路 ADC：3 个地址 GPIO（AD0~AD2）加 1 路 ADC（OUT）。
- 灰度板黑色输出低、白色输出高；每个探头必须分别做黑白标定，再归一化。
- 陀螺仪通过 UART 输出 5 字节二进制帧：`5A AA` 为角速度，`5A BB` 为 Yaw，末字节为低 8 位累加和。
- TB6612 使用两路 PWM、四路方向 GPIO 和 STBY。PWM 建议 20 kHz，STBY 必须拉高或由 GPIO 控制。
- 当前没有列出电机编码器，因此不能做真正的轮速 PID。现阶段速度只能用 PWM 前馈和左右轮标定；如果以后增加编码器，再加入左右轮速度 PI。

电气注意：灰度传感器使用独立、稳定的 5 V，不能和电机等感性负载共用未经隔离/滤波的 5 V。OUT 接 MCU ADC 前必须实测确认不超过 3.3 V，所有模块共地。

## 2. 总体分层

建议把程序分成四层，禁止任务逻辑直接写 GPIO 或 PWM。

```text
任务层       app / mission / route_table
             选择题目、执行路线、点位提示、圈数、完成/故障

行为控制层   vehicle_control / line_control / heading_control / recovery
             循线、无标志直行、转向接线、丢线恢复、速度规划

设备层       line_sensor / gyro / motor / buttons / indicator
             传感器值、UART 帧、PWM、按键、蜂鸣器和 LED

BSP 层       SysConfig 生成文件 / systick / adc / uart / timer / gpio
             只放 MSPM0 DriverLib 相关内容
```

同一时刻只能有一个“转向控制源”：循线状态使用循线控制器，开放场地状态使用航向控制器，恢复状态使用恢复控制器。不要让多个 PID 同时修改左右轮 PWM。

## 3. 建议文件

### 3.1 必需文件

| 文件 | 职责 | 关键接口 |
|---|---|---|
| `main.c` | 初始化和协作式调度，不放业务细节 | `App_init()`、`App_runOnce()` |
| `app.c/.h` | 系统总状态机 | `App_task10ms()`、`App_selectMission()` |
| `app_config.h` | 引脚以外的可调参数和功能开关 | 周期、限幅、超时、默认速度 |
| `route_table.c/.h` | 四道题的路线描述表 | `Route_get(missionId)` |
| `mission.c/.h` | 执行路线段、切段、点位和圈数 | `Mission_start()`、`Mission_update()` |
| `vehicle_control.c/.h` | 根据当前行为生成左右轮目标 | `Vehicle_update5ms()` |
| `line_sensor.c/.h` | 4051 地址切换、ADC、黑白标定、归一化 | `LineSensor_scan()`、`LineSensor_getFrame()` |
| `line_estimator.c/.h` | 质心、置信度、线状态、曲率事件 | `LineEstimator_update()` |
| `line_control.c/.h` | 循迹 PD、弯道降速 | `LineControl_step()` |
| `heading_control.c/.h` | 无线段航向保持、受控转向 | `HeadingControl_step()` |
| `recovery.c/.h` | 丢线判断、预测、搜索、重新接管 | `Recovery_update()` |
| `gyro.c/.h` | UART 字节状态机、校验、Yaw 解缠、数据超时 | `Gyro_rxByteISR()`、`Gyro_getSnapshot()` |
| `pid.c/.h` | 通用 PID/PI 数据结构、限幅、抗饱和、复位 | `PID_step()`、`PID_reset()` |
| `motor.c/.h` | TB6612 方向、PWM、STBY、前进安全限幅 | `Motor_setForward()`、`Motor_enable()` |
| `buttons.c/.h` | 消抖、短按、长按、组合键 | `Buttons_task1ms()`、`Buttons_getEvent()` |
| `indicator.c/.h` | 蜂鸣/LED 的非阻塞时序 | `Indicator_point()`、`Indicator_task10ms()` |
| `scheduler.c/.h` | 1 ms 节拍和任务标志 | `Scheduler_takeFlag()` |

### 3.2 调试阶段很有价值的文件

- `params.c/.h`：集中保存每条路线的速度、PID 和阈值，禁止“魔法数字”散落在状态机里。
- `telemetry.c/.h`：以 20~50 Hz 输出传感器、状态、误差和 PWM；不能在控制中断中 `printf`。
- `fault.c/.h`：记录灰度饱和、陀螺超时、ADC 异常、恢复超时等故障。

## 4. 调度模型

使用无 RTOS 的协作式调度即可。1 ms 定时器中断只置位计数/标志，禁止在中断里跑 PID、蜂鸣延时或打印。

| 周期 | 任务 |
|---|---|
| UART RX ISR | 收一个陀螺仪字节，投入 5 字节解析状态机 |
| ADC/1 ms | 完成八路灰度扫描或推进一次扫描序列 |
| 1 ms | 按键消抖、更新系统时间 |
| 5 ms（200 Hz） | 灰度估计、循线/航向控制、左右轮输出 |
| 10 ms（100 Hz） | 任务状态机、丢线状态机、声光时序 |
| 20~50 ms | 遥测输出和低优先级诊断 |

陀螺仪零偏校准约需 20 s，只能在显式 `CALIBRATING` 状态中非阻塞计时，不能调用 `delay_ms(21000)` 卡死整车。

## 5. 系统状态机

```c
typedef enum {
    APP_BOOT,
    APP_SELF_TEST,
    APP_SELECT,       // 选择 1~4 题
    APP_CALIBRATING,  // 可选：灰度/陀螺校准
    APP_ARMED,        // 等待启动，电机仍关闭
    APP_RUNNING,
    APP_FINISHED,
    APP_FAULT
} AppState;
```

推荐按键逻辑：

- “模式”短按：1->2->3->4 循环，LED/蜂鸣次数显示题号。
- “启动”短按：进入 ARMED；松手后延时 500 ms 启动，避免手碰车。
- “停止/复位”短按：立即 PWM=0、STBY=0，回到 SELECT。
- “模式+启动”长按：进入传感器校准，避免比赛时误触。

开发板原理图显示板载 PB22/PB23/PB24 按键按下接地，因此使用板载键时应配置为上拉、低有效。当前 `buttons.c` 的下拉、高有效逻辑只适用于外接到 3.3 V 的按键。

## 6. 路线段状态机与数据驱动切题

不要为每道题复制一份大状态机。把路线写成 `const SegmentDef[]`，由同一个执行器解释。

```c
typedef enum {
    SEG_FOLLOW_TO_CURVE,   // 沿直线，检测进入圆弧
    SEG_FOLLOW_ARC,        // 沿线走指定转角的半圆
    SEG_OPEN_TO_LINE,      // 无标志区航向保持，直到重新看见线
    SEG_JOIN_LINE,         // 只前进的柔和转弯，接入圆弧切线
    SEG_POINT_ACTION,      // 点位锁存和声光提示
    SEG_FINISH
} SegmentType;

typedef struct {
    SegmentType type;
    int8_t turnSign;           // -1/0/+1
    float targetTurnDeg;       // 半圆通常为 180°
    float targetHeadingDeg;    // 相对启动航向，开放段使用
    uint16_t basePwm;
    uint16_t minTimeMs;
    uint16_t timeoutMs;
    char arrivalPoint;         // 'A'/'B'/'C'/'D' 或 0
} SegmentDef;
```

每次切段必须统一执行：复位当前控制器积分/历史项、记录段起始 Yaw 和时间、清除事件去抖器、设置速度斜坡，避免上一段的积分带到下一段。

### 6.1 第 1 题：A->B 停车

起始将车头沿 A->B 方向摆放。A->B 的黑线在 B 处继续接右半圆，所以不能用“全白/线结束”判断 B。应使用“直线进入右弯”的曲率事件：

1. A 点启动并提示。
2. `SEG_FOLLOW_TO_CURVE`，设置最短直行时间，过滤启动瞬态。
3. 连续若干帧出现同方向转向需求，并且陀螺角速度/累计转角超过阈值，确认到达 B。
4. 立即声光提示、PWM 斜降并短刹车/滑行停车。

传感器装在车体前端时，检测到弯道会略晚。用 `B_STOP_COMP_MS` 或传感器到前轮/轴线的几何距离做补偿，实车标定到停车后车体投影仍覆盖 B。

### 6.2 第 2 题：A->B->C->D->A

这是连续黑线闭环：

1. A->B：直线，曲率开始确认 B。
2. B->C：右半圆，循线并对段起始解缠 Yaw 累计约 180°，结合曲率结束确认 C。
3. C->D：直线，曲率开始确认 D。
4. D->A：右转意义下的另一个半圆，累计约 180°，结合曲率结束确认 A并停车。

点位判断必须由“当前段 + 最短时间 + Yaw/曲率事件”共同决定，不能只看一个瞬时阈值。每个点设置一次性 latch，防止重复蜂鸣。

### 6.3 第 3 题：A->C->B->D->A

A->C 和 B->D 没有黑线，是本题最难部分。场地几何给出横向 100 cm、纵向 80 cm，因此对角线约 128.1 cm，与水平线夹角约 38.66°。

推荐流程：

1. 起始在 A，把车头直接朝 C 摆放并将该方向定义为相对航向 0°。
2. A 提示后进入 `SEG_OPEN_TO_LINE`，航向保持直行。刚离开 A 的全白是预期行为，不进入丢线恢复。
3. 超过最短行驶时间后，灰度置信度连续有效才确认到达 C；C 提示。
4. `SEG_JOIN_LINE`：保持前进，以柔和圆弧调整约 38.66°，接入 C->B 右圆的底部切线，再进入循线。
5. C->B 沿半圆走约 180°，以解缠 Yaw 和线置信度确认 B；B 提示。
6. 从 B 以只前进转弯修正约 38.66°，进入 B->D 对角线航向保持。
7. 超过最短时间后重新见线，确认 D；D 提示。再柔和接入 D->A 半圆。
8. 半圆累计约 180°到 A，提示并停车。

开放段必须设置 `minTimeMs`，否则离开 A/B 时会把原来的黑线误判成目标线；同时设置 `timeoutMs`，超时后停车而不是一直冲出场地。

### 6.4 第 4 题：按第 3 题路线四圈

复用第 3 题段表。到 A 后如果 `lapCount < 4`，不进入 FINISH，而是把下一段设为 A->C、圈数加一并继续；第 4 次到 A 才停车。圈数只允许在“D->A 段完成且 A 事件首次锁存”时增加。

## 7. 灰度采集与线估计

### 7.1 采集

按通道 0~7 输出 AD2/AD1/AD0，等待模拟开关和 ADC 采样稳定，再读取 OUT。稳定时间先从 20~50 us 试起，以示波器/重复性为准。一个完整 Frame 必须带时间戳，控制器只读取完整帧，不能读到一半新一半旧的数据。

```c
typedef struct {
    uint16_t raw[8];
    uint16_t darkness[8];   // 0=白，1000=黑
    uint32_t timestampMs;
    bool valid;
} LineFrame;
```

每路归一化：

```text
whiteNorm = clamp((raw - black[i]) * 1000 / (white[i] - black[i]), 0, 1000)
darkness  = 1000 - whiteNorm
```

如果 `white[i] - black[i]` 太小，标记该通道校准失败。可以对归一化值做一阶 IIR，但不要用太强滤波造成高速弯道延迟。

### 7.2 位置和置信度

使用加权质心，权重可取 `{-3500,-2500,-1500,-500,500,1500,2500,3500}`：

```text
lineError = sum(weight[i] * darkness[i]) / sum(darkness[i])
```

同时输出：

- `confidence`：总黑度及峰值是否足够。
- `activeCount`：超过阈值的探头数量。
- `edgeSide`：线最后出现在左边还是右边。
- `curveEvidence`：转向输出、线误差和陀螺角速度的低通结果。
- `allWhite/allBlack`：分别用于丢线/异常诊断；不要直接拿一次 allWhite 切状态。

## 8. 控制器配置

### 8.1 第一阶段必须实现

1. **循迹 PD**：`turn = Kp*lineError + Kd*dError`。先禁用 I，避免弯道和丢线时积分饱和。
2. **开放段航向控制**：外环用 Yaw 误差 P 产生目标角速度，内环用陀螺角速度 PI 产生差速量。
3. **PWM 前馈/左右轮补偿**：用 `leftGain/rightGain` 修正同占空比不等速；没有编码器时这不是 PID。

### 8.2 稳定后再加

- 循线控制可加入陀螺角速度阻尼：线误差生成目标角速度，角速度 PI 生成差速 PWM。
- 若电机带编码器，再为左右轮各加一个速度 PI；通常不需要 D。
- 电池电压 ADC 可用于 PWM 前馈补偿，但不是比赛第一优先级。

### 8.3 电机混合和“只能前进”

```text
left  = basePwm - turnPwm
right = basePwm + turnPwm
```

统一在 `Motor_setForward()` 中把两侧限制到 `[0, maxPwm]`，任务运行时绝不调用 REVERSE。急转时允许内侧轮为 0、外侧轮前进，不允许一正一反原地转。弯道按 `abs(turnPwm)` 或 `abs(lineError)` 降低 `basePwm`，并对 PWM 做斜坡，减少打滑。

所有 PID 结构都应包含：显式 `dt`、输出限幅、积分限幅、条件积分/抗饱和、`reset()`。参数按控制器实例保存，禁止用一组全局变量共用。

## 9. 丢线与找回

首先区分“预期无黑线”和“意外丢线”：`SEG_OPEN_TO_LINE` 中全白是正常状态；只有 `SEG_FOLLOW_*` 才启动丢线恢复。

```c
typedef enum {
    LINE_TRACKED,
    LINE_LOST_SUSPECT,
    LINE_PREDICT,
    LINE_SEARCH,
    LINE_REACQUIRE,
    LINE_RECOVERY_FAILED
} RecoveryState;
```

推荐策略：

1. **疑似丢线 0~30 ms**：保持上一帧转向，使用陀螺角速度阻尼，不立刻大转。
2. **短时预测 30~150 ms**：降速。直线按最后航向前进；圆弧按丢线前的平均转向量/角速度继续走已知半径方向。
3. **搜索**：依据最后一次 `lineError` 的符号先向丢线侧做前进圆弧，再做逐渐扩大的 S 形摆扫。两轮始终非负。
4. **重新捕获**：置信度连续 3~5 帧有效且质心没有跳到相反极端，才认为找回；用 100~200 ms 混合从搜索输出过渡回循线 PD。
5. **失败保护**：超过约 1.5~2 s、Yaw 偏离过大或接近任务超时，停车并进入 FAULT。

半圆半径已知，圆弧丢线时“保持丢线前曲率”通常比立即左右乱扫更可靠。最后一侧信息、段的转向方向、段起始 Yaw 都应保存在 `RecoveryContext` 中。

## 10. 点位识别

场地没有 A/B/C/D 字符或额外标记，点位只能由路线阶段和运动特征推断：

| 点位场景 | 主判据 | 辅助判据 |
|---|---|---|
| 第 1/2 题 B、第二题 D | 直线转圆弧的持续曲率出现 | 最短时间、同向线误差、陀螺角速度 |
| 半圆终点 | 段内解缠 Yaw 累计接近 180° | 曲率回落、最短时间、线仍有效 |
| 第 3/4 题 C、D | 开放段后重新捕获黑线 | 最短时间、航向误差、连续有效帧 |
| 第 3/4 题 B、A | 半圆累计转角接近 180° | 当前段身份、曲率和置信度 |

陀螺仪原始 Yaw 为 -180°~180°，必须在 `gyro.c` 内做解缠得到连续角度；任务层不能直接用 `target-current` 后只比较绝对值。点位事件采用“进入条件 + 滞回 + 一次性 latch”。

## 11. 陀螺仪驱动注意点

- UART ISR 使用逐字节状态机：找 `0x5A`、收满 5 字节、验和、再发布完整快照。
- `int16_t raw = (int16_t)(((uint16_t)high << 8) | low);`，避免有符号移位问题。
- 角速度换算按手册：`raw / 32768 * 2000 deg/s`；Yaw：`raw / 32768 * 180 deg`。
- 每份快照记录接收时间。超过例如 50~100 ms 未更新，禁止继续依赖航向闭环并进入降速/故障策略。
- 上电静止后再做 Yaw 归零；20 s 零偏校准放到赛前显式操作，不要每次启动都自动做。
- 手册波特率表有重复编码排版问题，先保持默认 9600 bps 做通，再根据实测和模块回读升级到更高波特率/输出率。

## 12. 当前代码应优先修改的地方

1. `main.c` 的阻塞式 `delay_cycles(1 ms)` 轮询改为硬件定时节拍和协作式调度。
2. `motor.c` 增加 STBY、只前进接口、PWM 斜坡和统一限幅；任务代码不再直接调用 `Motor_run(...REVERSE...)`。
3. 如果使用板载 PB22~PB24，`buttons.c` 改成上拉、低有效。
4. 当前文档写的是 80 MHz，但现有示例生成的 `ti_msp_dl_config.h` 显示 `CPUCLK_FREQ=32000000`；最终工程必须统一时钟，所有定时周期使用 SysConfig 生成的 `CPUCLK_FREQ`，不要在业务代码重复硬编码 80 MHz。
5. 预留 UART 的 PB4 与当前 AIN1 冲突。陀螺仪优先使用开发板 PA10/PA11 那组 UART（模块 TX->MCU RX，模块 RX->MCU TX），并在 SysConfig 中最终验证外设复用。
6. 现有陀螺仪示例可借鉴帧格式，但其解析器每 5 字节强制清零，抗错位能力一般；正式驱动应在错误时重新搜索帧头，并增加时间戳和原子快照。

## 13. 推荐实现和调试顺序

1. 只接 MCU、灰度和串口：完成八路 raw/归一化数据打印及黑白标定。
2. 架空车轮：验证 TB6612 方向、STBY、两路 PWM、急停，确保不会输出反转。
3. 静止/手持旋转：验证陀螺帧校验、Yaw 解缠、角速度符号和超时。
4. 低速完成直线循迹，只调循迹 P，再加 D。
5. 低速完成单个 40 cm 半圆，加入弯道降速和点位 180° 判定。
6. 完成第 1 题并标定 B 点停车补偿。
7. 完成第 2 题整圈和四点提示。
8. 单独调 A->C 开放段：先只验证航向保持和 C 重新捕获，不接后续圆弧。
9. 调 `JOIN_LINE` 的 38.66° 前进转弯，再串起第 3 题。
10. 最后启用第 4 题圈数逻辑、提速、调恢复和超时。

每一步都记录 `state, segment, raw[8], lineError, confidence, yaw, gyroZ, leftPwm, rightPwm`。先保证低速可重复，再提高速度；不要同时改机械安装高度、PID、速度和点位阈值。

## 14. 第一版验收标准

- 任意状态按停止键，20 ms 内 PWM 归零并关闭 STBY。
- 灰度扫描无半帧读取，8 路黑白归一化方向一致。
- 陀螺数据错一字节后能自动重新同步，超时可检测。
- APP、任务段和丢线恢复三个状态机可独立打印和复位。
- 切题只换 `RouteDef`，不复制控制代码。
- 所有可调速度、阈值、PID、超时都集中在参数结构中。
- 任务运行期间左右轮命令始终大于等于 0，不出现反转。
- 第 3/4 题开放段的“正常全白”不会触发丢线搜索。
