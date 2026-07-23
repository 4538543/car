# V1：反向制动与第三题 A→C

生成日期：2026-07-23

## 本版改动

- 第一、二、三题检测到终点黑线后，先给左右电机 110 ms、220‰ 的反向力，再关闭电机驱动，减小停车滑行距离。
- 新增独立的 `q3_mission.c/.h`，第三题参数和状态不会修改第一、二题的 PID 状态。
- PB23 只需按一次：先校正到开机绝对角度 0°，再原地 PID 转到 38°，稳定后保持 38°直行。
- 第三题直行期间，两路灰度检测到黑线并连续确认 20 ms 后，在 C 点反向制动并停车。
- C→B 循迹暂未实现；程序停在 C 点等待 `Q3Mission_continueFromB()`。
- 将来循迹模块到达 B 点后调用该接口，程序会自动校正 180°、转到 142°并继续 B→D，不需要第二次按 PB23。

## 可调参数

集中位于 `app_config.h`：

- `BRAKE_REVERSE_MS`：反向制动时间。
- `BRAKE_REVERSE_SPEED`：反向制动力。
- `Q3_AC_TARGET_DECI_DEG`：A→C 目标角度，380 表示 38.0°。
- `Q3_B_START_DECI_DEG`：B 点起始方向，1800 表示 180.0°。
- `Q3_BD_TARGET_DECI_DEG`：B→D 目标角度，1420 表示 142.0°。
- `TURN_SPEED`、`TURN_MIN_SPEED`、`TURN_TOLERANCE_DECI_DEG`：转弯限幅和到位容差。

## 文件

- `car_V1_source.zip`：本版工程源码。
- `car_V1_firmware.out`：已通过 TI ARM Clang 编译链接的固件。
