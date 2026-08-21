`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�?: exe.v
//   > 描述  : 五级流水 CPU 的执行级，负责触�? ALU / 乘法器运算并与后级交换控制信�?
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module exe(
    input              EXE_valid,    // 执行级当前周期是否携带有效指令？
    input      [166:0] ID_EXE_bus_r, // 来自译码级的控制/数据信息
    output             EXE_over,     // 执行级是否完成本条指�?
    output     [153:0] EXE_MEM_bus,  // 向访存级传�?�的总线内容
    output     [ 31:0] EXE_forward_data,  // 给译码级的执行阶段旁路数�?
    output             EXE_forward_valid, // 执行阶段旁路数据有效指示

    // 五级流水额外接口
    input              clk,          // 时钟信号（乘法器为时序�?�辑�?
    output     [  4:0] EXE_wdest,    // 执行级写回目的寄存器号（用于前�?�与冒险判断�?

    // 调试观测接口
    output     [ 31:0] EXE_pc        // 当前指令对应�? PC
);
    // =========================================================================
    // ID->EXE 寄存器中展开控制信号与数据操作数
    // =========================================================================
    wire        multiply;
    wire        mthi;
    wire        mtlo;
    wire [11:0] alu_control;
    wire [31:0] alu_operand1;
    wire [31:0] alu_operand2;
    wire [ 3:0] mem_control;
    wire [31:0] store_data;
    wire        mfhi;
    wire        mflo;
    wire        mtc0;
    wire        mfc0;
    wire [ 7:0] cp0r_addr;
    wire        syscall;
    wire        eret;
    wire        rf_wen;
    wire [ 4:0] rf_wdest;
    wire [31:0] pc;

    assign {multiply,
            mthi,
            mtlo,
            alu_control,
            alu_operand1,
            alu_operand2,
            mem_control,
            store_data,
            mfhi,
            mflo,
            mtc0,
            mfc0,
            cp0r_addr,
            syscall,
            eret,
            rf_wen,
            rf_wdest,
            pc} = ID_EXE_bus_r;

    // =========================================================================
    // 纯组�? ALU：译码级已经选好操作数，只需接入控制码即�?
    // =========================================================================
    wire [31:0] alu_result;
    alu alu_module(
        .alu_control (alu_control ),
        .alu_src1    (alu_operand1),
        .alu_src2    (alu_operand2),
        .alu_result  (alu_result  )
    );

    // =========================================================================
    // 时序乘法器：MULT 指令�?要多拍完成，利用 mult_begin/mult_end 握手
    // =========================================================================
    wire        mult_begin;
    wire [63:0] product;
    wire        mult_end;
    wire        inst_load;
    assign inst_load = mem_control[3];

    assign mult_begin = multiply & EXE_valid;  // 只有指令有效且为 MULT 才启动乘法器

    multiply multiply_module(
        .clk       (clk        ),
        .mult_begin(mult_begin ),
        .mult_op1  (alu_operand1),
        .mult_op2  (alu_operand2),
        .product   (product    ),
        .mult_end  (mult_end   )
    );

    // 执行级完成条件：非乘法指令单拍完成，乘法指令等待 mult_end
    assign EXE_over = EXE_valid & (~multiply | mult_end);

    // 写回目的寄存器只在当前拍有效时才对外公布，避免伪相关
    assign EXE_wdest = rf_wdest & {5{EXE_valid}};

    // =========================================================================
    // 形成送往访存级的数据：根据指令类型�?�择 HI/LO/CP0/ALU 的来�?
    // =========================================================================
    wire [31:0] exe_result;  // �? 32 位写回数据（对应 HI 或普通运算）
    wire [31:0] lo_result;   // �? 32 位写回数据（LO 寄存器）
    wire        hi_write;
    wire        lo_write;

    assign exe_result = mthi     ? alu_operand1 :   // MTHI 直接�? HI
                        mtc0     ? alu_operand2 :   // MTC0 将�?�用寄存器写�? CP0
                        multiply ? product[63:32] : // MULT 结果�? 32 位写 HI
                                     alu_result;    // 其他算术逻辑运算结果

    assign lo_result  = mtlo ? alu_operand1 : product[31:0];
    assign hi_write   = multiply | mthi;
    assign lo_write   = multiply | mtlo;

    assign EXE_forward_data  = exe_result;
    assign EXE_forward_valid = EXE_valid & rf_wen
                             & ~inst_load
                             & ~mfc0
                             & (~multiply | mult_end);

    // =========================================================================
    // 组装 EXE->MEM 总线：同时携带访存控制�?�HI/LO 写信号以及后�? WB �?�?信息
    // =========================================================================
    assign EXE_MEM_bus = {mem_control,store_data,          // 访存阶段的控�? + 写数�?
                          exe_result,                      // HI/ALU 结果
                          lo_result,                       // LO 结果
                          hi_write,lo_write,               // HI/LO 写使�?
                          mfhi,mflo,                       // 写回阶段是否读取 HI/LO
                          mtc0,mfc0,cp0r_addr,syscall,eret,// CP0 访问及异常控�?
                          rf_wen,rf_wdest,                 // 写回寄存器信�?
                          pc};                             // 调试�? PC

    // 透传调试接口
    assign EXE_pc = pc;
endmodule




