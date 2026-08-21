# 简化五级流水 CPU 代码导读

本文档结合 `code/` 目录下的源文件，对实验用五级流水 CPU 的整体结构、分层模块以及配套测试环境进行梳理，帮助快速理解每个文件的职责和它们之间的协作关系。

## 1. 顶层结构概览

- **核心顶层 `pipeline_cpu.v`**：负责串接 IF/ID/EXE/MEM/WB 五个流水级、维护 valid/allow_in 握手机制，并实例化指令/数据存储器及寄存器堆。文件中按阶段划分的总线（`IF_ID_bus`、`ID_EXE_bus` 等）将控制信号与数据打包，在流水寄存器中逐级传递。
- **显示封装 `pipeline_cpu_display.v`**：在 FPGA 上为 CPU 提供按键单步、LCD 显示与内存观察功能。通过 BUFGCE 控制 CPU 时钟，配合 `lcd_module` 实现实时信息展示。
- **约束文件 `pipeline_cpu.xdc`**：给出时钟、复位、按键和 LCD 相关 IO 的管脚绑定及电平标准，确保硬件连接一致。

整体架构如下图所示（逻辑示意）：

```
指令 ROM --> IF --> ID --> EXE --> MEM --> WB --> 寄存器堆
                           |            |          |
                        乘法器        数据 RAM   CP0/HI/LO
```

握手机制通过 `*_valid`、`*_over` 与 `*_allow_in` 信号协同工作：若下一级尚未准备好，则当前级保持 `valid`，阻止新指令流入；若发生 `syscall/eret`，写回级会拉高 `cancel`，各级在下一个时钟清空有效位。

## 2. 分阶段模块解析

### 2.1 取指级（IF）—— `fetch.v`

- 维护 PC 寄存器，支持顺序执行 `seq_pc = pc + 4`、分支/跳转重定向以及异常入口覆盖。
- 当 `next_fetch` 为真时锁存下一拍 PC，同时将 `{pc, inst}` 打包到 `IF_ID_bus`。
- `IF_over` 表示本拍取指完成，用于与 ID 级同步，使流水线在分支或暂停时保持一致。

### 2.2 译码级（ID）—— `decode.v`

- 解析 `IF_ID_bus_r` 中的指令字段，生成各类指令独热码（算数逻辑、访存、跳转、CP0 等）。
- 根据指令类别整理 ALU 控制、访存控制、寄存器写回目标等，并封装到 `ID_EXE_bus`。
- 通过 `inst_no_rs/inst_no_rt`、`EXE/MEM/WB_wdest` 检测 RAW 冒险，必要时阻塞 ID 级，或等待 `IF_over` 解决延迟槽问题。
- 生成 `jbr_bus`，当分支命中或跳转时通知 IF 级更新 PC。

### 2.3 执行级（EXE）—— `exe.v`

- 解包 `ID_EXE_bus_r`，选择 ALU 操作数并发起组合运算；特殊指令 `MTHI/MTLO/MULT/CP0` 会重定向数据路径。
- 对 MULT 指令使用 `multiply.v` 时序乘法器，`mult_begin/mult_end` 起握手作用，确保 WB 级仅在乘积有效时继续。
- 将 HI/LO 写入请求、访存控制和写回信息重新打包发送给 MEM 级。

### 2.4 访存级（MEM）—— `mem.v`

- 根据 `mem_control`决定 load/store 行为：SB 通过字节写使能完成对齐，LW 直接写四个字节。
- load 数据按地址低两位对齐，并依据 `lb_sign` 做符号或零扩展。
- 对同步 RAM 的读延迟使用 `MEM_valid_r` 进行一拍缓存，保证 `MEM_over` 在 load 时延后返回。
- 输出 `mem_result`（load 数据或 EXE 结果）、HI/LO 写信号及 CP0 控制，封装成 `MEM_WB_bus`。

### 2.5 写回级（WB）—— `wb.v`

- 维护 HI/LO 寄存器，并实现简化版 CP0（STATUS、CAUSE、EPC）。`syscall` 时写入 EPC/CAUSE，`eret` 取 EPC 返回。
- 根据优先级选择写回数据：HI/LO → CP0 → 普通寄存器。
- 生成 `exc_bus`，当 syscall/eret 发生时通知 IF 切换到异常入口或返回地址；同时通过 `cancel` 清空前级有效位。
- 将写回目的寄存器回馈给前级冒险检测（`WB_wdest`）。

### 2.6 各流水阶段时序概览

