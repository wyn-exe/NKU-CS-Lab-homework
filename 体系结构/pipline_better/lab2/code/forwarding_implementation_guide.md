# 译码级旁路前递实现详解

旁路前递（forwarding）看似只是给流水线多铺几条“捷径”，实则连着执行级、访存级、写回级的运算结果与握手信号。实验自带的基础工程只依赖寄存器堆同步写回，任何一条算术指令都要等到写回级落地之后译码级才能看到寄存器的新值，哪怕是距离最近的两条 `add` 也会被迫插入气泡。为了解除这一瓶颈，我们让译码级可以直接引用执行级、访存级甚至写回级即将写回的结果，并在每个阶段配套一位“结果已就绪”的标志位。下文按照模块顺序详细说明实现过程，并结合实际代码片段解释每一个判断条件的作用。

## 一、数据来源与有效性约束

旁路遵循“谁离前端近，谁的结果越新鲜”的原则，因此译码级始终按照 EXE → MEM → WB 的优先级去挑选数据。每个阶段都要提供两部分信息：32 位的旁路数据与一位有效标志。当命中的阶段有效位为 1 时，译码级即可直接使用该阶段的结果；否则需要继续向后检查，若所有阶段都未准备好，则回退到寄存器堆原始读数并停住译码级，等待下一拍再次尝试。

## 二、执行级 `exe.v` 的新增端口与有效条件

执行级的改动集中在输出端口和有效标志的判断。我们新增了 `EXE_forward_data` 与 `EXE_forward_valid` 两个输出，其中 `EXE_forward_data` 直接复用 `exe_result`，而有效性由以下语句控制：

```verilog
assign EXE_forward_valid = EXE_valid & rf_wen
                         & ~inst_load
                         & ~mfc0
                         & (~multiply | mult_end);
```

`EXE_valid & rf_wen` 说明当前拍携带的指令会写通用寄存器；`~inst_load` 把同步 RAM 的结果排除在外；`~mfc0` 对应 CP0 读取的特殊时序——`mfc0` 在执行级只得到占位值，真正的读数要到写回级才出现；`(~multiply | mult_end)` 则确保乘法器在 `mult_end` 拉高之前不会提前对外宣称结果可用。`inst_load` 来自对 `mem_control` 的拆包，`mfc0` 与 `multiply` 等信号均由译码级一路传递过来，所以执行级只需组合现成信号即可。

## 三、访存级 `mem.v` 的同步输出

访存级增加了 `MEM_forward_data` 与 `MEM_forward_valid`。旁路数据统一取 `mem_result`，它在 load 时选用 RAM 的返回值，其余指令沿用执行级的运算结果。有效标志为：

```verilog
assign MEM_forward_valid = MEM_over & rf_wen & ~mfc0;
```

其中 `MEM_over` 意味着同步 RAM 的读数据已经被锁存，`rf_wen` 依旧限制在写通用寄存器的指令范围内，`~mfc0` 避免 CP0 读结果再次被提前获取。因为 load 指令原本就通过 `MEM_valid_r` 额外等待一拍，旁路逻辑只需判断 `MEM_over` 即可知道结果是否稳定。

## 四、译码级 `decode.v` 的数据选择与阻塞逻辑

译码级需要消费三路旁路数据：新增端口收集 `EXE_forward_data/valid`、`MEM_forward_data/valid`、`WB_forward_data/valid`，内部用 `rs_need/rt_need` 判断当前指令是否真正读取相应寄存器，再通过 `rs_match_exe`、`rs_match_mem`、`rs_match_wb` 等信号检测寄存器号是否匹配。最终的数据选择写成：

```verilog
assign rs_data = rs_match_exe && EXE_forward_valid ? EXE_forward_data :
                 rs_match_mem && MEM_forward_valid ? MEM_forward_data :
                 rs_match_wb  && WB_forward_valid  ? WB_forward_data  :
                                                    rs_value;
```

`rt_data` 完全对称。阻塞判定沿用同样的优先级：

```verilog
assign rs_wait = rs_match_exe ? ~EXE_forward_valid :
                 rs_match_mem ? ~MEM_forward_valid :
                 rs_match_wb  ? ~WB_forward_valid  : 1'b0;
```

这表示一旦匹配到的阶段有效位为 0，就暂停译码级并等待下一拍；一旦有效位拉高，则立即解除阻塞。所有原先依赖 `rs_value/rt_value` 的逻辑——包括跳转目标、比较结果、ALU 操作数以及 `store` 写数据——都换成了 `rs_data/rt_data`，确保旁路结果全程贯穿译码阶段。

## 五、顶层 `pipeline_cpu.v` 与写回级的配合

顶层只负责组织连线：声明 `EXE_forward_*`、`MEM_forward_*`，在实例化 `decode`、`exe`、`mem` 时补齐端口，并把写回级的 `rf_wdata`、`rf_wen` 直接送到译码级作为 WB 旁路数据与有效标志。写回级本身无需修改，因为它会在 `WB_over` 为真后才把 `rf_wen` 拉高，恰好符合“结果已经准备好”的定义。旁路不会绕过寄存器堆写口，写回仍按原计划写入寄存器堆，用于后续完全无关的指令读取。

## 六、`mfc0` 与 `eret` 的异常案例

在没有 `~mfc0` 保护之前，异常处理序列 `mfc0 -> addiu -> mtc0 -> eret` 会把 EPC 写成 `0x00000004`。原因是执行级在 `mfc0` 时只输出占位零值，译码级立刻把这个伪结果当成有效数据，导致 `addiu` 之后再写回 CP0 时地址被覆盖。将 `~mfc0` 纳入 `EXE_forward_valid` 与 `MEM_forward_valid` 之后，译码级会乖乖等待到写回级的 `rf_wdata` 真正携带 EPC，这样 `eret` 才能正确返回到 `0x00000108`。这个例子强调：判断旁路有效性时不仅要看“是否写寄存器”，还要理解该指令真正生成结果的时间点。

## 七、验证与调试建议

实现旁路后建议从三个方向验证：一是算术依赖链（如 `add` 紧跟 `sub`）的波形中不再出现多余气泡；二是 `lw` → 使用目的寄存器的指令依旧会暂停一拍，但停顿恰好发生在 `MEM_forward_valid` 拉高之前；三是分支与跳转指令在读取前一条指令结果时能直接命中旁路。调试时可以同时观察 `EXE_forward_valid`、`MEM_forward_valid`、`WB_forward_valid` 和 `rs_wait/rt_wait`，若某阶段 valid 已经拉高但译码级仍在等待，说明寄存器匹配或需求判定可能有误。处理 CP0 相关场景时，还应关注写回级的 `cp0r_rdata` 与 EPC，确认译码级确实等到写回阶段才读取。

## 八、总结

旁路前递让五级流水 CPU 真正释放出性能潜力。通过让执行级、访存级、写回级在计算完成的第一时间把结果“递”回译码级，同时精准地控制哪些指令可以提前使用结果、哪些必须等待，我们既保留了 load、乘法、CP0 访问等指令固有的延迟，又最大限度地减少了算术类依赖造成的停顿。理解这些有效条件背后的时序语义，是今后扩展指令集或增加外设时的基础——只要弄清楚一条指令在哪一拍产出数据、如何确认它稳定，就能快速判断是否需要为它定制旁路策略。
