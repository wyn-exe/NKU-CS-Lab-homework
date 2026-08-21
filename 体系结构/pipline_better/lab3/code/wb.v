`timescale 1ns / 1ps
//*************************************************************************
//   > 文件名: wb.v
//   > 描述  : 五级流水 CPU 写回级，含 HI/LO、CP0 与异常/中断处理
//*************************************************************************
`include "exception_defines.vh"
module wb(
      input          WB_valid,
      input  [154:0] MEM_WB_bus_r,
      output         rf_wen,
      output [  4:0] rf_wdest,
      output [ 31:0] rf_wdata,
      output         WB_over,
      input          clk,
      input          resetn,
      input  [ 5:0]  int_in,     // 外部中断请求
      output [ 32:0] exc_bus,
      output [  4:0] WB_wdest,
      output         cancel,
      output [ 31:0] WB_pc,
      output [ 31:0] HI_data,
      output [ 31:0] LO_data
);
      //=========================================================================
      // MEM->WB 总线解包
      //=========================================================================
      wire [31:0] mem_result;
      wire [31:0] lo_result;
      wire        hi_write;
      wire        lo_write;
      wire        wen;
      wire [4:0]  wdest;
      wire        mfhi;
      wire        mflo;
      wire        mtc0;
      wire        mfc0;
      wire [7 :0] cp0r_addr;
      wire        eret;
      wire [31:0] pc;
      wire        exc_valid_i;
      wire [4:0]  exc_code_i;
      wire [31:0] bad_addr_i;
      assign {wen,
              wdest,
              mem_result,
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
              pc} = MEM_WB_bus_r;

      wire exc_commit,eret_commit;
      wire [4:0] exc_code_wb;
      //=========================================================================
      // HI/LO 寄存器
      //=========================================================================
      reg [31:0] hi;
      reg [31:0] lo;
      always @(posedge clk) begin
          if (hi_write && ~(exc_valid_i & WB_valid)) hi <= mem_result;
      end
      always @(posedge clk) begin
          if (lo_write && ~(exc_valid_i & WB_valid)) lo <= lo_result;
      end

      //=========================================================================
      // CP0 寄存器：Status/CAUSE/EPC/BadVAddr/Count/Compare
      //=========================================================================
      reg  status_exl_r;
      reg  status_ie_r;
      reg  [7:0] status_im_r;

      reg  [4:0] cause_exc_code_r;
      reg  [7:0] cause_ip_r;    // IP7=定时器，IP5:0=外部中断
      reg  [31:0] epc_r;
      reg  [31:0] badvaddr_r;
      reg  [31:0] count_r;
      reg  [31:0] compare_r;

      // mtc0 写使能
      wire status_wen   = mtc0 & (cp0r_addr=={5'd12,3'd0});
      wire epc_wen      = mtc0 & (cp0r_addr=={5'd14,3'd0});
      wire badvaddr_wen = mtc0 & (cp0r_addr=={5'd8 ,3'd0});
      wire count_wen    = mtc0 & (cp0r_addr=={5'd9 ,3'd0});
      wire compare_wen  = mtc0 & (cp0r_addr=={5'd11,3'd0});

      // 读端口
      wire [31:0] cp0r_status = {16'd0,status_im_r,6'd0,status_exl_r,status_ie_r};
      wire [31:0] cp0r_cause  = {16'd0,cause_ip_r,1'b0,cause_exc_code_r,2'd0};
      wire [31:0] cp0r_epc    = epc_r;
      wire [31:0] cp0r_rdata  = (cp0r_addr=={5'd8 ,3'd0}) ? badvaddr_r  :
                                (cp0r_addr=={5'd9 ,3'd0}) ? count_r     :
                                (cp0r_addr=={5'd11,3'd0}) ? compare_r   :
                                (cp0r_addr=={5'd12,3'd0}) ? cp0r_status :
                                (cp0r_addr=={5'd13,3'd0}) ? cp0r_cause  :
                                (cp0r_addr=={5'd14,3'd0}) ? cp0r_epc    : 32'd0;

      // BadVAddr
      always @(posedge clk) begin
          if (!resetn) badvaddr_r <= 32'd0;
          else if (badvaddr_wen) badvaddr_r <= mem_result;
          else if (exc_valid_i & WB_valid &&
                  ((exc_code_i==`EXC_CODE_ADEL) || (exc_code_i==`EXC_CODE_ADES)))
              badvaddr_r <= bad_addr_i;
      end

      // Status: IE/EXL/IM
      always @(posedge clk) begin
          if (!resetn || eret_commit) begin
              status_exl_r <= 1'b0;
              status_ie_r  <= 1'b0;
              status_im_r  <= 8'd0;
          end else if (exc_commit) begin
              status_exl_r <= 1'b1;
          end else if (status_wen) begin
              status_ie_r  <= mem_result[0];
              status_exl_r <= mem_result[1];
              status_im_r  <= mem_result[15:8];
          end
      end

      // Count/Compare
      always @(posedge clk) begin
          if (!resetn)       count_r <= 32'd0;
          else if (count_wen)count_r <= mem_result;
          else               count_r <= count_r + 1'b1;
      end
      always @(posedge clk) begin
          if (!resetn)          compare_r <= 32'd0;
          else if (compare_wen) compare_r <= mem_result;
      end

      // Cause.IP：外部中断 + 计时器 TI（bit7）
      always @(posedge clk) begin
          if (!resetn) begin
              cause_ip_r <= 8'd0;
          end else begin
              cause_ip_r[5:0] <= int_in;
              // 计时器中断：count==compare 且 compare!=0
              if (compare_r!=32'd0 && count_r==compare_r)
                  cause_ip_r[7] <= 1'b1;
              else if (compare_wen)
                  cause_ip_r[7] <= 1'b0; // 写 Compare 清 TI
          end
      end

      // Cause.ExcCode
      always @(posedge clk) begin
          if (!resetn)          cause_exc_code_r <= 5'd0;
          else if (exc_commit)  cause_exc_code_r <= exc_code_wb;
      end

      // EPC
      always @(posedge clk) begin
          if (!resetn)           epc_r <= 32'd0;
          else if (exc_commit)   epc_r <= pc;
          else if (epc_wen)      epc_r <= mem_result;
      end

      //=========================================================================
      // 异常/中断仲裁
      //=========================================================================
      assign eret_commit = eret & WB_valid;
      wire [7:0] int_pending = cause_ip_r & status_im_r;
      wire irq_req  = status_ie_r & ~status_exl_r & |int_pending & WB_valid;
      wire exc_req  = exc_valid_i & WB_valid;
      assign exc_commit   = irq_req | exc_req;
      assign exc_code_wb = irq_req ? `EXC_CODE_INT : exc_code_i;

      // cancel / rf 写屏蔽
      assign cancel   = (exc_commit | eret_commit) & WB_over;
      assign WB_over  = WB_valid;
      assign rf_wen   = wen & WB_over & ~exc_commit;

      // 写回数据/目的
      assign rf_wdest = wdest;
      assign rf_wdata = mfhi ? hi :
                        mflo ? lo :
                        mfc0 ? cp0r_rdata : mem_result;

      // 异常向量
      wire        exc_valid = exc_commit | eret_commit;
      wire [31:0] exc_pc    = exc_commit ? `EXC_ENTER_ADDR : cp0r_epc;
      assign exc_bus   = {exc_valid,exc_pc};

      // 旁路/观测
      assign WB_wdest = rf_wdest & {5{WB_valid}};
      assign WB_pc    = pc;
      assign HI_data  = hi;
      assign LO_data  = lo;
  endmodule