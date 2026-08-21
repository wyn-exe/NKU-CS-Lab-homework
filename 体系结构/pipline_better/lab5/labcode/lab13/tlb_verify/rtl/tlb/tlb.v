`timescale 1ns / 1ps
//*****************************************************************************
//   > 文件名: tlb.v
//   > 描述  : TLB模块实现，支持双端口查询和读写操作
//   > 作者  : Lab5 Implementation
//   > 日期  : 2026-01-05
//*****************************************************************************

module tlb #(
    parameter TLBNUM = 16
)(
    input  wire        clk,

    // search port 0 (for instruction fetch)
    input  wire [18:0] s0_vpn2,
    input  wire        s0_odd_page,
    input  wire [ 7:0] s0_asid,
    output wire        s0_found,
    output wire [$clog2(TLBNUM)-1:0] s0_index,
    output wire [19:0] s0_pfn,
    output wire [ 2:0] s0_c,
    output wire        s0_d,
    output wire        s0_v,

    // search port 1 (for data access)
    input  wire [18:0] s1_vpn2,
    input  wire        s1_odd_page,
    input  wire [ 7:0] s1_asid,
    output wire        s1_found,
    output wire [$clog2(TLBNUM)-1:0] s1_index,
    output wire [19:0] s1_pfn,
    output wire [ 2:0] s1_c,
    output wire        s1_d,
    output wire        s1_v,

    // write port
    input  wire        we,
    input  wire [$clog2(TLBNUM)-1:0] w_index,
    input  wire [18:0] w_vpn2,
    input  wire [ 7:0] w_asid,
    input  wire        w_g,
    input  wire [19:0] w_pfn0,
    input  wire [ 2:0] w_c0,
    input  wire        w_d0,
    input  wire        w_v0,
    input  wire [19:0] w_pfn1,
    input  wire [ 2:0] w_c1,
    input  wire        w_d1,
    input  wire        w_v1,

    // read port
    input  wire [$clog2(TLBNUM)-1:0] r_index,
    output wire [18:0] r_vpn2,
    output wire [ 7:0] r_asid,
    output wire        r_g,
    output wire [19:0] r_pfn0,
    output wire [ 2:0] r_c0,
    output wire        r_d0,
    output wire        r_v0,
    output wire [19:0] r_pfn1,
    output wire [ 2:0] r_c1,
    output wire        r_d1,
    output wire        r_v1
);

    // TLB存储结构
    // 每个TLB表项包含：
    // - VPN2 (19 bits): 虚拟页号
    // - ASID (8 bits): 地址空间标识符
    // - G (1 bit): 全局位
    // - PFN0 (20 bits): 偶数页物理帧号
    // - C0 (3 bits): 偶数页cache属性
    // - D0 (1 bit): 偶数页脏位
    // - V0 (1 bit): 偶数页有效位
    // - PFN1 (20 bits): 奇数页物理帧号
    // - C1 (3 bits): 奇数页cache属性
    // - D1 (1 bit): 奇数页脏位
    // - V1 (1 bit): 奇数页有效位

    reg [18:0] tlb_vpn2  [TLBNUM-1:0];
    reg [ 7:0] tlb_asid  [TLBNUM-1:0];
    reg        tlb_g     [TLBNUM-1:0];
    reg [19:0] tlb_pfn0  [TLBNUM-1:0];
    reg [ 2:0] tlb_c0    [TLBNUM-1:0];
    reg        tlb_d0    [TLBNUM-1:0];
    reg        tlb_v0    [TLBNUM-1:0];
    reg [19:0] tlb_pfn1  [TLBNUM-1:0];
    reg [ 2:0] tlb_c1    [TLBNUM-1:0];
    reg        tlb_d1    [TLBNUM-1:0];
    reg        tlb_v1    [TLBNUM-1:0];

    // 写端口：同步写入
    integer i;
    always @(posedge clk) begin
        if (we) begin
            tlb_vpn2[w_index] <= w_vpn2;
            tlb_asid[w_index] <= w_asid;
            tlb_g[w_index]    <= w_g;
            tlb_pfn0[w_index] <= w_pfn0;
            tlb_c0[w_index]   <= w_c0;
            tlb_d0[w_index]   <= w_d0;
            tlb_v0[w_index]   <= w_v0;
            tlb_pfn1[w_index] <= w_pfn1;
            tlb_c1[w_index]   <= w_c1;
            tlb_d1[w_index]   <= w_d1;
            tlb_v1[w_index]   <= w_v1;
        end
    end

    // 读端口：组合逻辑读取
    assign r_vpn2 = tlb_vpn2[r_index];
    assign r_asid = tlb_asid[r_index];
    assign r_g    = tlb_g[r_index];
    assign r_pfn0 = tlb_pfn0[r_index];
    assign r_c0   = tlb_c0[r_index];
    assign r_d0   = tlb_d0[r_index];
    assign r_v0   = tlb_v0[r_index];
    assign r_pfn1 = tlb_pfn1[r_index];
    assign r_c1   = tlb_c1[r_index];
    assign r_d1   = tlb_d1[r_index];
    assign r_v1   = tlb_v1[r_index];

    // 查询端口0：组合逻辑查询
    wire [TLBNUM-1:0] s0_match;
    genvar g0;
    generate
        for (g0 = 0; g0 < TLBNUM; g0 = g0 + 1) begin : gen_s0_match
            assign s0_match[g0] = (tlb_vpn2[g0] == s0_vpn2) &&
                                  (tlb_g[g0] || (tlb_asid[g0] == s0_asid));
        end
    endgenerate

    assign s0_found = |s0_match;

    // 优先编码器：找到第一个匹配项
    reg [$clog2(TLBNUM)-1:0] s0_index_r;
    always @(*) begin
        s0_index_r = 0;
        for (i = 0; i < TLBNUM; i = i + 1) begin
            if (s0_match[i]) begin
                s0_index_r = i;
            end
        end
    end
    assign s0_index = s0_index_r;

    // 根据odd_page选择偶数页或奇数页的信息
    assign s0_pfn = s0_odd_page ? tlb_pfn1[s0_index] : tlb_pfn0[s0_index];
    assign s0_c   = s0_odd_page ? tlb_c1[s0_index]   : tlb_c0[s0_index];
    assign s0_d   = s0_odd_page ? tlb_d1[s0_index]   : tlb_d0[s0_index];
    assign s0_v   = s0_odd_page ? tlb_v1[s0_index]   : tlb_v0[s0_index];

    // 查询端口1：组合逻辑查询
    wire [TLBNUM-1:0] s1_match;
    genvar g1;
    generate
        for (g1 = 0; g1 < TLBNUM; g1 = g1 + 1) begin : gen_s1_match
            assign s1_match[g1] = (tlb_vpn2[g1] == s1_vpn2) &&
                                  (tlb_g[g1] || (tlb_asid[g1] == s1_asid));
        end
    endgenerate

    assign s1_found = |s1_match;

    // 优先编码器：找到第一个匹配项
    reg [$clog2(TLBNUM)-1:0] s1_index_r;
    always @(*) begin
        s1_index_r = 0;
        for (i = 0; i < TLBNUM; i = i + 1) begin
            if (s1_match[i]) begin
                s1_index_r = i;
            end
        end
    end
    assign s1_index = s1_index_r;

    // 根据odd_page选择偶数页或奇数页的信息
    assign s1_pfn = s1_odd_page ? tlb_pfn1[s1_index] : tlb_pfn0[s1_index];
    assign s1_c   = s1_odd_page ? tlb_c1[s1_index]   : tlb_c0[s1_index];
    assign s1_d   = s1_odd_page ? tlb_d1[s1_index]   : tlb_d0[s1_index];
    assign s1_v   = s1_odd_page ? tlb_v1[s1_index]   : tlb_v0[s1_index];

endmodule
