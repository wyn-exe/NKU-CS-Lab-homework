`timescale 1ns / 1ps
//*************************************************************************
//   > 文件名: fetch.v
//   > 描述  : 流水线取指级，负责维护 PC、处理分支与异常入口并驱动指令存储器。
//   > 作者  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
`define STARTADDR 32'H00000034
module fetch(
    input             clk,
    input             resetn,
    input             IF_valid,
    input             next_fetch,
    input      [31:0] inst,
    input      [32:0] jbr_bus,
    output     [31:0] inst_addr,
    output reg        IF_over,
    output     [63:0] IF_ID_bus,
    input      [32:0] exc_bus,
    output     [31:0] IF_pc,
    output     [31:0] IF_inst
);
    // =========================================================================
    // IF 级关键寄存器：当前 PC 与下一拍预测值。
    // =========================================================================
    wire [31:0] next_pc;
    wire [31:0] seq_pc;
    reg  [31:0] pc;
    // 分支/跳转结果来自译码级，通过总线带回是否命中及目标地址
    wire        jbr_taken;
    wire [31:0] jbr_target;
    assign {jbr_taken, jbr_target} = jbr_bus;
    wire        exc_valid;
    wire [31:0] exc_pc;
    assign {exc_valid,exc_pc} = exc_bus;
    // 顺序 PC = pc + 4，使用位拼接避免溢出到低 2 位
    assign seq_pc[31:2]    = pc[31:2] + 1'b1;
    assign seq_pc[1:0]     = pc[1:0];
    // 下一 PC 选择优先级：异常 > 分支跳转 > 顺序执行
    assign next_pc = exc_valid ? exc_pc :
                     jbr_taken ? jbr_target : seq_pc;
    // PC 寄存器更新：复位跳到起始地址，流水线允许时写入 next_pc
    always @(posedge clk)
    begin
        if (!resetn)
        begin
            pc <= `STARTADDR;
        end
        else if (next_fetch)
        begin
            pc <= next_pc;
        end
    end
    assign inst_addr = pc;
    // IF_over 表示本拍取指是否完成，用于与下游握手保持同步
    always @(posedge clk)
    begin
        if (!resetn || next_fetch)
        begin
            IF_over <= 1'b0;
        end
        else
        begin
            IF_over <= IF_valid;
        end
    end
    // 将 PC 与指令打包写入 IF/ID 流水寄存器
    assign IF_ID_bus = {pc, inst};
    // 观测接口：对外暴露取指后的 PC 与指令数据
    assign IF_pc   = pc;
    assign IF_inst = inst;
endmodule
