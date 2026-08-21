`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�?: alu.v
//   > 描述  : 五级流水 CPU 执行级的算术逻辑单元，实�? 12 种基�?运算并保持纯组合逻辑输出�?
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module alu(
    input  [11:0] alu_control,  // 来自译码阶段的功能�?�择信号，每�? bit 对应�?种运�?
    input  [31:0] alu_src1,     // 运算源操作数 1，可是寄存器值�?�立即数�? PC
    input  [31:0] alu_src2,     // 运算源操作数 2，同样由译码阶段准备�?
    output [31:0] alu_result    // 组合逻辑输出，供后级流水段直接使�?
    );

    // -------------------------
    // 功能位译�?
    // -------------------------
    // 译码阶段�? ALU �?要执行的操作编码�? 12 位独热码，便于在执行级快速�?�择目标逻辑�?
    wire alu_add;   // 加法或�?�带符号减法（�?�过第二个操作数取反实现�?
    wire alu_sub;   // 减法操作，沿�? adder 进行有符号差值计�?
    wire alu_slt;   // 有符号比较，小于则结果为 1 �? SLT
    wire alu_sltu;  // 无符号比较，小于则结果为 1 �? SLTU
    wire alu_and;   // 按位�?
    wire alu_nor;   // 按位或非
    wire alu_or;    // 按位�?
    wire alu_xor;   // 按位异或
    wire alu_sll;   // 逻辑左移
    wire alu_srl;   // 逻辑右移
    wire alu_sra;   // 算术右移（保持符号位�?
    wire alu_lui;   // 装载立即数高 16 位（LUI�?

    assign alu_add  = alu_control[11];
    assign alu_sub  = alu_control[10];
    assign alu_slt  = alu_control[ 9];
    assign alu_sltu = alu_control[ 8];
    assign alu_and  = alu_control[ 7];
    assign alu_nor  = alu_control[ 6];
    assign alu_or   = alu_control[ 5];
    assign alu_xor  = alu_control[ 4];
    assign alu_sll  = alu_control[ 3];
    assign alu_srl  = alu_control[ 2];
    assign alu_sra  = alu_control[ 1];
    assign alu_lui  = alu_control[ 0];

    // -------------------------
    // 各类并行运算结果缓存
    // -------------------------
    wire [31:0] add_sub_result;
    wire [31:0] slt_result;
    wire [31:0] sltu_result;
    wire [31:0] and_result;
    wire [31:0] nor_result;
    wire [31:0] or_result;
    wire [31:0] xor_result;
    wire [31:0] sll_result;
    wire [31:0] srl_result;
    wire [31:0] sra_result;
    wire [31:0] lui_result;

    assign and_result = alu_src1 & alu_src2;
    assign or_result  = alu_src1 | alu_src2;
    assign nor_result = ~or_result;               // NOR 直接对按位或结果取反
    assign xor_result = alu_src1 ^ alu_src2;
    assign lui_result = {alu_src2[15:0], 16'd0};  // 立即数摆放在�? 16 位，低位�? 0

    // -------------------------
    // 共享加法器：用于 ADD/SUB/SLT/SLTU
    // -------------------------
    // 通过对第二个操作数取反�?�调�? cin 实现加减复用�?
    wire [31:0] adder_operand1;
    wire [31:0] adder_operand2;
    wire        adder_cin;
    wire [31:0] adder_result;
    wire        adder_cout;

    assign adder_operand1 = alu_src1;
    assign adder_operand2 = alu_add ? alu_src2 : ~alu_src2;  // SUB/SLT 时取反配�? cin 实现补码减法
    assign adder_cin      = ~alu_add;                        // SUB/SLT 给出 cin=1，构�? A + ~B + 1

    adder adder_module(
        .operand1(adder_operand1),
        .operand2(adder_operand2),
        .cin     (adder_cin     ),
        .result  (adder_result  ),
        .cout    (adder_cout    )
    );

    assign add_sub_result = adder_result;

    // SLT：有符号比较，为防止溢出直接分析符号位组合关�?
    assign slt_result[31:1] = 31'd0;
    assign slt_result[0]    = (alu_src1[31] & ~alu_src2[31]) |
                              (~(alu_src1[31] ^ alu_src2[31]) & adder_result[31]);

    // SLTU：无符号比较只关注进位位，cout=0 说明 src1<src2
    assign sltu_result = {31'd0, ~adder_cout};

    // -------------------------
    // Barrel Shifter：分级移位避免使用综合器黑盒资源
    // -------------------------
    wire [4:0] shf = alu_src1[4:0];    // 移位量统�?来自操作�? 1 的低 5 �?
    wire [1:0] shf_1_0 = shf[1:0];     // 第一层处�? 0~3 位移
    wire [1:0] shf_3_2 = shf[3:2];     // 第二层处�? 0/4/8/12 位移

    // 逻辑左移：分两级并在�?高位补零，最后若�? 5 位为 1 再整体左�? 16 �?
    wire [31:0] sll_step1;
    wire [31:0] sll_step2;
    assign sll_step1 = {32{shf_1_0 == 2'b00}} & alu_src2                   |
                       {32{shf_1_0 == 2'b01}} & {alu_src2[30:0], 1'd0}     |
                       {32{shf_1_0 == 2'b10}} & {alu_src2[29:0], 2'd0}     |
                       {32{shf_1_0 == 2'b11}} & {alu_src2[28:0], 3'd0};
    assign sll_step2 = {32{shf_3_2 == 2'b00}} & sll_step1                  |
                       {32{shf_3_2 == 2'b01}} & {sll_step1[27:0], 4'd0}    |
                       {32{shf_3_2 == 2'b10}} & {sll_step1[23:0], 8'd0}    |
                       {32{shf_3_2 == 2'b11}} & {sll_step1[19:0], 12'd0};
    assign sll_result = shf[4] ? {sll_step2[15:0], 16'd0} : sll_step2;

    // 逻辑右移：同样分级实现，空缺位全部填 0
    wire [31:0] srl_step1;
    wire [31:0] srl_step2;
    assign srl_step1 = {32{shf_1_0 == 2'b00}} & alu_src2                   |
                       {32{shf_1_0 == 2'b01}} & {1'd0, alu_src2[31:1]}     |
                       {32{shf_1_0 == 2'b10}} & {2'd0, alu_src2[31:2]}     |
                       {32{shf_1_0 == 2'b11}} & {3'd0, alu_src2[31:3]};
    assign srl_step2 = {32{shf_3_2 == 2'b00}} & srl_step1                  |
                       {32{shf_3_2 == 2'b01}} & {4'd0, srl_step1[31:4]}    |
                       {32{shf_3_2 == 2'b10}} & {8'd0, srl_step1[31:8]}    |
                       {32{shf_3_2 == 2'b11}} & {12'd0, srl_step1[31:12]};
    assign srl_result = shf[4] ? {16'd0, srl_step2[31:16]} : srl_step2;

    // 算术右移：与逻辑右移结构�?致，但空缺位使用原最高位符号扩展
    wire [31:0] sra_step1;
    wire [31:0] sra_step2;
    assign sra_step1 = {32{shf_1_0 == 2'b00}} & alu_src2                                 |
                       {32{shf_1_0 == 2'b01}} & {alu_src2[31], alu_src2[31:1]}           |
                       {32{shf_1_0 == 2'b10}} & {{2{alu_src2[31]}}, alu_src2[31:2]}      |
                       {32{shf_1_0 == 2'b11}} & {{3{alu_src2[31]}}, alu_src2[31:3]};
    assign sra_step2 = {32{shf_3_2 == 2'b00}} & sra_step1                                |
                       {32{shf_3_2 == 2'b01}} & {{4{sra_step1[31]}}, sra_step1[31:4]}    |
                       {32{shf_3_2 == 2'b10}} & {{8{sra_step1[31]}}, sra_step1[31:8]}    |
                       {32{shf_3_2 == 2'b11}} & {{12{sra_step1[31]}}, sra_step1[31:12]};
    assign sra_result = shf[4] ? {{16{sra_step2[31]}}, sra_step2[31:16]} : sra_step2;

    // -------------------------
    // 统一的输出多路�?�择
    // -------------------------
    assign alu_result = (alu_add | alu_sub) ? add_sub_result :
                        alu_slt            ? slt_result     :
                        alu_sltu           ? sltu_result    :
                        alu_and            ? and_result     :
                        alu_nor            ? nor_result     :
                        alu_or             ? or_result      :
                        alu_xor            ? xor_result     :
                        alu_sll            ? sll_result     :
                        alu_srl            ? srl_result     :
                        alu_sra            ? sra_result     :
                        alu_lui            ? lui_result     :
                        32'd0;             // 未使能任何操作时�?化为 0，避免锁�?
endmodule
