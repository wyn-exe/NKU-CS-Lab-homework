// Simple wrapper that adapts the ls132r core SRAM interface to the
// req/ack style ports used by soc_sram_lite_top.
module mycpu_top(
    input         clk,
    input         resetn,  // low active

    output        inst_sram_req,
    output        inst_sram_wr,
    output [1 :0] inst_sram_size,
    output [3 :0] inst_sram_wstrb,
    output [31:0] inst_sram_addr,
    output [31:0] inst_sram_wdata,
    input         inst_sram_addr_ok,
    input         inst_sram_data_ok,
    input  [31:0] inst_sram_rdata,
    
    output        data_sram_req,
    output        data_sram_wr,
    output [1 :0] data_sram_size,
    output [3 :0] data_sram_wstrb,
    output [31:0] data_sram_addr,
    output [31:0] data_sram_wdata,
    input         data_sram_addr_ok,
    input         data_sram_data_ok,
    input  [31:0] data_sram_rdata,

    //debug interface
    output [31:0] debug_wb_pc,
    output [3 :0] debug_wb_rf_wen,
    output [4 :0] debug_wb_rf_wnum,
    output [31:0] debug_wb_rf_wdata
);
// core <-> wrapper signals
wire        core_inst_en;
wire [3 :0] core_inst_wen;
wire [31:0] core_inst_addr;
wire [31:0] core_inst_wdata;
wire [31:0] core_inst_rdata;

wire        core_data_en;
wire [3 :0] core_data_wen;
wire [31:0] core_data_addr;
wire [31:0] core_data_wdata;
wire [31:0] core_data_rdata;

// Instantiate the provided CPU core
ls132r_top u_ls132r_core(
    .clk              (clk              ),
    .resetn           (resetn           ),

    .inst_sram_en     (core_inst_en     ),
    .inst_sram_wen    (core_inst_wen    ),
    .inst_sram_addr   (core_inst_addr   ),
    .inst_sram_wdata  (core_inst_wdata  ),
    .inst_sram_rdata  (core_inst_rdata  ),
    
    .data_sram_en     (core_data_en     ),
    .data_sram_wen    (core_data_wen    ),
    .data_sram_addr   (core_data_addr   ),
    .data_sram_wdata  (core_data_wdata  ),
    .data_sram_rdata  (core_data_rdata  ),

    //debug interface
    .debug_wb_pc      (debug_wb_pc      ),
    .debug_wb_rf_wen  (debug_wb_rf_wen  ),
    .debug_wb_rf_wnum (debug_wb_rf_wnum ),
    .debug_wb_rf_wdata(debug_wb_rf_wdata)
);

// Convert core-style byte enable to size encoding expected by sram_wrap
function [1:0] wstrb_to_size;
    input [3:0] strobe;
    begin
        case (strobe)
            4'b0001, 4'b0010, 4'b0100, 4'b1000: wstrb_to_size = 2'd0; // byte
            4'b0011, 4'b0110, 4'b1100:          wstrb_to_size = 2'd1; // half
            default:                             wstrb_to_size = 2'd2; // word/aligned fetch
        endcase
    end
endfunction

// Instruction channel mapping
assign inst_sram_req   = core_inst_en;
assign inst_sram_wr    = |core_inst_wen;
assign inst_sram_wstrb = core_inst_wen;
assign inst_sram_size  = wstrb_to_size(core_inst_wen);
assign inst_sram_addr  = core_inst_addr;
assign inst_sram_wdata = core_inst_wdata;
assign core_inst_rdata = inst_sram_rdata;

// Data channel mapping
assign data_sram_req   = core_data_en;
assign data_sram_wr    = |core_data_wen;
assign data_sram_wstrb = core_data_wen;
assign data_sram_size  = wstrb_to_size(core_data_wen);
assign data_sram_addr  = core_data_addr;
assign data_sram_wdata = core_data_wdata;
assign core_data_rdata = data_sram_rdata;

// The addr_ok/data_ok handshakes are always accepted by the core, so no extra gating is required here.

endmodule
