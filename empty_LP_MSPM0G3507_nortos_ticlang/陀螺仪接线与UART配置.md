# 单轴陀螺仪接线与 UART 配置

## 接线

| 陀螺仪 | MSPM0G3507 | 说明 |
|---|---|---|
| VCC | 5V | 手册允许 3.3~16V，典型供电为5V |
| GND | GND | 必须与MCU、电机驱动和电池负极共地 |
| TX | PA11 / UART0_RX | 模块发送接MCU接收 |
| RX | PA10 / UART0_TX | 模块接收接MCU发送 |

TX和RX必须交叉连接，不能TX接TX。模块应水平固定，Z轴垂直车体平面；远离电机电源线，并避免安装板松动。

## SysConfig

在左侧添加一个 `UART` 实例：

- Name：`GYRO_UART`
- Selected Peripheral：`UART0`
- UART Mode：Normal
- Direction：TX and RX
- UART Clock Source：MFCLK
- Clock Divider：Divide by 1
- Target Baud Rate：115200
- Word Length：8 Bits
- Parity：None
- Stop Bits：One
- Hardware Flow Control：None
- Oversampling：16x或Auto
- Enabled Interrupts：只勾选 RX
- Interrupt Priority：Default
- DMA：Disabled
- Event Publisher/Subscriber：Disabled
- RX FIFO：默认即可；若要求阈值，选择最低/1 byte
- TX FIFO：默认即可

PinMux：

- TX Pin：明确选择 PA10 / UART0_TX
- RX Pin：明确选择 PA11 / UART0_RX
- Internal Resistor：No Resistor
- Invert：Disabled
- 不要保留 `Any(...)`

手册参数页和配套MSPM0例程均使用115200。手册的寄存器说明页另有“默认值0x0002/9600”的矛盾文字；若按下面方法观察不到任何有效帧，可把Target Baud Rate临时改为9600再试，但第一选择仍为115200。

## 协议与代码

模块发送5字节二进制帧，不是ASCII：

- Z轴角速度：`5A AA DataL DataH Sum`
- Yaw角度：`5A BB DataL DataH Sum`
- `Sum` 为前4字节相加后的低8位

代码文件：`gyro.c/.h`。UART中断只接收、同步和校验帧，不在中断里运行PID。

公开接口：

- `Gyro_isOnline()`：500ms内收到过合法帧
- `Gyro_hasYaw()`：收到过Yaw帧
- `Gyro_hasRate()`：收到过角速度帧
- `Gyro_getYawDeg()`：返回 -180~180度
- `Gyro_getRateDps()`：返回Z轴角速度，单位度/秒
- `Gyro_zeroYawAndSave()`：解锁、Yaw归零并保存；程序不会自动调用

## 首次调试

保存SysConfig，右键工程 `Refresh`，再 `Clean Project` 和 `Build Project`。进入Debug并Resume，在Expressions中添加：

- `gGyroValidFrameCount`：应持续增加
- `gGyroChecksumErrorCount`：正常应保持0或偶尔增加
- `gGyroRawYaw`：旋转模块时变化
- `gGyroRawRate`：旋转速度变化时变化，静止时接近0
- `gGyroAgeMs`：持续通信时应反复回到0，并小于500

当前电机按键测试模式保持不变；陀螺仪数据暂时只采集，不直接修正左右轮PWM。
