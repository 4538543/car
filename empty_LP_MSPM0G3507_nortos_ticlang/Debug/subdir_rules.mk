################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/CCS_21.0.0.00014_win/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang" -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang/Debug" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1513806572: ../empty.syscfg
	@echo 'SysConfig - building file: "$<"'
	"D:/CCS_21.0.0.00014_win/ccs/utils/sysconfig_1.28.0/sysconfig_cli.bat" -s "D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/.metadata/product.json" --script "F:/car/empty_LP_MSPM0G3507_nortos_ticlang/empty.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-1513806572 ../empty.syscfg
device.opt: build-1513806572
device.cmd.genlibs: build-1513806572
ti_msp_dl_config.c: build-1513806572
ti_msp_dl_config.h: build-1513806572
Event.dot: build-1513806572

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/CCS_21.0.0.00014_win/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang" -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang/Debug" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/CCS_21.0.0.00014_win/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang" -I"F:/car/empty_LP_MSPM0G3507_nortos_ticlang/Debug" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"D:/CCS_21.0.0.00014_win/CCS_21.0.0.00014_win/mspm0_sdk_2_11_00_07/source" -g -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


