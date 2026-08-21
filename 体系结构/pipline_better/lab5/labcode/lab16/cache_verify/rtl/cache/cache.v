`timescale 1ns / 1ps
//*****************************************************************************
//   > 文件名: cache.v
//   > 描述  : 统一Cache模块，可配置为ICache或DCache
//   > 作者  : Lab5 Implementation
//   > 日期  : 2026-01-05
//*****************************************************************************

module cache #(
    parameter INDEX_WIDTH = 8,    // 索引位宽，决定cache行数 (2^8 = 256行)
    parameter OFFSET_WIDTH = 4,   // 偏移位宽，决定cache行大小 (2^4 = 16字节)
    parameter WAY_NUM = 2         // 路数，2路组相联
)(
    input  wire        clk_g,
    input  wire        resetn,

    // CPU侧接口
    input  wire        valid,      // 请求有效
    input  wire        op,         // 操作类型：0=读，1=写
    input  wire [INDEX_WIDTH-1:0]  index,   // 索引
    input  wire [19:0] tag,        // 标签
    input  wire [OFFSET_WIDTH-1:0] offset,  // 偏移
    input  wire [ 3:0] wstrb,      // 写字节使能
    input  wire [31:0] wdata,      // 写数据

    output wire        addr_ok,    // 地址握手信号
    output wire        data_ok,    // 数据握手信号
    output wire [31:0] rdata,      // 读数据

    // AXI读接口
    output wire        rd_req,     // 读请求
    output wire [ 2:0] rd_type,    // 读类型
    output wire [31:0] rd_addr,    // 读地址
    input  wire        rd_rdy,     // 读就绪
    input  wire        ret_valid,  // 返回数据有效
    input  wire        ret_last,   // 返回数据最后一拍
    input  wire [31:0] ret_data,   // 返回数据

    // AXI写接口
    output wire        wr_req,     // 写请求
    output wire [ 2:0] wr_type,    // 写类型
    output wire [31:0] wr_addr,    // 写地址
    output wire [ 3:0] wr_wstrb,   // 写字节使能
    output wire [127:0] wr_data,   // 写数据（整个cache行）
    input  wire        wr_rdy      // 写就绪
);

    localparam TAG_WIDTH = 20;
    localparam LINE_SIZE = 4;      // 每行4个字（16字节）
    localparam SET_NUM = 1 << INDEX_WIDTH;  // cache组数

    // Cache状态机
    localparam IDLE       = 3'b000;
    localparam LOOKUP     = 3'b001;
    localparam MISS       = 3'b010;
    localparam REPLACE    = 3'b011;
    localparam REFILL     = 3'b100;

    reg [2:0] state, next_state;

    // Cache存储阵列
    reg [TAG_WIDTH-1:0] tag_array   [WAY_NUM-1:0][SET_NUM-1:0];
    reg                 valid_array [WAY_NUM-1:0][SET_NUM-1:0];
    reg                 dirty_array [WAY_NUM-1:0][SET_NUM-1:0];
    reg [31:0]          data_array  [WAY_NUM-1:0][SET_NUM-1:0][LINE_SIZE-1:0];

    // LRU替换策略（简化版：1位伪LRU）
    reg [WAY_NUM-1:0] lru_array [SET_NUM-1:0];

    // 请求寄存器
    reg        req_op;
    reg [INDEX_WIDTH-1:0] req_index;
    reg [TAG_WIDTH-1:0]   req_tag;
    reg [OFFSET_WIDTH-1:0] req_offset;
    reg [ 3:0] req_wstrb;
    reg [31:0] req_wdata;

    // 命中检测
    wire [WAY_NUM-1:0] way_hit;
    wire hit;
    wire [WAY_NUM-1:0] hit_way;
    integer w;

    genvar i;
    generate
        for (i = 0; i < WAY_NUM; i = i + 1) begin : gen_way_hit
            assign way_hit[i] = valid_array[i][req_index] &&
                               (tag_array[i][req_index] == req_tag);
        end
    endgenerate

    assign hit = |way_hit;
    assign hit_way = way_hit;

    // 选择替换路
    wire [WAY_NUM-1:0] replace_way;
    assign replace_way = lru_array[req_index];

    // Refill计数器
    reg [1:0] refill_cnt;
    wire refill_done = (refill_cnt == 2'b11) && ret_valid && ret_last;

    // 状态机
    always @(posedge clk_g) begin
        if (!resetn) begin
            state <= IDLE;
        end else begin
            state <= next_state;
        end
    end

    always @(*) begin
        case (state)
            IDLE: begin
                if (valid) begin
                    next_state = LOOKUP;
                end else begin
                    next_state = IDLE;
                end
            end
            LOOKUP: begin
                if (hit) begin
                    next_state = IDLE;
                end else begin
                    next_state = MISS;
                end
            end
            MISS: begin
                // 检查是否需要写回
                if (dirty_array[replace_way][req_index] &&
                    valid_array[replace_way][req_index]) begin
                    next_state = REPLACE;
                end else begin
                    next_state = REFILL;
                end
            end
            REPLACE: begin
                if (wr_rdy) begin
                    next_state = REFILL;
                end else begin
                    next_state = REPLACE;
                end
            end
            REFILL: begin
                if (refill_done) begin
                    next_state = IDLE;
                end else begin
                    next_state = REFILL;
                end
            end
            default: next_state = IDLE;
        endcase
    end

    // 请求锁存
    always @(posedge clk_g) begin
        if (!resetn) begin
            req_op     <= 1'b0;
            req_index  <= 0;
            req_tag    <= 0;
            req_offset <= 0;
            req_wstrb  <= 4'b0;
            req_wdata  <= 32'b0;
        end else if (state == IDLE && valid) begin
            req_op     <= op;
            req_index  <= index;
            req_tag    <= tag;
            req_offset <= offset;
            req_wstrb  <= wstrb;
            req_wdata  <= wdata;
        end
    end

    // Refill计数器
    always @(posedge clk_g) begin
        if (!resetn) begin
            refill_cnt <= 2'b00;
        end else if (state == REFILL && ret_valid) begin
            refill_cnt <= refill_cnt + 1'b1;
        end else if (state != REFILL) begin
            refill_cnt <= 2'b00;
        end
    end

    // Cache数据更新
    integer way_idx;
    always @(posedge clk_g) begin
        if (!resetn) begin
            for (w = 0; w < WAY_NUM; w = w + 1) begin
                for (way_idx = 0; way_idx < SET_NUM; way_idx = way_idx + 1) begin
                    valid_array[w][way_idx] <= 1'b0;
                    dirty_array[w][way_idx] <= 1'b0;
                end
            end
        end else begin
            // 命中写
            if (state == LOOKUP && hit && req_op) begin
                for (w = 0; w < WAY_NUM; w = w + 1) begin
                    if (way_hit[w]) begin
                        if (req_wstrb[0]) data_array[w][req_index][req_offset[3:2]][ 7: 0] <= req_wdata[ 7: 0];
                        if (req_wstrb[1]) data_array[w][req_index][req_offset[3:2]][15: 8] <= req_wdata[15: 8];
                        if (req_wstrb[2]) data_array[w][req_index][req_offset[3:2]][23:16] <= req_wdata[23:16];
                        if (req_wstrb[3]) data_array[w][req_index][req_offset[3:2]][31:24] <= req_wdata[31:24];
                        dirty_array[w][req_index] <= 1'b1;
                    end
                end
            end
            // Refill
            else if (state == REFILL && ret_valid) begin
                for (w = 0; w < WAY_NUM; w = w + 1) begin
                    if (replace_way[w]) begin
                        data_array[w][req_index][refill_cnt] <= ret_data;
                        if (ret_last) begin
                            tag_array[w][req_index]   <= req_tag;
                            valid_array[w][req_index] <= 1'b1;
                            dirty_array[w][req_index] <= 1'b0;
                        end
                    end
                end
            end
        end
    end

    // LRU更新
    always @(posedge clk_g) begin
        if (!resetn) begin
            for (way_idx = 0; way_idx < SET_NUM; way_idx = way_idx + 1) begin
                lru_array[way_idx] <= 2'b01;  // 初始指向way 0
            end
        end else if (state == LOOKUP && hit) begin
            // 更新LRU：将命中的路标记为最近使用
            lru_array[req_index] <= ~hit_way;
        end else if (state == REFILL && refill_done) begin
            // Refill完成后更新LRU
            lru_array[req_index] <= ~replace_way;
        end
    end

    // 输出信号
    assign addr_ok = (state == IDLE) && valid;
    assign data_ok = (state == LOOKUP) && hit;

    // 读数据输出
    reg [31:0] rdata_r;
    always @(*) begin
        rdata_r = 32'b0;
        for (w = 0; w < WAY_NUM; w = w + 1) begin
            if (way_hit[w]) begin
                rdata_r = data_array[w][req_index][req_offset[3:2]];
            end
        end
    end
    assign rdata = rdata_r;

    // AXI读接口
    assign rd_req  = (state == MISS && !dirty_array[replace_way][req_index]) ||
                     (state == REFILL && !rd_rdy);
    assign rd_type = 3'b100;  // burst读，4个字
    assign rd_addr = {req_tag, req_index, 4'b0};

    // AXI写接口
    reg [127:0] wr_data_r;
    always @(*) begin
        wr_data_r = 128'b0;
        for (w = 0; w < WAY_NUM; w = w + 1) begin
            if (replace_way[w]) begin
                wr_data_r = {data_array[w][req_index][3],
                            data_array[w][req_index][2],
                            data_array[w][req_index][1],
                            data_array[w][req_index][0]};
            end
        end
    end

    assign wr_req  = (state == REPLACE);
    assign wr_type = 3'b100;  // burst写，4个字
    assign wr_addr = {tag_array[replace_way][req_index], req_index, 4'b0};
    assign wr_wstrb = 4'b1111;
    assign wr_data = wr_data_r;

endmodule
