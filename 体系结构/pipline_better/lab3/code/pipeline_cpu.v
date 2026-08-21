`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�???: pipeline_cpu.v
//   > 描述  : 五级流水 CPU 顶层，整合各级流水寄存器、冒险控制以及与外设交互�???
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module pipeline_cpu(
    input clk,
    input resetn,
    input [5:0] int_in,
    input  [ 4:0] rf_addr,
    input  [31:0] mem_addr,
    output [31:0] rf_data,
    output [31:0] mem_data,
    output [31:0] IF_pc,
    output [31:0] IF_inst,
    output [31:0] ID_pc,
    output [31:0] EXE_pc,
    output [31:0] MEM_pc,
    output [31:0] WB_pc,
    output [31:0] cpu_5_valid,
    output [31:0] HI_data,
    output [31:0] LO_data
    );
    // =========================================================================
    // 5 级流水有效标志寄存器：用于宏观控制级间流动�??
    // =========================================================================
    reg IF_valid;
    reg ID_valid;
    reg EXE_valid;
    reg MEM_valid;
    reg WB_valid;
    wire IF_over;
    wire ID_over;
    wire EXE_over;
    wire MEM_over;
    wire WB_over;
    wire IF_allow_in;
    wire ID_allow_in;
    wire EXE_allow_in;
    wire MEM_allow_in;
    wire WB_allow_in;
    wire cancel;
    wire [31:0] EXE_forward_data;
    wire        EXE_forward_valid;
    wire [31:0] MEM_forward_data;
    wire        MEM_forward_valid;
    wire        WB_forward_valid;
    
    // 逐级握手：若上一级完成且下一级允许或 cancel 触发，则可以流入下一�???
    assign IF_allow_in  = (IF_over & ID_allow_in) | cancel;
    // allow_in = 下级可接�??? || 当前级为空，实现流水线冒险暂停机�???
    assign ID_allow_in  = ~ID_valid  | (ID_over  & EXE_allow_in);
    assign EXE_allow_in = ~EXE_valid | (EXE_over & MEM_allow_in);
    assign MEM_allow_in = ~MEM_valid | (MEM_over & WB_allow_in );
    assign WB_allow_in  = ~WB_valid  | WB_over;
   // 级间 valid 更新：复位或异常取消时清零，否则由上�???阶段 over 驱动
   always @(posedge clk)
    begin
        if (!resetn)
        begin
            IF_valid <= 1'b0;
        end
        else
        begin
            IF_valid <= 1'b1;
        end
    end
    always @(posedge clk)
    begin
        if (!resetn || cancel)
        begin
            ID_valid <= 1'b0;
        end
        else if (ID_allow_in)
        begin
            ID_valid <= IF_over;
        end
    end
    always @(posedge clk)
    begin
        if (!resetn || cancel)
        begin
            EXE_valid <= 1'b0;
        end
        else if (EXE_allow_in)
        begin
            EXE_valid <= ID_over;
        end
    end
    always @(posedge clk)
    begin
        if (!resetn || cancel)
        begin
            MEM_valid <= 1'b0;
        end
        else if (MEM_allow_in)
        begin
            MEM_valid <= EXE_over;
        end
    end
    always @(posedge clk)
    begin
        if (!resetn || cancel)
        begin
            WB_valid <= 1'b0;
        end
        else if (WB_allow_in)
        begin
            WB_valid <= MEM_over;
        end
    end
    // 调试可视化：将五�??? valid 打包�??? 5×4bit，便于灯�???/显示输出
    assign cpu_5_valid = {12'd0         ,{4{IF_valid }},{4{ID_valid}},
                          {4{EXE_valid}},{4{MEM_valid}},{4{WB_valid}}};
    // 各级间的流水线寄存器总线定义
    wire [ 63:0] IF_ID_bus;
    wire [206:0] ID_EXE_bus;
    wire [190:0] EXE_MEM_bus;
    wire [154:0] MEM_WB_bus;
    reg [ 63:0] IF_ID_bus_r;
    reg [206:0] ID_EXE_bus_r;
    reg [190:0] EXE_MEM_bus_r;
    reg [154:0] MEM_WB_bus_r;
    // 各级流水线寄存器：当上一级完成且下一级允许时锁存总线数据
    always @(posedge clk)
    begin
        if(IF_over && ID_allow_in)
        begin
            IF_ID_bus_r <= IF_ID_bus;
        end
    end
    always @(posedge clk)
    begin
        if(ID_over && EXE_allow_in)
        begin
            ID_EXE_bus_r <= ID_EXE_bus;
        end
    end
    always @(posedge clk)
    begin
        if(EXE_over && MEM_allow_in)
        begin
            EXE_MEM_bus_r <= EXE_MEM_bus;
        end
    end
    always @(posedge clk)
    begin
        if(MEM_over && WB_allow_in)
        begin
            MEM_WB_bus_r <= MEM_WB_bus;
        end
    end
    // 阶段间连线：跳转、寄存器地址、数据存储器、写回信号等
    wire [ 32:0] jbr_bus;
    wire [31:0] inst_addr;
    wire [31:0] inst;
    wire [ 4:0] EXE_wdest;
    wire [ 4:0] MEM_wdest;
    wire [ 4:0] WB_wdest;
    wire [ 3:0] dm_wen;
    wire [31:0] dm_addr;
    wire [31:0] dm_wdata;
    wire [31:0] dm_rdata;
    wire [ 4:0] rs;
    wire [ 4:0] rt;
    wire [31:0] rs_value;
    wire [31:0] rt_value;
    wire        rf_wen;
    wire [ 4:0] rf_wdest;
    wire [31:0] rf_wdata;
    wire [32:0] exc_bus;
    wire next_fetch;
    
    assign WB_forward_valid = rf_wen;
    // IF 阶段是否可以推进：与 allow_in 相同，取消冒险时可拉�???
    assign next_fetch = IF_allow_in;
    // -------------------- IF 取指级实�??? --------------------
    fetch IF_module(
        .clk       (clk       ),
        .resetn    (resetn    ),
        .IF_valid  (IF_valid  ),
        .next_fetch(next_fetch),
        .inst      (inst      ),
        .jbr_bus   (jbr_bus   ),
        .inst_addr (inst_addr ),
        .IF_over   (IF_over   ),
        .IF_ID_bus (IF_ID_bus ),
        .exc_bus   (exc_bus   ),
        .IF_pc     (IF_pc     ),
        .IF_inst   (IF_inst   )
    );
    // -------------------- ID 译码级实�??? --------------------
    decode ID_module(
        .ID_valid   (ID_valid   ),
        .IF_ID_bus_r(IF_ID_bus_r),
        .rs_value   (rs_value   ),
        .rt_value   (rt_value   ),
        .rs         (rs         ),
        .rt         (rt         ),
        .jbr_bus    (jbr_bus    ),
        .ID_over    (ID_over    ),
        .ID_EXE_bus (ID_EXE_bus ),
        .IF_over     (IF_over     ),
        .EXE_wdest   (EXE_wdest   ),
        .MEM_wdest   (MEM_wdest   ),
        .WB_wdest    (WB_wdest    ),
        .ID_pc       (ID_pc       ),
        .EXE_forward_data (EXE_forward_data ),
        .EXE_forward_valid(EXE_forward_valid),
        .MEM_forward_data (MEM_forward_data ),
        .MEM_forward_valid(MEM_forward_valid),
        .WB_forward_data  (rf_wdata        ),
        .WB_forward_valid (WB_forward_valid)
    );
    // -------------------- EXE 执行级实�??? -------------------
    exe EXE_module(
        .EXE_valid   (EXE_valid   ),
        .ID_EXE_bus_r(ID_EXE_bus_r),
        .EXE_over    (EXE_over    ),
        .EXE_MEM_bus (EXE_MEM_bus ),
        .clk         (clk         ),
        .EXE_wdest   (EXE_wdest   ),
        .EXE_pc      (EXE_pc      ),
        .EXE_forward_data (EXE_forward_data ),
        .EXE_forward_valid(EXE_forward_valid)
    );
    // -------------------- MEM 访存级实�??? -------------------
    mem MEM_module(
        .clk          (clk          ),
        .MEM_valid    (MEM_valid    ),
        .EXE_MEM_bus_r(EXE_MEM_bus_r),
        .dm_rdata     (dm_rdata     ),
        .dm_addr      (dm_addr      ),
        .dm_wen       (dm_wen       ),
        .dm_wdata     (dm_wdata     ),
        .MEM_over     (MEM_over     ),
        .MEM_WB_bus   (MEM_WB_bus   ),
        .MEM_allow_in (MEM_allow_in ),
        .MEM_wdest    (MEM_wdest    ),
        .MEM_pc       (MEM_pc       ),
        .MEM_forward_data (MEM_forward_data ),
        .MEM_forward_valid(MEM_forward_valid)
    );
    // -------------------- WB 写回级实�??? --------------------
    wb WB_module(
        .WB_valid    (WB_valid    ),
        .MEM_WB_bus_r(MEM_WB_bus_r),
        .rf_wen      (rf_wen      ),
        .rf_wdest    (rf_wdest    ),
        .rf_wdata    (rf_wdata    ),
        .WB_over     (WB_over     ),
        .clk         (clk         ),
        .resetn      (resetn      ),
        .int_in      (int_in      ),
        .exc_bus     (exc_bus     ),
        .WB_wdest    (WB_wdest    ),
        .cancel      (cancel      ),
        .WB_pc       (WB_pc       ),
        .HI_data     (HI_data     ),
        .LO_data     (LO_data     )
    );
    // 指令存储器（ROM IP 核）
    inst_rom inst_rom_module(
        .clka       (clk           ),
        .addra      (inst_addr[9:2]),
        .douta      (inst          )
    );
    // 通用寄存器堆，双读单写并附带调试端口
    regfile rf_module(
        .clk    (clk      ),
        .wen    (rf_wen   ),
        .raddr1 (rs       ),
        .raddr2 (rt       ),
        .waddr  (rf_wdest ),
        .wdata  (rf_wdata ),
        .rdata1 (rs_value ),
        .rdata2 (rt_value ),
        .test_addr(rf_addr),
        .test_data(rf_data)
    );
    // 数据存储器（RAM IP 核），A 口供 CPU 访存，B 口用于调试观�???
    data_ram data_ram_module(
        .clka   (clk         ),
        .wea    (dm_wen      ),
        .addra  (dm_addr[9:2]),
        .dina   (dm_wdata    ),
        .douta  (dm_rdata    ),
        .clkb   (clk          ),
        .web    (4'd0         ),
        .addrb  (mem_addr[9:2]),
        .doutb  (mem_data     ),
        .dinb   (32'd0        )
    );
endmodule
