`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�?: adder.v
//   > 描述  : 五级流水 CPU 中复用的 32 位可控加法器，既能执�? A+B，也可�?�过 cin 实现减法/带进位加法�??
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module adder(
    input  [31:0] operand1,
    input  [31:0] operand2,
    input         cin,
    output [31:0] result,
    output        cout
    );
    // 组合加法：Verilog 会先计算 33 位的求和结果，再拆分出最高位进位 cout �? 32 位结�? result
    assign {cout,result} = operand1 + operand2 + cin;

endmodule
