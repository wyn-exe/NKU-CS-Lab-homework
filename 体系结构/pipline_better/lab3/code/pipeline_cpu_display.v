`timescale 1ns / 1ps
//*************************************************************************
//   > 文件名: pipeline_cpu_display.v
//   > 描述  : 顶层封装，集成 CPU 与 LCD 显示控制逻辑，实现按键控制与实时观测。
//   > 作者  : LOONGSON
//   > 日期  : 2016-04-14
//*************************************************************************
module pipeline_cpu_display(
    input clk,
    input resetn,
    input btn_clk,
    output lcd_rst,
    output lcd_cs,
    output lcd_rs,
    output lcd_wr,
    output lcd_rd,
    inout[15:0] lcd_data_io,
    output lcd_bl_ctr,
    inout ct_int,
    inout ct_sda,
    output ct_scl,
    output ct_rstn
    );
    wire cpu_clk;
	 reg btn_clk_r1;
	 reg btn_clk_r2;
    always @(posedge clk)
    begin
        if (!resetn)
        begin
            btn_clk_r1<= 1'b0;
        end
        else
        begin
            btn_clk_r1 <= ~btn_clk;
        end
        btn_clk_r2 <= btn_clk_r1;
    end
	 // clk_en 为高时 CPU 正常运行，为低时保持流水暂停便于观察
	 wire clk_en;
    assign clk_en = !resetn || (!btn_clk_r1 && btn_clk_r2);
    BUFGCE cpu_clk_cg(.I(clk),.CE(clk_en),.O(cpu_clk));
    wire [ 4:0] rf_addr;
    wire [31:0] rf_data;
    reg  [31:0] mem_addr;
    wire [31:0] mem_data;
    wire [31:0] IF_pc;
    wire [31:0] IF_inst;
    wire [31:0] ID_pc;
    wire [31:0] EXE_pc;
    wire [31:0] MEM_pc;
    wire [31:0] WB_pc;
    wire [31:0] cpu_5_valid;
    wire [31:0] HI_data;
    wire [31:0] LO_data;
    // 嵌入核心 CPU：通过调试端口输出寄存器、访存与流水位置信息
    pipeline_cpu cpu(
        .clk     (cpu_clk ),
        .resetn  (resetn  ),
        .rf_addr (rf_addr ),
        .mem_addr(mem_addr),
        .rf_data (rf_data ),
        .mem_data(mem_data),
        .IF_pc   (IF_pc   ),
        .IF_inst (IF_inst ),
        .ID_pc   (ID_pc   ),
        .EXE_pc  (EXE_pc  ),
        .MEM_pc  (MEM_pc  ),
        .WB_pc   (WB_pc   ),
        .cpu_5_valid (cpu_5_valid),
          .HI_data (HI_data ),
          .LO_data (LO_data )
    );
    reg         display_valid;
    reg  [39:0] display_name;
    reg  [31:0] display_value;
    wire [5 :0] display_number;
    wire        input_valid;
    wire [31:0] input_value;
    // LCD 控制模块：驱动液晶并支持输入按键调整监控内容
    lcd_module lcd_module(
        .clk            (clk           ),
        .resetn         (resetn        ),
        .display_valid  (display_valid ),
        .display_name   (display_name  ),
        .display_value  (display_value ),
        .display_number (display_number),
        .input_valid    (input_valid   ),
        .input_value    (input_value   ),
        .lcd_rst        (lcd_rst       ),
        .lcd_cs         (lcd_cs        ),
        .lcd_rs         (lcd_rs        ),
        .lcd_wr         (lcd_wr        ),
        .lcd_rd         (lcd_rd        ),
        .lcd_data_io    (lcd_data_io   ),
        .lcd_bl_ctr     (lcd_bl_ctr    ),
        .ct_int         (ct_int        ),
        .ct_sda         (ct_sda        ),
        .ct_scl         (ct_scl        ),
        .ct_rstn        (ct_rstn       )
    );
    // 根据触摸输入更新调试访存地址，观察 data_ram 内容
    always @(posedge clk)
    begin
        if (!resetn)
        begin
            mem_addr <= 32'd0;
        end
        else if (input_valid)
        begin
            mem_addr <= input_value;
        end
    end
    assign rf_addr = display_number-6'd13;
    // 根据显示序号选择要输出的流水/寄存器/存储器信息
    always @(posedge clk)
    begin
        if (display_number >6'd12 && display_number <6'd45 )
        begin
            display_valid <= 1'b1;
            display_name[39:16] <= "REG";
            display_name[15: 8] <= {4'b0011,3'b000,rf_addr[4]};
            display_name[7 : 0] <= {4'b0011,rf_addr[3:0]};
            display_value       <= rf_data;
          end
        else
        begin
            case(display_number)
                6'd1 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "IF_PC";
                    display_value <= IF_pc;
                end
                6'd2 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "IF_IN";
                    display_value <= IF_inst;
                end
                6'd3 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "ID_PC";
                    display_value <= ID_pc;
                end
                6'd4 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "EXEPC";
                    display_value <= EXE_pc;
                end
                6'd5 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "MEMPC";
                    display_value <= MEM_pc;
                end
                6'd6 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "WB_PC";
                    display_value <= WB_pc;
                end
                6'd7 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "MADDR";
                    display_value <= mem_addr;
                end
                6'd8 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "MDATA";
                    display_value <= mem_data;
                end
                6'd9 :
                begin
                    display_valid <= 1'b1;
                    display_name  <= "VALID";
                    display_value <= cpu_5_valid;
                end
                6'd11:
                begin
                    display_valid <= 1'b1;
                    display_name  <= "   HI";
                    display_value <= HI_data;
                end
                6'd12:
                begin
                    display_valid <= 1'b1;
                    display_name  <= "   LO";
                    display_value <= LO_data;
                end
                default :
                begin
                    display_valid <= 1'b0;
                    display_name  <= 40'd0;
                    display_value <= 32'd0;
                end
            endcase
        end
    end
endmodule


