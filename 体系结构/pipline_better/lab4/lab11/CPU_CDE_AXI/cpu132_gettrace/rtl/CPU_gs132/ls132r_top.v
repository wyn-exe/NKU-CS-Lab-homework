/*------------------------------------------------------------------------------
--------------------------------------------------------------------------------
Copyright (c) 2016, Loongson Technology Corporation Limited.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this 
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of Loongson Technology Corporation Limited nor the names of 
its contributors may be used to endorse or promote products derived from this 
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL LOONGSON TECHNOLOGY CORPORATION LIMITED BE LIABLE
TO ANY PARTY FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

`include "ls132r_define.h"

module ls132r_top(
    input            clk,
    input            resetn,            //low active

    output           inst_sram_en,
    output  [ 3:0]   inst_sram_wen,
    output  [31:0]   inst_sram_addr,
    output  [31:0]   inst_sram_wdata,
    input   [31:0]   inst_sram_rdata,
    
    output           data_sram_en,
    output  [ 3:0]   data_sram_wen,
    output  [31:0]   data_sram_addr,
    output  [31:0]   data_sram_wdata,
    input   [31:0]   data_sram_rdata,

    //axi master interface
    output  [3 :0]   arid,
    output  [31:0]   araddr,
    output  [7 :0]   arlen,
    output  [2 :0]   arsize,
    output  [1 :0]   arburst,
    output  [1 :0]   arlock,
    output  [3 :0]   arcache,
    output  [2 :0]   arprot,
    output           arvalid,
    input            arready,

    input   [3 :0]   rid,
    input   [31:0]   rdata,
    input   [1 :0]   rresp,
    input            rlast,
    input            rvalid,
    output           rready,

    output  [3 :0]   awid,
    output  [31:0]   awaddr,
    output  [7 :0]   awlen,
    output  [2 :0]   awsize,
    output  [1 :0]   awburst,
    output  [1 :0]   awlock,
    output  [3 :0]   awcache,
    output  [2 :0]   awprot,
    output           awvalid,
    input            awready,

    output  [3 :0]   wid,
    output  [31:0]   wdata,
    output  [3 :0]   wstrb,
    output           wlast,
    output           wvalid,
    input            wready,

    input   [3 :0]   bid,
    input   [1 :0]   bresp,
    input            bvalid,
    output           bready,

    //debug interface
    output  [31:0]   debug_wb_pc,
    output  [3 :0]   debug_wb_rf_wen,
    output  [4 :0]   debug_wb_rf_wnum,
    output  [31:0]   debug_wb_rf_wdata
);
wire           nmi_n_i;
assign nmi_n_i = 1'd1;

//output          sleeping_o;
wire          sleeping_o;

wire  [ 3:0]  inst_sram_cen;
wire          inst_sram_wr;
wire          inst_sram_ack  = 1'b1;
wire          inst_sram_rrdy = 1'b1;
assign inst_sram_en  = |inst_sram_cen;
assign inst_sram_wen = inst_sram_cen & {4{inst_sram_wr}};

wire  [ 3:0]  data_sram_cen;
wire          data_sram_wr;
wire          data_sram_ack  = 1'b1;
wire          data_sram_rrdy = 1'b1;
assign data_sram_en  = |data_sram_cen;
assign data_sram_wen = data_sram_cen & {4{data_sram_wr}};

wire aclk     = clk;
wire areset_n = resetn;
//read address channel
wire          arready_int;
wire  [3:0]   arid_int;
wire  [31:0]  araddr_int;
wire  [3:0]   arlen_int;
wire  [2:0]   arsize_int;
wire  [1:0]   arburst_int;
wire  [1:0]   arlock_int;
wire  [3:0]   arcache_int;
wire  [2:0]   arprot_int;
wire          arvalid_int;

//read data channel
wire  [3:0]   rid_int;
wire  [31:0]  rdata_int;
wire  [1:0]   rresp_int;
wire          rlast_int;
wire          rvalid_int;
wire          rready_int;

//write address channel
wire          awready_int;
wire  [3:0]   awid_int;
wire  [31:0]  awaddr_int;
wire  [3:0]   awlen_int;
wire  [2:0]   awsize_int;
wire  [1:0]   awburst_int;
wire  [1:0]   awlock_int;
wire  [3:0]   awcache_int;
wire  [2:0]   awprot_int;
wire          awvalid_int;

//write data channel
wire          wready_int;
wire  [3:0]   wid_int;
wire  [31:0]  wdata_int;
wire  [3:0]   wstrb_int;
wire          wlast_int;
wire          wvalid_int;

//write response channel
wire  [3:0]   bid_int;
wire  [1:0]   bresp_int;
wire          bvalid_int;
wire          bready_int;

// tie internal AXI signals to top-level ports
assign arid        = arid_int;
assign araddr      = araddr_int;
assign arlen       = {4'd0, arlen_int};
assign arsize      = arsize_int;
assign arburst     = arburst_int;
assign arlock      = arlock_int;
assign arcache     = arcache_int;
assign arprot      = arprot_int;
assign arvalid     = arvalid_int;
assign arready_int = arready;

assign rid_int     = rid;
assign rdata_int   = rdata;
assign rresp_int   = rresp;
assign rlast_int   = rlast;
assign rvalid_int  = rvalid;
assign rready      = rready_int;

assign awid        = awid_int;
assign awaddr      = awaddr_int;
assign awlen       = {4'd0, awlen_int};
assign awsize      = awsize_int;
assign awburst     = awburst_int;
assign awlock      = awlock_int;
assign awcache     = awcache_int;
assign awprot      = awprot_int;
assign awvalid     = awvalid_int;
assign awready_int = awready;

assign wid         = wid_int;
assign wdata       = wdata_int;
assign wstrb       = wstrb_int;
assign wlast       = wlast_int;
assign wvalid      = wvalid_int;
assign wready_int  = wready;

assign bid_int     = bid;
assign bresp_int   = bresp;
assign bvalid_int  = bvalid;
assign bready      = bready_int;

wire          ejtag_trst = 1'b0;
wire          ejtag_tck  = 1'b0;
wire          ejtag_tdi  = 1'b0;
wire          ejtag_tms  = 1'b0;
wire          ejtag_tdo  ;

wire          test_mode  = 1'b0;

wire                        clock;
wire                        reset;
wire                        nmi;
wire                        nmi_internal;
wire                        status_nmi;
wire [               5:0]   hw_int;
wire [    `Ltoifcbus-1:0]   inst_ifc_bus;
wire [  `Lfromifcbus-1:0]   ifc_inst_bus;
wire [    `Ltoifcbus-1:0]   data_ifc_bus;
wire [  `Lfromifcbus-1:0]   ifc_data_bus;
wire [    `Lcpustbus-1:0]   cpu_status;
wire [       `Lexbus-1:0]   exbus;
wire [       `Lbrbus-1:0]   brbus;
wire [    `Lexestbus-1:0]   exe_status;
wire [    `Ldecstbus-1:0]   dec_status;
wire [       `Lirbus-1:0]   irbus;
wire                        ex_int;
wire [       `Lwbbus-1:0]   wbbus;
wire [    `Lissuebus-1:0]   issuebus;

wire                        wait_sleep;
wire                        idle_in_ir;
wire                        idle_in_rs;
wire                        idle_in_ifc;

wire                        trst; 
wire [              31:0]   data_from_tap;
wire                        pracc_from_tap;
wire                        prrst_from_tap;
wire                        proben_from_tap;
wire                        trap_from_tap;
wire                        ejtagbrk_from_tap;
wire [              31:0]   drseg_res_dcr;
wire [              31:0]   drseg_res_hb;
wire [              31:0]   addr_to_tap;
wire [              31:0]   data_to_tap;
wire [               1:0]   width_to_tap;
wire                        write_to_tap;
wire                        pracc_to_tap;
wire                        ejtagbrk_to_tap;
wire                        reset_to_tap;
wire                        debugmode_to_tap;
wire                        prrst_to_core;
wire                        proben_to_core;
wire                        dint;
wire                        debug_mode;
wire                        ejtagbrk;
wire                        ejtag_inte;
wire                        ejtagboot;
wire                        commit_ex;
wire                        proben;
wire                        probtrap;
wire                        hb_dib;
wire [    `Licompbus-1:0]   hb_icompbus;
wire [`Ldmsegiresbus-1:0]   dmseg_iresbus;
wire [`Ldmsegireqbus-1:0]   dmseg_ireqbus;
wire [`Ldmsegdresbus-1:0]   dmseg_dresbus;
wire [`Ldmsegdreqbus-1:0]   dmseg_dreqbus;
wire [ `Ldrsegreqbus-1:0]   dcr_reqbus;
wire [ `Ldrsegreqbus-1:0]   hb_reqbus;


assign wait_sleep   = dec_status[2];
assign idle_in_ir   = dec_status[36];
assign idle_in_rs   = exe_status[2];

assign sleeping_o   = wait_sleep && idle_in_ir && idle_in_rs && idle_in_ifc;

reg nmi_r;
always @(posedge clock)
  nmi_r <= reset        ? 1'b0 :
           status_nmi   ? 1'b0 :
           nmi_internal ? 1'b1 :
                          nmi_r;


ls132r_interface
    u_interface(
      .int_n_i      (6'b1111_11 ),
      .nmi_n_i      (nmi_n_i    ),
    
      .aclk         (aclk       ),
      .areset_n     (areset_n   ),
    
      .arid         (arid_int   ),
      .araddr       (araddr_int ),
      .arlen        (arlen_int  ),
      .arsize       (arsize_int ),
      .arburst      (arburst_int),
      .arlock       (arlock_int ),
      .arcache      (arcache_int),
      .arprot       (arprot_int ),
      .arvalid      (arvalid_int),
      .arready      (arready_int),
    
      .rid          (rid_int    ),
      .rdata        (rdata_int  ),
      .rresp        (rresp_int  ),
      .rlast        (rlast_int  ),
      .rvalid       (rvalid_int ),
      .rready       (rready_int ),
    
      .awid         (awid_int   ),
      .awaddr       (awaddr_int ),
      .awlen        (awlen_int  ),
      .awsize       (awsize_int ),
      .awburst      (awburst_int),
      .awlock       (awlock_int ),
      .awcache      (awcache_int),
      .awprot       (awprot_int ),
      .awvalid      (awvalid_int),
      .awready      (awready_int),
    
      .wid          (wid_int    ),
      .wdata        (wdata_int  ),
      .wstrb        (wstrb_int  ),
      .wlast        (wlast_int  ),
      .wvalid       (wvalid_int ),
      .wready       (wready_int ),
    
      .bid          (bid_int    ),
      .bresp        (bresp_int  ),
      .bvalid       (bvalid_int ),
      .bready       (bready_int ),
    
      .inst_sram_cen    (inst_sram_cen  ),
      .inst_sram_wr     (inst_sram_wr   ),
      .inst_sram_addr   (inst_sram_addr ),
      .inst_sram_wdata  (inst_sram_wdata),
      .inst_sram_ack    (inst_sram_ack  ),
      .inst_sram_rrdy   (inst_sram_rrdy ),
      .inst_sram_rdata  (inst_sram_rdata),
    
      .data_sram_cen    (data_sram_cen  ),
      .data_sram_wr     (data_sram_wr   ),
      .data_sram_addr   (data_sram_addr ),
      .data_sram_wdata  (data_sram_wdata),
      .data_sram_ack    (data_sram_ack  ),
      .data_sram_rrdy   (data_sram_rrdy ),
      .data_sram_rdata  (data_sram_rdata),
    
      .prrst_i      (prrst_to_core  ),
      .clock_o      (clock          ),
      .reset_o      (reset          ),
      .hw_int_o     (hw_int         ),
      .nmi_internal_o(nmi_internal  ),
    
      .idle_in_ifc_o(idle_in_ifc    ),

      .inst_ifc_i   (inst_ifc_bus   ),
      .ifc_inst_o   (ifc_inst_bus   ),

      .data_ifc_i   (data_ifc_bus   ),
      .ifc_data_o   (ifc_data_bus   )
    );



ls132r_fetch_stage
    u_fetch(
      .clock          (clock         ),
      .reset          (reset         ),
    
      .cpu_status_i   (cpu_status    ),
      .exbus_i        (exbus         ),
      .brbus_i        (brbus         ),
      .exe_status_i   (exe_status    ),
      .dec_status_i   (dec_status    ),
      .irbus_o        (irbus         ),
    
      .ifc_inst_i     (ifc_inst_bus  ),
      .inst_ifc_o     (inst_ifc_bus  ) 
     ,.ejtagboot_i    (ejtagboot     )
     ,.proben_i       (proben        )
     ,.probtrap_i     (probtrap      )
     ,.dmseg_ires_i   (dmseg_iresbus )
     ,.dmseg_ireq_o   (dmseg_ireqbus )
     ,.hb_dib_i       (hb_dib        )
     ,.hb_icompbus_o  (hb_icompbus   )
    );



ls132r_decode_stage
    u_decode(
      .clock           (clock      ),
      .reset           (reset      ),
      .nmi_i           (nmi        ),
      .cpu_status_i    (cpu_status ),
      .exbus_i         (exbus      ),
      .brbus_i         (brbus      ),
      .ex_int_i        (ex_int     ),
      .irbus_i         (irbus      ),
      .wbbus_i         (wbbus      ),
      .issuebus_o      (issuebus   ),
      .dec_status_o    (dec_status ) 
     ,.ejtag_inte_i    (ejtag_inte )
    );



ls132r_execute_stage
    u_execute(
      .clock           (clock         ),
      .reset           (reset         ),
      .issuebus_i      (issuebus      ),
      .dec_status_i    (dec_status    ),
      .hw_int_i        (hw_int        ),
      .cpu_status_o    (cpu_status    ),
      .exbus_o         (exbus         ),
      .brbus_o         (brbus         ),
      .ex_int_o        (ex_int        ),
      .wbbus_o         (wbbus         ),
      .exe_status_o    (exe_status    ),
      .status_nmi_o    (status_nmi    ),
    
      .ifc_data_i      (ifc_data_bus  ),
      .data_ifc_o      (data_ifc_bus  )
     ,.ejtagboot_i     (ejtagboot     )
     ,.ejtagbrk_i      (ejtagbrk      )
     ,.proben_i        (proben        )
     ,.dmseg_dres_i    (dmseg_dresbus )
     ,.dmseg_dreq_o    (dmseg_dreqbus )
     ,.drseg_res_dcr_i (drseg_res_dcr )
     ,.dcr_req_o       (dcr_reqbus    )
     ,.drseg_res_hb_i  (drseg_res_hb  )
     ,.hb_req_o        (hb_reqbus     ) 

     //debug
     ,
     .debug_wb_pc      (debug_wb_pc      ),
     .debug_wb_rf_wen  (debug_wb_rf_wen  ),
     .debug_wb_rf_wnum (debug_wb_rf_wnum ),
     .debug_wb_rf_wdata(debug_wb_rf_wdata)
    );

assign commit_ex  = exbus[0];
assign ejtagboot  = ejtagbrk;
assign debug_mode = cpu_status[13];
assign dint       = cpu_status[16];

ls132r_ejtag_rstgen
    u_ejtag_rstgen(
      .testmode  (test_mode  ),
      .tck       (ejtag_tck  ), 
      .trst_in   (ejtag_trst ), 
      .trst_out  (trst       )
    );

ls132r_ejtag_tap
    u_ejtag_tap(
      .tck             (ejtag_tck         ),
      .trst            (trst              ),
      .tms             (ejtag_tms         ),
      .tdi             (ejtag_tdi         ),
      .tdo             (ejtag_tdo         ),

      .dmseg_addr      (addr_to_tap       ),
      .dmseg_rdata     (data_to_tap       ),
      .dmseg_wdata     (data_from_tap     ),
      .rocc_in         (reset_to_tap      ),
      .dmseg_be_in     (width_to_tap      ),
      .prnw            (write_to_tap      ),
      .pracc_in        (pracc_to_tap      ),
      .ejtagbrk_in     (ejtagbrk_to_tap   ),
      .dm              (debugmode_to_tap  ),
      .pracc_out       (pracc_from_tap    ),
      .prrst           (prrst_from_tap    ),
      .proben          (proben_from_tap   ),
      .probtrap        (trap_from_tap     ),
      .ejtagbrk_out    (ejtagbrk_from_tap ) 
    );

ls132r_ejtag_tap_buffer 
    u_ejtag_tap_buffer(
      .clock                (clock              ),
      .reset                (reset              ),
      .softreset            (1'b0               ),
      .commit_ex            (commit_ex          ),
      .test_mode            (test_mode          ),
      .ejtagbrk_from_core   (dint               ),
      .debugmode_from_core  (debug_mode         ),
      .ejtagbrk_to_core     (ejtagbrk           ),
      .prrst_to_core        (prrst_to_core      ),
      .proben_to_core       (proben_to_core     ),
      .trap_to_core         (probtrap           ),

      .dmseg_dreqbus        (dmseg_dreqbus      ),
      .dresbus_from_dmseg   (dmseg_dresbus      ),
      .dmseg_ireqbus        (dmseg_ireqbus      ),
      .iresbus_from_dmseg   (dmseg_iresbus      ),

      .pracc_to_tap         (pracc_to_tap       ),
      .pracc_from_tap       (pracc_from_tap     ),
      .addr_to_tap          (addr_to_tap        ),
      .data_to_tap          (data_to_tap        ),
      .width_to_tap         (width_to_tap       ),
      .write_to_tap         (write_to_tap       ),
      .ejtagbrk_to_tap      (ejtagbrk_to_tap    ),
      .reset_to_tap         (reset_to_tap       ),
      .debugmode_to_tap     (debugmode_to_tap   ),
      .data_from_tap        (data_from_tap      ),
      .prrst_from_tap       (prrst_from_tap     ),
      .proben_from_tap      (proben_from_tap    ),
      .trap_from_tap        (trap_from_tap      ),
      .ejtagbrk_from_tap    (ejtagbrk_from_tap  ) 
    );


ls132r_ejtag_dcr 
    u_ejtag_dcr(
      .clock            (clock          ),
      .reset            (reset          ),
      .nmi_in           (nmi_r          ),
      .proben_in        (proben_to_core ),
      .dcr_reqbus       (dcr_reqbus     ),
      .drseg_res_dcr    (drseg_res_dcr  ),
      .inte             (ejtag_inte     ),
      .nmi_out          (nmi            ),
      .proben_out       (proben         )
    );


ls132r_ejtag_hb 
    u_ejtag_hb(
      .clock            (clock          ),
      .reset            (reset          ),
      .debug_mode       (debug_mode     ),
      .exbus_i          (exbus          ),
      .hb_reqbus        (hb_reqbus      ),
      .drseg_res_hb     (drseg_res_hb   ),

      .hb_icompbus      (hb_icompbus    ),
      .hb_dit           (               ),
      .hb_dib           (hb_dib         ) 
    );
endmodule //ls132r_top
