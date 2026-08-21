`timescale 1ns / 1ps
//*************************************************************************
//   > 文件名: multiply.v
//   > 描述  : 乘法器流水线模块，实现 Booth 风格的移位累加，支持同步乘法操作。
//   > 作者  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module multiply(
    input         clk,
    input         mult_begin,
    input  [31:0] mult_op1,
    input  [31:0] mult_op2,
    output [63:0] product,
    output        mult_end
);
    // mult_valid 为乘法运行状态：被拉高后在部分积完全移完时自动结束
    reg mult_valid;
    assign mult_end = mult_valid & ~(|multiplier);
    always @(posedge clk)
    begin
        if (!mult_begin || mult_end)
        begin
            mult_valid <= 1'b0;
        end
        else
        begin
            mult_valid <= 1'b1;
        end
    end
    // 取出两个操作数的符号并转换为绝对值，最后再处理符号还原
    wire        op1_sign;
    wire        op2_sign;
    wire [31:0] op1_absolute;
    wire [31:0] op2_absolute;
    assign op1_sign = mult_op1[31];
    assign op2_sign = mult_op2[31];
    assign op1_absolute = op1_sign ? (~mult_op1+1) : mult_op1;
    assign op2_absolute = op2_sign ? (~mult_op2+1) : mult_op2;
    // 被乘数移位寄存器：每个周期左移一位，准备下一次部分积
    reg  [63:0] multiplicand;
    always @ (posedge clk)
    begin
        if (mult_valid)
        begin
            multiplicand <= {multiplicand[62:0],1'b0};
        end
        else if (mult_begin)
        begin
            multiplicand <= {32'd0,op1_absolute};
        end
    end
    // 乘数移位寄存器：右移并输出最低位作为部分积选择
    reg  [31:0] multiplier;
    always @ (posedge clk)
    begin
        if (mult_valid)
        begin
            multiplier <= {1'b0,multiplier[31:1]};
        end
        else if (mult_begin)
        begin
            multiplier <= op2_absolute;
        end
    end
    // 若当前最低位为 1，则部分积等于当前被乘数，否则为 0
    wire [63:0] partial_product;
    assign partial_product = multiplier[0] ? multiplicand : 64'd0;
    // 积累寄存器：逐拍累加部分积，mult_begin 置 0 以启动新一轮计算
    reg [63:0] product_temp;
    always @ (posedge clk)
    begin
        if (mult_valid)
        begin
            product_temp <= product_temp + partial_product;
        end
        else if (mult_begin)
        begin
            product_temp <= 64'd0;
        end
    end
    // 最后根据原始符号位确定乘积符号，匹配有符号乘法行为
    reg product_sign;
    always @ (posedge clk)
    begin
        if (mult_valid)
        begin
              product_sign <= op1_sign ^ op2_sign;
        end
    end
    assign product = product_sign ? (~product_temp+1) : product_temp;
endmodule