| 流水级 | 常规耗时 | 额外停顿条件 | 本拍（或停顿期间）执行的核心操作 |
| --- | --- | --- | --- |
| IF | 1 拍 | `IF_allow_in` 为 0 时暂停；异常/跳转重定向时需等 `cancel`/`jbr_bus` | 更新 PC（顺序/分支/异常），向 `inst_rom` 取指，将 `{pc, inst}` 写入 `IF_ID_bus` |
| ID | 1 拍 | 检测到 `rs/rt` 与 EXE/MEM/WB 级目的寄存器冲突时保持原指令 | 解析指令字段，生成控制信号，计算分支/跳转目标，封装 `ID_EXE_bus` |
| EXE | 1 拍 | `multiply` 指令需多拍：等待 `mult_end` 才能置 `EXE_over` | 触发 ALU 运算，或驱动乘法器/HI/LO/CP0 操作，将结果和控制信息写入 `EXE_MEM_bus` |
| MEM | 1 拍（store）/2 拍（load） | 同步 RAM 读操作：load 指令需等待下一拍数据返回 | store：生成字节写使能与写数据；load：对齐并扩展返回数据，封装 `MEM_WB_bus` |
| WB | 1 拍 | `syscall`/`eret` 会在本拍置位 `cancel` 并向前级清空 | 选择最终写回数据（HI/LO/CP0/寄存器），更新 HI/LO/CP0，生成 `exc_bus` 和 `WB_wdest` |

> 注：常规情况下，每个阶段都能在单拍内完成；若 EXE 遇到 `MULT`，或 MEM 遇到 load，流水线会在对应阶段停顿，直到结果有效后再继续推进。

## 3. 常用基础模块

- **`adder.v` / `alu.v`**：ALU 支持 12 类运算，内部共用加法器实现加/减/比较，并通过分级移位实现 SLL/SRL/SRA。
- **`multiply.v`**：移位累加式乘法器，先将操作数转为绝对值，再根据符号位补偿结果。
- **`regfile.v`**：32×32 位寄存器堆，双组合读、单同步写，同时提供调试读口 `test_addr/test_data`。
- **`pipeline_cpu_display.v`**：封装 CPU + LCD 显示逻辑，可单步控制、查看寄存器、流水线 PC、HI/LO、内存等信息。

## 4. 测试与验证

- **仿真平台 `tb.v`**：产生 10ns 周期时钟，先复位 100ns，再运行 1000ns。关键信号逐拍写入 `simulation_results1.txt`，便于离线分析。
- **按键显示**：`pipeline_cpu_display.v` 通过 `lcd_module` 收集 LCD 输入；当 `display_number` 在 13~44 之间时显示寄存器内容，否则根据编号显示流水级 PC、内存数据、valid 状态以及 HI/LO 寄存器。

## 5. 冒险与异常处理总结

- **数据冒险**：译码级通过比较 `rs/rt` 与 EXE/MEM/WB 级的目的寄存器号来判断 RAW 冒险。当目标仍在前级时，阻塞 ID 级，使 `IF_allow_in` 也被拉低，借由流水寄存器保持指令。
- **结构冒险**：数据 RAM 为同步读，MEM 级使用 `MEM_valid_r` 等待一拍，避免 WB 级错读旧数据。
- **控制冒险**：分支命中依赖 ID 级，并在 `jbr_bus` 中带回目标 PC；`syscall/eret` 由 WB 级统一取消前级指令，保障异常向量执行。

## 6. 文件到功能的映射表

| 文件 | 主要功能 |
| --- | --- |
| `pipeline_cpu.v` | 五级流水顶层、握手控制、模块实例化 |
| `fetch.v` | 取指级：PC 更新、异常/跳转处理 |
| `decode.v` | 译码级：指令解析、控制信号生成、冒险检测 |
| `exe.v` | 执行级：ALU 运算、乘法器、HI/LO/CP0 操作 |
| `mem.v` | 访存级：对齐、字节写入、load 扩展 |
| `wb.v` | 写回级：HI/LO、CP0、异常入口、写回选择 |
| `alu.v` / `adder.v` / `multiply.v` | 基础算术单元 |
| `regfile.v` | 32×32 寄存器堆 |
| `pipeline_cpu_display.v` | FPGA 外设封装、LCD 显示控制 |
| `pipeline_cpu.xdc` | FPGA 引脚与 IO 标准约束 |
| `tb.v` | 仿真测试平台 |

## 7. 设计要点回顾

1. **分层清晰**：每级模块只负责本级的控制与数据加工，通过总线结构向下游传递信号，避免跨级耦合。
2. **握手统一**：`valid/allow_in/over` 三元组在五级中保持一致模式，使得暂停、单步、异常都能自然地统一处理。
3. **调试友好**：借助 `pipeline_cpu_display.v` 和 `lcd_module`，可在 FPGA 上观察寄存器、内存及流水线状态；`tb.v` 也实现了仿真记录。
4. **扩展基础**：HI/LO 及 CP0 框架已经搭建，可在此基础上继续实现更多异常类型或指令。

通过阅读上述文件与注释，可以从顶层把握五级流水 CPU 的运作流程，再逐个深入每个阶段的细节，实现对实验代码的全面理解。
