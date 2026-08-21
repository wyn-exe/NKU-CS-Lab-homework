`ifndef EXCEPTION_DEFINES_VH
`define EXCEPTION_DEFINES_VH
    `define EXC_ENTER_ADDR 32'h0000_0000
    `define EXC_CODE_ADEL  5'd4
    `define EXC_CODE_ADES  5'd5
    `define EXC_CODE_SYS   5'd8
    `define EXC_CODE_RI    5'd10
    `define EXC_CODE_INT   5'd0
    `define EXC_CODE_BP    5'd9
    `define EXC_CODE_OV   5'd12
`endif