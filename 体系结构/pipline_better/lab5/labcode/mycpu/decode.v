`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�??: decode.v
//   > 描述  : 五级流水 CPU 的译码级核心逻辑，负责指令解析�?�控制信号生成以及冒险检测�??
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
`include "exception_defines.vh"
module decode(
    input              ID_valid,
    input      [ 63:0] IF_ID_bus_r,
    input      [ 31:0] rs_value,
    input      [ 31:0] rt_value,
    output     [  4:0] rs,
    output     [  4:0] rt,
    output     [ 32:0] jbr_bus,
    output             ID_over,
    output     [206:0] ID_EXE_bus,
     input              IF_over,
    input      [  4:0] EXE_wdest,
    input      [  4:0] MEM_wdest,
    input      [  4:0] WB_wdest,
    input      [ 31:0] EXE_forward_data,
    input              EXE_forward_valid,
    input      [ 31:0] MEM_forward_data,
    input              MEM_forward_valid,
    input      [ 31:0] WB_forward_data,
    input              WB_forward_valid,
    output     [ 31:0] ID_pc
);
    // =========================================================================
    // IF->ID 总线拆包：拿到本周期�??要译码的 PC 与指令�??
    // =========================================================================
    wire [31:0] pc;
    wire [31:0] inst;
    assign {pc, inst} = IF_ID_bus_r;
    // =========================================================================
    // 指令字段分解：按 MIPS 32 位格式提取基�??字段供后续�?�辑使用�??
    // =========================================================================
    wire [5:0] op;
    wire [4:0] rd;
    wire [4:0] sa;
    wire [5:0] funct;
    wire [15:0] imm;
    wire [15:0] offset;
    wire [25:0] target;
    wire [2:0] cp0r_sel;
    assign op     = inst[31:26];
    assign rs     = inst[25:21];
    assign rt     = inst[20:16];
    assign rd     = inst[15:11];
    assign sa     = inst[10:6];
    assign funct  = inst[5:0];
    assign imm    = inst[15:0];
    assign offset = inst[15:0];
    assign target = inst[25:0];
    assign cp0r_sel= inst[2:0];
    // =========================================================================
    // 指令类型识别：根�?? op、funct 等字段生成独热码，每条语句对应一种指令�??
    // =========================================================================
    wire inst_ADDU, inst_SUBU , inst_SLT , inst_AND;
    wire inst_NOR , inst_OR   , inst_XOR , inst_SLL;
    wire inst_SRL , inst_ADDIU, inst_BEQ , inst_BNE;
    wire inst_LW  , inst_SW   , inst_LUI , inst_J;
    wire inst_SLTU, inst_JALR , inst_JR  , inst_SLLV;
    wire inst_SRA , inst_SRAV , inst_SRLV, inst_SLTIU;
    wire inst_SLTI, inst_BGEZ , inst_BGTZ, inst_BLEZ;
    wire inst_BLTZ, inst_LB   , inst_LBU , inst_SB;
    wire inst_ANDI, inst_ORI  , inst_XORI, inst_JAL;
    wire inst_MULT, inst_MFLO , inst_MFHI, inst_MTLO;
    wire inst_MTHI, inst_MFC0 , inst_MTC0;
    wire inst_ERET, inst_SYSCALL;
    wire inst_supported, exc_syscall_id, exc_ri_id, exc_valid_id, exc_adel_if, exc_break;
    wire inst_ADD, inst_SUB, inst_BREAK;
    wire [4:0] exc_code_id;
    wire [31:0] bad_addr_id;
    wire op_zero;
    wire sa_zero;
    assign op_zero = ~(|op);
    assign sa_zero = ~(|sa);
    assign inst_ADDU  = op_zero & sa_zero    & (funct == 6'b100001);
    assign inst_SUBU  = op_zero & sa_zero    & (funct == 6'b100011);
    assign inst_SLT   = op_zero & sa_zero    & (funct == 6'b101010);
    assign inst_SLTU  = op_zero & sa_zero    & (funct == 6'b101011);
    assign inst_JALR  = op_zero & (rt==5'd0) & (rd==5'd31)
                      & sa_zero & (funct == 6'b001001);
    assign inst_JR    = op_zero & (rt==5'd0) & (rd==5'd0 )
                      & sa_zero & (funct == 6'b001000);
    assign inst_AND   = op_zero & sa_zero    & (funct == 6'b100100);
    assign inst_NOR   = op_zero & sa_zero    & (funct == 6'b100111);
    assign inst_OR    = op_zero & sa_zero    & (funct == 6'b100101);
    assign inst_XOR   = op_zero & sa_zero    & (funct == 6'b100110);
    assign inst_SLL   = op_zero & (rs==5'd0) & (funct == 6'b000000);
    assign inst_SLLV  = op_zero & sa_zero    & (funct == 6'b000100);
    assign inst_SRA   = op_zero & (rs==5'd0) & (funct == 6'b000011);
    assign inst_SRAV  = op_zero & sa_zero    & (funct == 6'b000111);
    assign inst_SRL   = op_zero & (rs==5'd0) & (funct == 6'b000010);
    assign inst_SRLV  = op_zero & sa_zero    & (funct == 6'b000110);
    assign inst_MULT  = op_zero & (rd==5'd0)
                      & sa_zero & (funct == 6'b011000);
    assign inst_MFLO  = op_zero & (rs==5'd0) & (rt==5'd0)
                      & sa_zero & (funct == 6'b010010);
    assign inst_MFHI  = op_zero & (rs==5'd0) & (rt==5'd0)
                      & sa_zero & (funct == 6'b010000);
    assign inst_MTLO  = op_zero & (rt==5'd0) & (rd==5'd0)
                      & sa_zero & (funct == 6'b010011);
    assign inst_MTHI  = op_zero & (rt==5'd0) & (rd==5'd0)
                      & sa_zero & (funct == 6'b010001);
    assign inst_ADDIU = (op == 6'b001001);
    assign inst_SLTI  = (op == 6'b001010);
    assign inst_SLTIU = (op == 6'b001011);
    assign inst_BEQ   = (op == 6'b000100);
    assign inst_BGEZ  = (op == 6'b000001) & (rt==5'd1);
    assign inst_BGTZ  = (op == 6'b000111) & (rt==5'd0);
    assign inst_BLEZ  = (op == 6'b000110) & (rt==5'd0);
    assign inst_BLTZ  = (op == 6'b000001) & (rt==5'd0);
    assign inst_BNE   = (op == 6'b000101);
    assign inst_LW    = (op == 6'b100011);
    assign inst_SW    = (op == 6'b101011);
    assign inst_LB    = (op == 6'b100000);
    assign inst_LBU   = (op == 6'b100100);
    assign inst_SB    = (op == 6'b101000);
    assign inst_ANDI  = (op == 6'b001100);
    assign inst_LUI   = (op == 6'b001111) & (rs==5'd0);
    assign inst_ORI   = (op == 6'b001101);
    assign inst_XORI  = (op == 6'b001110);
    assign inst_J     = (op == 6'b000010);
    assign inst_JAL   = (op == 6'b000011);
    assign inst_MFC0    = (op == 6'b010000) & (rs==5'd0)
                        & sa_zero & (funct[5:3] == 3'b000);
    assign inst_MTC0    = (op == 6'b010000) & (rs==5'd4)
                        & sa_zero & (funct[5:3] == 3'b000);
    assign inst_SYSCALL = (op == 6'b000000) & (funct == 6'b001100);
    assign inst_ERET    = (op == 6'b010000) & (rs==5'd16) & (rt==5'd0)
                        & (rd==5'd0) & sa_zero & (funct == 6'b011000);
    assign inst_ADD    = op_zero & sa_zero & (funct == 6'b100000);
    assign inst_SUB    = op_zero & sa_zero & (funct == 6'b100010);
    assign inst_BREAK  = op_zero & (funct == 6'b001101);
    assign inst_supported = inst_ADDU | inst_SUBU | inst_SLT | inst_AND |
                            inst_NOR  | inst_OR   | inst_XOR | inst_SLL |
                            inst_SRL  | inst_ADDIU| inst_BEQ | inst_BNE |
                            inst_LW   | inst_SW   | inst_LUI | inst_J   |
                            inst_SLTU | inst_JALR | inst_JR  | inst_SLLV|
                            inst_SRA  | inst_SRAV | inst_SRLV| inst_SLTIU|
                            inst_SLTI | inst_BGEZ | inst_BGTZ| inst_BLEZ|
                            inst_BLTZ | inst_LB   | inst_LBU | inst_SB  |
                            inst_ANDI | inst_ORI  | inst_XORI| inst_JAL |
                            inst_MULT | inst_MFLO | inst_MFHI| inst_MTLO|
                            inst_MTHI | inst_MFC0 | inst_MTC0| inst_ERET|
                            inst_SYSCALL| inst_ADD | inst_SUB | inst_BREAK;
    assign exc_syscall_id = ID_valid & inst_SYSCALL;
    assign exc_ri_id      = ID_valid & ~inst_supported;
    wire target_unaligned;
    wire [31:0] jbr_target_raw;
    wire jbr_req;
    wire exc_adel_if_curr = ID_valid & |pc[1:0];          // �������� ROM ��ʼ��ַδ����
    wire exc_adel_if_jbr  = ID_valid & jbr_req & target_unaligned;  // ��תĿ��δ����
    assign exc_adel_if = exc_adel_if_curr | exc_adel_if_jbr;
    assign exc_break   = ID_valid & inst_BREAK;
    assign exc_valid_id = exc_syscall_id | exc_ri_id | exc_break | exc_adel_if;
    assign exc_code_id = exc_adel_if   ? `EXC_CODE_ADEL :
                         exc_break     ? `EXC_CODE_BP   :
                         exc_syscall_id? `EXC_CODE_SYS  :
                         exc_ri_id     ? `EXC_CODE_RI   : 5'd0;
    assign bad_addr_id = exc_adel_if_curr ? pc :
                         exc_adel_if_jbr  ? jbr_target_raw :
                         exc_ri_id        ? pc : 32'd0;
    
     wire inst_add_ov = inst_ADD;
     wire inst_sub_ov = inst_SUB;
    // =========================================================================
    // 译码后功能分组：拆分跳转、链接等控制信号，便于后续处理�??
    // =========================================================================
    wire inst_jr;
    wire inst_j_link;
    wire inst_jbr;
    assign inst_jr     = inst_JALR | inst_JR;
    assign inst_j_link = inst_JAL | inst_JALR;
    assign inst_jbr = inst_J    | inst_JAL  | inst_jr
                    | inst_BEQ  | inst_BNE  | inst_BGEZ
                    | inst_BGTZ | inst_BLEZ | inst_BLTZ;
    wire inst_load;
    wire inst_store;
    // =========================================================================
    // ALU / Load-Store 功能归类：将具体指令映射到执行�?�访存所�??的功能类别�??
    // =========================================================================
    assign inst_load  = inst_LW | inst_LB | inst_LBU;
    assign inst_store = inst_SW | inst_SB;
    wire inst_add, inst_sub, inst_slt,inst_sltu;
    wire inst_and, inst_nor, inst_or, inst_xor;
    wire inst_sll, inst_srl, inst_sra,inst_lui;
    assign inst_add = inst_ADDU | inst_ADDIU | inst_ADD  |
                      inst_load | inst_store | inst_j_link;
    assign inst_sub = inst_SUBU | inst_SUB;
    assign inst_slt = inst_SLT | inst_SLTI;
    assign inst_sltu= inst_SLTIU | inst_SLTU;
    assign inst_and = inst_AND | inst_ANDI;
    assign inst_nor = inst_NOR;
    assign inst_or  = inst_OR  | inst_ORI;
    assign inst_xor = inst_XOR | inst_XORI;
    assign inst_sll = inst_SLL | inst_SLLV;
    assign inst_srl = inst_SRL | inst_SRLV;
    assign inst_sra = inst_SRA | inst_SRAV;
    assign inst_lui = inst_LUI;
    // 运算数来源判定：区分移位量�?�零扩展与符号扩展的立即数形�??
    wire inst_shf_sa;
    assign inst_shf_sa =  inst_SLL | inst_SRL | inst_SRA;
    wire inst_imm_zero;
    wire inst_imm_sign;
    assign inst_imm_zero = inst_ANDI  | inst_LUI  | inst_ORI | inst_XORI;
    assign inst_imm_sign = inst_ADDIU | inst_SLTI | inst_SLTIU
                         | inst_load | inst_store;
    // 写回目标寄存器�?�择：根据指令类型确定目的寄存器�??
    wire inst_wdest_rt;
    wire inst_wdest_31;
    wire inst_wdest_rd;
    assign inst_wdest_rt = inst_imm_zero | inst_ADDIU | inst_SLTI
                         | inst_SLTIU | inst_load | inst_MFC0;
    assign inst_wdest_31 = inst_JAL;
    assign inst_wdest_rd =inst_ADDU | inst_ADD | inst_SUB | inst_SUBU | inst_SLT  | inst_SLTU
                         | inst_JALR | inst_AND  | inst_NOR  | inst_OR
                            | inst_XOR  | inst_SLL  | inst_SLLV | inst_SRA
                         | inst_SRAV | inst_SRL  | inst_SRLV
                         | inst_MFHI | inst_MFLO;
    // 数据冒险�??测准备：哪些指令不读�?? rs/rt，可以跳过相关�?�判�??
    wire inst_no_rs;
    wire inst_no_rt;
    assign inst_no_rs = inst_MTC0 | inst_SYSCALL | inst_ERET | inst_BREAK;
    assign inst_no_rt = inst_ADDIU | inst_SLTI | inst_SLTIU
                      | inst_BGEZ  | inst_load | inst_imm_zero
                      | inst_J     | inst_JAL  | inst_MFC0
                      | inst_SYSCALL | inst_BREAK;
                      
    wire        rs_need;
    wire        rt_need;
    wire        rs_match_exe;
    wire        rs_match_mem;
    wire        rs_match_wb;
    wire        rt_match_exe;
    wire        rt_match_mem;
    wire        rt_match_wb;
    wire [31:0] rs_data;
    wire [31:0] rt_data;

    assign rs_need = ~inst_no_rs & (rs != 5'd0);
    assign rt_need = ~inst_no_rt & (rt != 5'd0);

    assign rs_match_exe = rs_need & (rs == EXE_wdest);
    assign rs_match_mem = rs_need & (rs == MEM_wdest);
    assign rs_match_wb  = rs_need & (rs == WB_wdest);
    assign rt_match_exe = rt_need & (rt == EXE_wdest);
    assign rt_match_mem = rt_need & (rt == MEM_wdest);
    assign rt_match_wb  = rt_need & (rt == WB_wdest);

    assign rs_data = rs_match_exe && EXE_forward_valid ? EXE_forward_data :
                     rs_match_mem && MEM_forward_valid ? MEM_forward_data :
                     rs_match_wb  && WB_forward_valid  ? WB_forward_data  :
                                                       rs_value;

    assign rt_data = rt_match_exe && EXE_forward_valid ? EXE_forward_data :
                     rt_match_mem && MEM_forward_valid ? MEM_forward_data :
                     rt_match_wb  && WB_forward_valid  ? WB_forward_data  :
                                                       rt_value;                  
                
    // =========================================================================
    // 分支与跳转目标计算：生成延迟�?? PC、跳转目标以及是否命中�??
    // =========================================================================
    wire [31:0] bd_pc;
    assign bd_pc = pc + 3'b100;
    wire        j_taken;
    wire [31:0] j_target;
    assign j_taken = inst_J | inst_JAL | inst_jr;
    assign j_target = inst_jr ? rs_data : {bd_pc[31:28],target,2'b00};
    wire rs_equql_rt;
    wire rs_ez;
    wire rs_ltz;
    assign rs_equql_rt = (rs_data == rt_data);
    assign rs_ez       = ~(|rs_data);
    assign rs_ltz      = rs_data[31];
    wire br_taken;
    wire [31:0] br_target;
    assign br_taken = inst_BEQ  & rs_equql_rt
                    | inst_BNE  & ~rs_equql_rt
                    | inst_BGEZ & ~rs_ltz
                    | inst_BGTZ & ~rs_ltz & ~rs_ez
                    | inst_BLEZ & (rs_ltz | rs_ez)
                    | inst_BLTZ & rs_ltz;
    assign br_target[31:2] = bd_pc[31:2] + {{14{offset[15]}}, offset};
    assign br_target[1:0]  = bd_pc[1:0];
    wire jbr_taken;
    wire [31:0] jbr_target;
    assign jbr_req = (j_taken | br_taken);
    assign jbr_target_raw = j_taken ? j_target : br_target;
    assign target_unaligned = jbr_req & |jbr_target_raw[1:0];
    assign jbr_taken = jbr_req & ID_over & ~target_unaligned;
    assign jbr_target = jbr_target_raw;
    assign jbr_bus = {jbr_taken, jbr_target};
    // =========================================================================
    // 结构/数据冒险�??测：与后级目的寄存器冲突时阻塞译码级�??
    // =========================================================================
    wire rs_wait;
    wire rt_wait;
    assign rs_wait = rs_match_exe ? ~EXE_forward_valid :
                     rs_match_mem ? ~MEM_forward_valid :
                     rs_match_wb  ? ~WB_forward_valid  :
                                    1'b0;

    assign rt_wait = rt_match_exe ? ~EXE_forward_valid :
                     rt_match_mem ? ~MEM_forward_valid :
                     rt_match_wb  ? ~WB_forward_valid  :
                                    1'b0;
    assign ID_over = ID_valid & ~rs_wait & ~rt_wait & (~inst_jbr | IF_over);
    // =========================================================================
    // 执行级输出准备：将执行�?�访存�?�写回需要的控制/数据统一打包�??
    // =========================================================================
    wire multiply;
    wire mthi;
    wire mtlo;
    assign multiply = inst_MULT;
    assign mthi     = inst_MTHI;
    assign mtlo     = inst_MTLO;
    wire [11:0] alu_control;
    wire [31:0] alu_operand1;
    wire [31:0] alu_operand2;
    // ALU 操作数�?�择：在 PC、移位量与寄存器值之间复�??
    assign alu_operand1 = inst_j_link ? pc :
                          inst_shf_sa ? {27'd0,sa} : rs_data;
    assign alu_operand2 = inst_j_link ? 32'd8 :
                          inst_imm_zero ? {16'd0, imm} :
                          inst_imm_sign ?  {{16{imm[15]}}, imm} : rt_data;
    assign alu_control = {inst_add,
                          inst_sub,
                          inst_slt,
                          inst_sltu,
                          inst_and,
                          inst_nor,
                          inst_or,
                          inst_xor,
                          inst_sll,
                          inst_srl,
                          inst_sra,
                          inst_lui};
    wire lb_sign;
    wire ls_word;
    wire [3:0] mem_control;
    wire [31:0] store_data;
    assign lb_sign = inst_LB;
    assign ls_word = inst_LW | inst_SW;
    // 访存控制信息打包：高位指�?? load/store，低位描述访问宽度及符号扩展
    assign mem_control = {inst_load,
                          inst_store,
                          ls_word,
                          lb_sign };
    wire mfhi;
    wire mflo;
    wire mtc0;
    wire mfc0;
    wire [7 :0] cp0r_addr;
    wire       syscall;
    wire       eret;
    wire       rf_wen;
    wire [4:0] rf_wdest;
    assign syscall  = inst_SYSCALL;
    assign eret     = inst_ERET;
    assign mfhi     = inst_MFHI;
    assign mflo     = inst_MFLO;
    assign mtc0     = inst_MTC0;
    assign mfc0     = inst_MFC0;
    assign cp0r_addr= {rd,cp0r_sel};
    assign rf_wen   = inst_wdest_rt | inst_wdest_31 | inst_wdest_rd;
    // 写回目标优先级：优先 rt，其次链接寄存器 31，最�?? rd
    assign rf_wdest = inst_wdest_rt ? rt :
                      inst_wdest_31 ? 5'd31 :
                      inst_wdest_rd ? rd : 5'd0;
    assign store_data = rt_data;
    // 打包形成 ID->EXE 总线，确保流水线寄存器信息齐�??
    assign ID_EXE_bus = {multiply,mthi,mtlo,
                         alu_control,alu_operand1,alu_operand2,
                         mem_control,store_data,
                         mfhi,mflo,
                         mtc0,mfc0,cp0r_addr,eret,
                         exc_valid_id,exc_code_id,bad_addr_id,
                         rf_wen, rf_wdest,
                         inst_add_ov, inst_sub_ov,
                         pc};
    // 对外输出当前指令�?? PC，方便调试与观测
    assign ID_pc = pc;
endmodule
