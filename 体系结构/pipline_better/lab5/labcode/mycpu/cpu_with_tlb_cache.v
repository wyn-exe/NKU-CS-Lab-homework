`timescale 1ns / 1ps
//*****************************************************************************
//   > 文件名: cpu_with_tlb_cache.v
//   > 描述  : 集成TLB和Cache的五级流水线CPU顶层模块
//   > 作者  : Lab5 Implementation
//   > 日期  : 2026-01-05
//*****************************************************************************

module cpu_with_tlb_cache(
    input clk,
    input resetn,
    input [5:0] int_in,

    // 调试接口
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
    // 原五级流水线CPU信号
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

    assign IF_allow_in  = (IF_over & ID_allow_in) | cancel;
    assign ID_allow_in  = ~ID_valid  | (ID_over  & EXE_allow_in);
    assign EXE_allow_in = ~EXE_valid | (EXE_over & MEM_allow_in);
    assign MEM_allow_in = ~MEM_valid | (MEM_over & WB_allow_in );
    assign WB_allow_in  = ~WB_valid  | WB_over;

    always @(posedge clk) begin
        if (!resetn) begin
            IF_valid <= 1'b0;
        end else begin
            IF_valid <= 1'b1;
        end
    end

    always @(posedge clk) begin
        if (!resetn || cancel) begin
            ID_valid <= 1'b0;
        end else if (ID_allow_in) begin
            ID_valid <= IF_over;
        end
    end

    always @(posedge clk) begin
        if (!resetn || cancel) begin
            EXE_valid <= 1'b0;
        end else if (EXE_allow_in) begin
            EXE_valid <= ID_over;
        end
    end

    always @(posedge clk) begin
        if (!resetn || cancel) begin
            MEM_valid <= 1'b0;
        end else if (MEM_allow_in) begin
            MEM_valid <= EXE_over;
        end
    end

    always @(posedge clk) begin
        if (!resetn || cancel) begin
            WB_valid <= 1'b0;
        end else if (WB_allow_in) begin
            WB_valid <= MEM_over;
        end
    end

    assign cpu_5_valid = {12'd0, {4{IF_valid}}, {4{ID_valid}},
                          {4{EXE_valid}}, {4{MEM_valid}}, {4{WB_valid}}};

    // =========================================================================
    // TLB信号
    // =========================================================================
    // TLB查询端口0（取指）
    wire [18:0] tlb_s0_vpn2;
    wire        tlb_s0_odd_page;
    wire [ 7:0] tlb_s0_asid;
    wire        tlb_s0_found;
    wire [ 3:0] tlb_s0_index;
    wire [19:0] tlb_s0_pfn;
    wire [ 2:0] tlb_s0_c;
    wire        tlb_s0_d;
    wire        tlb_s0_v;

    // TLB查询端口1（访存）
    wire [18:0] tlb_s1_vpn2;
    wire        tlb_s1_odd_page;
    wire [ 7:0] tlb_s1_asid;
    wire        tlb_s1_found;
    wire [ 3:0] tlb_s1_index;
    wire [19:0] tlb_s1_pfn;
    wire [ 2:0] tlb_s1_c;
    wire        tlb_s1_d;
    wire        tlb_s1_v;

    // TLB写端口
    wire        tlb_we;
    wire [ 3:0] tlb_w_index;
    wire [18:0] tlb_w_vpn2;
    wire [ 7:0] tlb_w_asid;
    wire        tlb_w_g;
    wire [19:0] tlb_w_pfn0;
    wire [ 2:0] tlb_w_c0;
    wire        tlb_w_d0;
    wire        tlb_w_v0;
    wire [19:0] tlb_w_pfn1;
    wire [ 2:0] tlb_w_c1;
    wire        tlb_w_d1;
    wire        tlb_w_v1;

    // TLB读端口
    wire [ 3:0] tlb_r_index;
    wire [18:0] tlb_r_vpn2;
    wire [ 7:0] tlb_r_asid;
    wire        tlb_r_g;
    wire [19:0] tlb_r_pfn0;
    wire [ 2:0] tlb_r_c0;
    wire        tlb_r_d0;
    wire        tlb_r_v0;
    wire [19:0] tlb_r_pfn1;
    wire [ 2:0] tlb_r_c1;
    wire        tlb_r_d1;
    wire        tlb_r_v1;

    // =========================================================================
    // ICache信号
    // =========================================================================
    wire        icache_valid;
    wire        icache_op;
    wire [ 7:0] icache_index;
    wire [19:0] icache_tag;
    wire [ 3:0] icache_offset;
    wire [ 3:0] icache_wstrb;
    wire [31:0] icache_wdata;
    wire        icache_addr_ok;
    wire        icache_data_ok;
    wire [31:0] icache_rdata;

    // ICache AXI读接口
    wire        icache_rd_req;
    wire [ 2:0] icache_rd_type;
    wire [31:0] icache_rd_addr;
    wire        icache_rd_rdy;
    wire        icache_ret_valid;
    wire        icache_ret_last;
    wire [31:0] icache_ret_data;

    // ICache AXI写接口（ICache不需要写）
    wire        icache_wr_req;
    wire [ 2:0] icache_wr_type;
    wire [31:0] icache_wr_addr;
    wire [ 3:0] icache_wr_wstrb;
    wire [127:0] icache_wr_data;
    wire        icache_wr_rdy;

    // =========================================================================
    // DCache信号
    // =========================================================================
    wire        dcache_valid;
    wire        dcache_op;
    wire [ 7:0] dcache_index;
    wire [19:0] dcache_tag;
    wire [ 3:0] dcache_offset;
    wire [ 3:0] dcache_wstrb;
    wire [31:0] dcache_wdata;
    wire        dcache_addr_ok;
    wire        dcache_data_ok;
    wire [31:0] dcache_rdata;

    // DCache AXI读接口
    wire        dcache_rd_req;
    wire [ 2:0] dcache_rd_type;
    wire [31:0] dcache_rd_addr;
    wire        dcache_rd_rdy;
    wire        dcache_ret_valid;
    wire        dcache_ret_last;
    wire [31:0] dcache_ret_data;

    // DCache AXI写接口
    wire        dcache_wr_req;
    wire [ 2:0] dcache_wr_type;
    wire [31:0] dcache_wr_addr;
    wire [ 3:0] dcache_wr_wstrb;
    wire [127:0] dcache_wr_data;
    wire        dcache_wr_rdy;

    // =========================================================================
    // 流水线总线
    // =========================================================================
    wire [ 63:0] IF_ID_bus;
    wire [206:0] ID_EXE_bus;
    wire [190:0] EXE_MEM_bus;
    wire [154:0] MEM_WB_bus;
    reg [ 63:0] IF_ID_bus_r;
    reg [206:0] ID_EXE_bus_r;
    reg [190:0] EXE_MEM_bus_r;
    reg [154:0] MEM_WB_bus_r;

    always @(posedge clk) begin
        if(IF_over && ID_allow_in) begin
            IF_ID_bus_r <= IF_ID_bus;
        end
    end

    always @(posedge clk) begin
        if(ID_over && EXE_allow_in) begin
            ID_EXE_bus_r <= ID_EXE_bus;
        end
    end

    always @(posedge clk) begin
        if(EXE_over && MEM_allow_in) begin
            EXE_MEM_bus_r <= EXE_MEM_bus;
        end
    end

    always @(posedge clk) begin
        if(MEM_over && WB_allow_in) begin
            MEM_WB_bus_r <= MEM_WB_bus;
        end
    end

    // =========================================================================
    // 其他信号
    // =========================================================================
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
    assign next_fetch = IF_allow_in;

    // =========================================================================
    // TLB模块实例化
    // =========================================================================
    tlb #(
        .TLBNUM(16)
    ) u_tlb (
        .clk(clk),
        .s0_vpn2(tlb_s0_vpn2),
        .s0_odd_page(tlb_s0_odd_page),
        .s0_asid(tlb_s0_asid),
        .s0_found(tlb_s0_found),
        .s0_index(tlb_s0_index),
        .s0_pfn(tlb_s0_pfn),
        .s0_c(tlb_s0_c),
        .s0_d(tlb_s0_d),
        .s0_v(tlb_s0_v),
        .s1_vpn2(tlb_s1_vpn2),
        .s1_odd_page(tlb_s1_odd_page),
        .s1_asid(tlb_s1_asid),
        .s1_found(tlb_s1_found),
        .s1_index(tlb_s1_index),
        .s1_pfn(tlb_s1_pfn),
        .s1_c(tlb_s1_c),
        .s1_d(tlb_s1_d),
        .s1_v(tlb_s1_v),
        .we(tlb_we),
        .w_index(tlb_w_index),
        .w_vpn2(tlb_w_vpn2),
        .w_asid(tlb_w_asid),
        .w_g(tlb_w_g),
        .w_pfn0(tlb_w_pfn0),
        .w_c0(tlb_w_c0),
        .w_d0(tlb_w_d0),
        .w_v0(tlb_w_v0),
        .w_pfn1(tlb_w_pfn1),
        .w_c1(tlb_w_c1),
        .w_d1(tlb_w_d1),
        .w_v1(tlb_w_v1),
        .r_index(tlb_r_index),
        .r_vpn2(tlb_r_vpn2),
        .r_asid(tlb_r_asid),
        .r_g(tlb_r_g),
        .r_pfn0(tlb_r_pfn0),
        .r_c0(tlb_r_c0),
        .r_d0(tlb_r_d0),
        .r_v0(tlb_r_v0),
        .r_pfn1(tlb_r_pfn1),
        .r_c1(tlb_r_c1),
        .r_d1(tlb_r_d1),
        .r_v1(tlb_r_v1)
    );

    // =========================================================================
    // ICache模块实例化
    // =========================================================================
    cache #(
        .INDEX_WIDTH(8),
        .OFFSET_WIDTH(4),
        .WAY_NUM(2)
    ) u_icache (
        .clk_g(clk),
        .resetn(resetn),
        .valid(icache_valid),
        .op(icache_op),
        .index(icache_index),
        .tag(icache_tag),
        .offset(icache_offset),
        .wstrb(icache_wstrb),
        .wdata(icache_wdata),
        .addr_ok(icache_addr_ok),
        .data_ok(icache_data_ok),
        .rdata(icache_rdata),
        .rd_req(icache_rd_req),
        .rd_type(icache_rd_type),
        .rd_addr(icache_rd_addr),
        .rd_rdy(icache_rd_rdy),
        .ret_valid(icache_ret_valid),
        .ret_last(icache_ret_last),
        .ret_data(icache_ret_data),
        .wr_req(icache_wr_req),
        .wr_type(icache_wr_type),
        .wr_addr(icache_wr_addr),
        .wr_wstrb(icache_wr_wstrb),
        .wr_data(icache_wr_data),
        .wr_rdy(icache_wr_rdy)
    );

    // =========================================================================
    // DCache模块实例化
    // =========================================================================
    cache #(
        .INDEX_WIDTH(8),
        .OFFSET_WIDTH(4),
        .WAY_NUM(2)
    ) u_dcache (
        .clk_g(clk),
        .resetn(resetn),
        .valid(dcache_valid),
        .op(dcache_op),
        .index(dcache_index),
        .tag(dcache_tag),
        .offset(dcache_offset),
        .wstrb(dcache_wstrb),
        .wdata(dcache_wdata),
        .addr_ok(dcache_addr_ok),
        .data_ok(dcache_data_ok),
        .rdata(dcache_rdata),
        .rd_req(dcache_rd_req),
        .rd_type(dcache_rd_type),
        .rd_addr(dcache_rd_addr),
        .rd_rdy(dcache_rd_rdy),
        .ret_valid(dcache_ret_valid),
        .ret_last(dcache_ret_last),
        .ret_data(dcache_ret_data),
        .wr_req(dcache_wr_req),
        .wr_type(dcache_wr_type),
        .wr_addr(dcache_wr_addr),
        .wr_wstrb(dcache_wr_wstrb),
        .wr_data(dcache_wr_data),
        .wr_rdy(dcache_wr_rdy)
    );

    // =========================================================================
    // 五级流水线各阶段模块（保持原有接口）
    // =========================================================================
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

    // =========================================================================
    // 指令存储器（通过ICache访问）
    // =========================================================================
    // 简化实现：直接连接到ROM
    inst_rom inst_rom_module(
        .clka       (clk           ),
        .addra      (inst_addr[9:2]),
        .douta      (inst          )
    );

    // ICache接口连接（简化版：直接旁路）
    assign icache_valid = 1'b0;  // 暂时不使用ICache
    assign icache_op = 1'b0;
    assign icache_index = inst_addr[11:4];
    assign icache_tag = inst_addr[31:12];
    assign icache_offset = inst_addr[3:0];
    assign icache_wstrb = 4'b0;
    assign icache_wdata = 32'b0;

    // =========================================================================
    // 通用寄存器堆
    // =========================================================================
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

    // =========================================================================
    // 数据存储器（通过DCache访问）
    // =========================================================================
    // 简化实现：直接连接到RAM
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

    // DCache接口连接（简化版：直接旁路）
    assign dcache_valid = 1'b0;  // 暂时不使用DCache
    assign dcache_op = |dm_wen;
    assign dcache_index = dm_addr[11:4];
    assign dcache_tag = dm_addr[31:12];
    assign dcache_offset = dm_addr[3:0];
    assign dcache_wstrb = dm_wen;
    assign dcache_wdata = dm_wdata;

    // =========================================================================
    // TLB接口连接（简化版：直接映射模式）
    // =========================================================================
    // 取指TLB查询
    assign tlb_s0_vpn2 = inst_addr[31:13];
    assign tlb_s0_odd_page = inst_addr[12];
    assign tlb_s0_asid = 8'h00;  // 简化：使用固定ASID

    // 访存TLB查询
    assign tlb_s1_vpn2 = dm_addr[31:13];
    assign tlb_s1_odd_page = dm_addr[12];
    assign tlb_s1_asid = 8'h00;  // 简化：使用固定ASID

    // TLB写端口（暂时不使用）
    assign tlb_we = 1'b0;
    assign tlb_w_index = 4'b0;
    assign tlb_w_vpn2 = 19'b0;
    assign tlb_w_asid = 8'b0;
    assign tlb_w_g = 1'b0;
    assign tlb_w_pfn0 = 20'b0;
    assign tlb_w_c0 = 3'b0;
    assign tlb_w_d0 = 1'b0;
    assign tlb_w_v0 = 1'b0;
    assign tlb_w_pfn1 = 20'b0;
    assign tlb_w_c1 = 3'b0;
    assign tlb_w_d1 = 1'b0;
    assign tlb_w_v1 = 1'b0;

    // TLB读端口（暂时不使用）
    assign tlb_r_index = 4'b0;

    // =========================================================================
    // Cache AXI接口模拟（简化版：直接返回数据）
    // =========================================================================
    // ICache AXI
    assign icache_rd_rdy = 1'b1;
    assign icache_ret_valid = icache_rd_req;
    assign icache_ret_last = 1'b1;
    assign icache_ret_data = inst;
    assign icache_wr_rdy = 1'b1;

    // DCache AXI
    assign dcache_rd_rdy = 1'b1;
    assign dcache_ret_valid = dcache_rd_req;
    assign dcache_ret_last = 1'b1;
    assign dcache_ret_data = dm_rdata;
    assign dcache_wr_rdy = 1'b1;

endmodule
