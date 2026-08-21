`timescale 1ns / 1ps

module tb;
    // Testbench inputs
    reg clk;
    reg resetn;
    reg [4:0]  rf_addr;
    reg [31:0] mem_addr;
    reg [5:0]  int_in;          // 外部中断输入

    // Testbench observation ports
    wire [31:0] rf_data;
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

    // DUT instance
    pipeline_cpu uut (
        .clk        (clk),
        .resetn     (resetn),
        .int_in     (int_in),
        .rf_addr    (rf_addr),
        .mem_addr   (mem_addr),
        .rf_data    (rf_data),
        .mem_data   (mem_data),
        .IF_pc      (IF_pc),
        .IF_inst    (IF_inst),
        .ID_pc      (ID_pc),
        .EXE_pc     (EXE_pc),
        .MEM_pc     (MEM_pc),
        .WB_pc      (WB_pc),
        .cpu_5_valid(cpu_5_valid),
        .HI_data    (HI_data),
        .LO_data    (LO_data)
    );

    integer file;
    integer trace_file;

    // Clock/reset stimulus and logging
    initial begin
        clk      = 0;
        resetn   = 0;
        rf_addr  = 0;
        mem_addr = 0;
        int_in   = 6'b0;      // 初始化中断信号

        file = $fopen("D:/pipline_better/lab3/simulation_results_new.txt", "w");
        if (file == 0) begin
            $display("Error opening file!");
            $finish;
        end

        trace_file = $fopen("simulation_trace.txt", "w");
        if (trace_file == 0) begin
            $display("Error opening trace file!");
            $finish;
        end

        $fwrite(file, "Time\tresetn\tIF_pc\tIF_inst\tID_pc\tEXE_pc\tMEM_pc\tWB_pc\tcpu_5_valid\tHI_data\tLO_data\trf_data\tmem_data\n");
        $fwrite(trace_file, "time_ps,resetn,IF_pc,ID_pc,EXE_pc,MEM_pc,WB_pc,IF_inst,cpu_5_valid,HI_data,LO_data,rf_data,mem_data\n");

        #100;
        resetn = 1;

        #10000;

        $fwrite(file, "Final values:\n");
        $fwrite(file, "IF_pc: %h\n", IF_pc);
        $fwrite(file, "IF_inst: %h\n", IF_inst);
        $fwrite(file, "ID_pc: %h\n", ID_pc);
        $fwrite(file, "EXE_pc: %h\n", EXE_pc);
        $fwrite(file, "MEM_pc: %h\n", MEM_pc);
        $fwrite(file, "WB_pc: %h\n", WB_pc);
        $fwrite(file, "cpu_5_valid: %h\n", cpu_5_valid);
        $fwrite(file, "HI_data: %h\n", HI_data);
        $fwrite(file, "LO_data: %h\n", LO_data);
        $fwrite(file, "rf_data: %h\n", rf_data);
        $fwrite(file, "mem_data: %h\n", mem_data);

        $fwrite(trace_file, "Final values:\n");
        $fwrite(trace_file, "IF_pc: %h\n", IF_pc);
        $fwrite(trace_file, "IF_inst: %h\n", IF_inst);
        $fwrite(trace_file, "ID_pc: %h\n", ID_pc);
        $fwrite(trace_file, "EXE_pc: %h\n", EXE_pc);
        $fwrite(trace_file, "MEM_pc: %h\n", MEM_pc);
        $fwrite(trace_file, "WB_pc: %h\n", WB_pc);
        $fwrite(trace_file, "cpu_5_valid: %h\n", cpu_5_valid);
        $fwrite(trace_file, "HI_data: %h\n", HI_data);
        $fwrite(trace_file, "LO_data: %h\n", LO_data);
        $fwrite(trace_file, "rf_data: %h\n", rf_data);
        $fwrite(trace_file, "mem_data: %h\n", mem_data);

        $fclose(file);
        $fclose(trace_file);
        $finish;
    end

    // 100MHz clock
    always #5 clk = ~clk;

    // Cycle-by-cycle logging
    always @(posedge clk) begin
        $fwrite(file, "%t\t%b\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\n",
                $time, resetn, IF_pc, IF_inst, ID_pc, EXE_pc, MEM_pc, WB_pc,
                cpu_5_valid, HI_data, LO_data, rf_data, mem_data);
        $fwrite(trace_file, "%0t,%0b,%08h,%08h,%08h,%08h,%08h,%08h,%08h,%08h,%08h,%08h,%08h\n",
                $time, resetn, IF_pc, ID_pc, EXE_pc, MEM_pc, WB_pc,
                IF_inst, cpu_5_valid, HI_data, LO_data, rf_data, mem_data);
    end
endmodule
