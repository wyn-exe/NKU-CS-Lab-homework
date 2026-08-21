# 五级流水线旁路示意图

下图展示当前五级流水线 CPU 中的旁路（前递）结构。可以看到，执行级（EXE）、访存级（MEM）以及写回级（WB）在结果就绪时，会将数据沿着专用旁路通道送回译码级（ID），译码级根据 EXE→MEM→WB 的优先级选择最新值，用于替换寄存器堆读出的旧数据。

```
                ┌──────────────────────────────────────────────────┐
                │                    顶层 pipeline_cpu              │
                │                                                  │
                │                        ┌─────────┐               │
   rs_value/rt_value ◄────────────┐      │ RegFile │      ┌─────┐  │
                │                 │      └─────────┘      │ WB  │  │
                │                 │           ▲            │     │  │
                │                 │     WB_forward_valid   │     │  │
                │   ┌─────────────▼──────┐    (=rf_wen)    │     │  │
                │   │      Decode        │◄──rf_wdata──────┘     │  │
                │   │  rs_data / rt_data │                      │  │
                │   └────────┬───────────┘                      │  │
                │            ▲ ▲ ▲                              │  │
                │            │ │ │                              │  │
                │            │ │ │                              │  │
                │      ┌─────┘ │ └────────┐                     │  │
                │      │       │          │                     │  │
                │  EXE_forward│data/valid │ MEM_forward_data/valid│ │
                │      │       │          │                     │  │
                │      ▼       ▼          ▼                     │  │
                │   ┌─────┐ ┌─────┐   ┌─────┐                   │  │
                │   │ EXE │ │ MEM │   │ WB  │(写回结果)          │  │
                │   └──┬──┘ └──┬──┘   └──┬──┘                   │  │
                │      │       │          │                      │  │
                │   exe_result │      mem_result                 │  │
                │              │                                 │  │
                │   (乘法握手 mult_end、同步 RAM 等条件          │  │
                │    决定对应的 forward_valid 是否为 1)          │  │
                └──────────────────────────────────────────────────┘
```

说明：

- **EXE_forward_data/valid**：来自执行级的组合运算结果（含 HI/LO、CP0 写数据），`valid` 由 `EXE_valid & rf_wen & ~inst_load & ~mfc0 & (~multiply | mult_end)` 控制。
- **MEM_forward_data/valid**：来自访存级的最终写回值（load 结果或执行级运算结果），`valid` 为 `MEM_over & rf_wen & ~mfc0`。
- **WB_forward_data/valid**：直接使用写回级的 `rf_wdata` 与 `rf_wen`。
- **Decode 侧优先级**：译码级按 EXE → MEM → WB 的顺序检查目标寄存器是否命中且 `valid`=1，一旦命中立即使用该数据，否则退回寄存器堆读值；当命中的阶段还未就绪时，会拉高 `rs_wait/rt_wait` 并暂停译码级。
