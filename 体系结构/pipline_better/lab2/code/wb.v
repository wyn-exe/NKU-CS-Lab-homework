`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�?: wb.v
//   > 描述  : 五级流水 CPU 的写回级，同时处�? HI/LO、CP0 访问与异常入口�??
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
`define EXC_ENTER_ADDR 32'd0
module wb(
    input          WB_valid,
    input  [117:0] MEM_WB_bus_r,
    output         rf_wen,
    output [  4:0] rf_wdest,
    output [ 31:0] rf_wdata,
    output         WB_over,
     input             clk,
    input             resetn,
     output [ 32:0] exc_bus,
     output [  4:0] WB_wdest,
     output         cancel,
     output [ 31:0] WB_pc,
     output [ 31:0] HI_data,
     output [ 31:0] LO_data
);
    // =========================================================================
    // MEM->WB 总线拆包：取出访存结果�?�HI/LO 状�?�以�? CP0 控制信息�?
    // =========================================================================
    wire [31:0] mem_result;
    wire [31:0] lo_result;
    wire        hi_write;
    wire        lo_write;
    wire wen;
    wire [4:0] wdest;
    wire mfhi;
    wire mflo;
    wire mtc0;
    wire mfc0;
    wire [7 :0] cp0r_addr;
    wire       syscall;
    wire       eret;
    wire [31:0] pc;
    assign {wen,
            wdest,
            mem_result,
            lo_result,
            hi_write,
            lo_write,
            mfhi,
            mflo,
            mtc0,
            mfc0,
            cp0r_addr,
            syscall,
            eret,
            pc} = MEM_WB_bus_r;
    // HI/LO 寄存器：乘法与移动指令在写回级更新可见状�?
    reg [31:0] hi;
    reg [31:0] lo;
    always @(posedge clk)
    begin
        if (hi_write)
        begin
            hi <= mem_result;
        end
    end
    always @(posedge clk)
    begin
        if (lo_write)
        begin
            lo <= lo_result;
        end
    end
   // CP0 STATUS/CAUSE/EPC 的内部实现，仅覆盖实验所�?寄存�?
   wire [31:0] cp0r_status;
   wire [31:0] cp0r_cause;
   wire [31:0] cp0r_epc;
   wire status_wen;
   wire epc_wen;
   // mtc0 针对 STATUS/EPC 的写入判定，CAUSE 仅由异常更新
   assign status_wen = mtc0 & (cp0r_addr=={5'd12,3'd0});
   assign epc_wen    = mtc0 & (cp0r_addr=={5'd14,3'd0});
   wire [31:0] cp0r_rdata;
   assign cp0r_rdata = (cp0r_addr=={5'd12,3'd0}) ? cp0r_status :
                       (cp0r_addr=={5'd13,3'd0}) ? cp0r_cause  :
                       (cp0r_addr=={5'd14,3'd0}) ? cp0r_epc : 32'd0;
   // STATUS.EXL：syscall 置位，eret/复位清零，限制异常嵌�?
   reg status_exl_r;
   assign cp0r_status = {30'd0,status_exl_r,1'b0};
   always @(posedge clk)
   begin
       if (!resetn || eret)
       begin
           status_exl_r <= 1'b0;
       end
       else if (syscall)
       begin
           status_exl_r <= 1'b1;
       end
       else if (status_wen)
       begin
           status_exl_r <= mem_result[1];
       end
   end
   // CAUSE.ExcCode：当前仅实现 syscall，编�? 8
   reg [4:0] cause_exc_code_r;
   assign cp0r_cause = {25'd0,cause_exc_code_r,2'd0};
   always @(posedge clk)
   begin
       if (syscall)
       begin
           cause_exc_code_r <= 5'd8;
       end
   end
   // EPC：记录异常返回地�?，syscall 捕获当前 PC，mtc0 可覆�?
   reg [31:0] epc_r;
   assign cp0r_epc = epc_r;
   always @(posedge clk)
   begin
       if (syscall)
        begin
            epc_r <= pc;
        end
       else if (epc_wen)
       begin
           epc_r <= mem_result;
       end
   end
   // syscall/eret 会刷新前级流水线，需等当前写回完成后再发取消信号
   assign cancel = (syscall | eret) & WB_over;
    // 写回级无多拍操作，WB_valid 为真即可视为完成
    assign WB_over = WB_valid;
    assign rf_wen   = wen & WB_over;
    assign rf_wdest = wdest;
    // 写回数据优先级：HI/LO > CP0 > 普�?�访存结�?
    assign rf_wdata = mfhi ? hi :
                      mflo ? lo :
                      mfc0 ? cp0r_rdata : mem_result;
    wire        exc_valid;
    wire [31:0] exc_pc;
    // 生成异常入口：syscall 跳到固定入口，eret 返回 EPC
    assign exc_valid = (syscall | eret) & WB_valid;
    assign exc_pc = syscall ? `EXC_ENTER_ADDR : cp0r_epc;
    assign exc_bus = {exc_valid,exc_pc};
    // 冒险反馈：仅在指令有效时对外公布目的寄存器号
    assign WB_wdest = rf_wdest & {5{WB_valid}};
    // 调试输出：写回阶段的 PC 以及 HI/LO 当前�?
    assign WB_pc = pc;
    assign HI_data = hi;
    assign LO_data = lo;
endmodule
