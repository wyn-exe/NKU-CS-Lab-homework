`timescale 1ns / 1ps
//*************************************************************************
//   > 文件�?: mem.v
//   > 描述  : 五级流水 CPU 的访存级，负责与数据 RAM 交互、处理字节对齐和 Load 扩展
//   > 作�??  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
`include "exception_defines.vh"
module mem(
    input              clk,
    input              MEM_valid,
    input      [190:0] EXE_MEM_bus_r,
    input      [ 31:0] dm_rdata,
    output     [ 31:0] dm_addr,
    output reg [  3:0] dm_wen,
    output reg [ 31:0] dm_wdata,
    output             MEM_over,
    output     [154:0] MEM_WB_bus,
    output     [ 31:0] MEM_forward_data,  // 访存级旁路到译码级的数据
    output             MEM_forward_valid, // 访存级旁路有效指�?
    input              MEM_allow_in,
    output     [  4:0] MEM_wdest,
    output     [ 31:0] MEM_pc
);
    // =========================================================================
    // EXE->MEM 总线拆包：获取访存控制�?�HI/LO、CP0 及写回相关信�?
    // =========================================================================
    wire [3 :0] mem_control;
    wire [31:0] store_data;
    wire [31:0] exe_result;
    wire [31:0] lo_result;
    wire        hi_write;
    wire        lo_write;
    wire mfhi;
    wire mflo;
    wire mtc0;
    wire mfc0;
    wire [7 :0] cp0r_addr;
    wire       eret;
    wire       rf_wen;
    wire [4:0] rf_wdest;
    wire [31:0] pc;
    wire        exc_valid_i;
    wire [4:0]  exc_code_i;
    wire [31:0] bad_addr_i;
    assign {mem_control,
            store_data,
            exe_result,
            lo_result,
            hi_write,
            lo_write,
            mfhi,
            mflo,
            mtc0,
            mfc0,
            cp0r_addr,
            eret,
            exc_valid_i,exc_code_i,bad_addr_i,
            rf_wen,
            rf_wdest,
            pc         } = EXE_MEM_bus_r;
    wire inst_load;
    wire inst_store;
    wire ls_word;
    wire lb_sign;
    assign {inst_load,inst_store,ls_word,lb_sign} = mem_control;
    wire misalign_word = ls_word & |dm_addr[1:0];
    wire exc_adel = MEM_valid & inst_load  & ~exc_valid_i & misalign_word;
    wire exc_ades = MEM_valid & inst_store & ~exc_valid_i & misalign_word;
    wire exc_valid_mem = exc_valid_i | exc_adel | exc_ades;
    wire [4:0] exc_code_mem = exc_valid_i ? exc_code_i :
                              exc_adel    ? `EXC_CODE_ADEL :
                              exc_ades    ? `EXC_CODE_ADES : 5'd0;
    wire [31:0] bad_addr_mem = exc_valid_i ? bad_addr_i :
                               (exc_adel | exc_ades) ? dm_addr : 32'd0;
    // 访存地址直接由执行级计算结果给出，兼�? load/store
    assign dm_addr = exe_result;
    // 根据地址低两位计算字节写使能：字节存储按单字节使能，字存直接�? 1
    always @ (*)
    begin
        if (MEM_valid && inst_store && ~exc_ades)
        begin
            if (ls_word)
            begin
                dm_wen <= 4'b1111;
            end
            else
            begin
                case (dm_addr[1:0])
                    2'b00   : dm_wen <= 4'b0001;
                    2'b01   : dm_wen <= 4'b0010;
                    2'b10   : dm_wen <= 4'b0100;
                    2'b11   : dm_wen <= 4'b1000;
                    default : dm_wen <= 4'b0000;
                endcase
            end
        end
        else
        begin
            dm_wen <= 4'b0000;
        end
    end
    // 针对 SB 指令，按照地�?对字节写数据做位置重�?
    always @ (*)
    begin
        case (dm_addr[1:0])
            2'b00   : dm_wdata <= store_data;
            2'b01   : dm_wdata <= {16'd0, store_data[7:0], 8'd0};
            2'b10   : dm_wdata <= {8'd0, store_data[7:0], 16'd0};
            2'b11   : dm_wdata <= {store_data[7:0], 24'd0};
            default : dm_wdata <= store_data;
        endcase
    end
    // Load 结果的符号位与数据组装：按地�?对齐并完成符�?/零扩�?
    wire        load_sign;
    wire [31:0] load_result;
    assign load_sign = (dm_addr[1:0]==2'd0) ? dm_rdata[ 7] :
                       (dm_addr[1:0]==2'd1) ? dm_rdata[15] :
                       (dm_addr[1:0]==2'd2) ? dm_rdata[23] : dm_rdata[31] ;
    assign load_result[7:0] = (dm_addr[1:0]==2'd0) ? dm_rdata[ 7:0 ] :
                               (dm_addr[1:0]==2'd1) ? dm_rdata[15:8 ] :
                               (dm_addr[1:0]==2'd2) ? dm_rdata[23:16] :
                                                      dm_rdata[31:24] ;
    assign load_result[31:8]= ls_word ? dm_rdata[31:8] : {24{lb_sign & load_sign}};
    // RAM 为同步读：load 指令�?要额外一拍等待返回数据，通过 valid_r 保持状�??
    reg MEM_valid_r;
    always @(posedge clk)
    begin
        if (MEM_allow_in)
        begin
            MEM_valid_r <= 1'b0;
        end
        else
        begin
            MEM_valid_r <= MEM_valid;
        end
    end
    // load �?要等待上�?拍的 valid_r，store 则当拍可以结�?
    assign MEM_over = (inst_load & ~exc_adel) ? MEM_valid_r : MEM_valid;
    // 仅当本级有效时向前反馈目的寄存器，用于前递冒险判�?
    assign MEM_wdest = rf_wdest & {5{MEM_valid}};
    // Load 返回数据与执行结果二选一，统�?给写回级
    wire [31:0] mem_result;
    assign mem_result = inst_load ? load_result : exe_result;
    
    assign MEM_forward_data  = mem_result;
    assign MEM_forward_valid = MEM_over & rf_wen & ~mfc0 & ~exc_valid_mem;
    // 将访存产出�?�HI/LO、CP0 控制等封装交给写回级
    assign MEM_WB_bus = {rf_wen,rf_wdest,
                         mem_result,
                         lo_result,
                         hi_write,lo_write,
                         mfhi,mflo,
                         mtc0,mfc0,cp0r_addr,eret,
                         exc_valid_mem,exc_code_mem,bad_addr_mem,
                         pc};
    // 调试接口：输出当前访存指令的 PC
    assign MEM_pc = pc;
endmodule


