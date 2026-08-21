`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   11:57:16 04/23/2016
// Design Name:   pipeline_cpu
// Module Name:   F:/new_lab/8_pipeline_cpu/tb.v
// Project Name:  pipeline_cpu
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: pipeline_cpu
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module tb;

    // Inputs
    reg clk;
    reg resetn;
    reg [4:0] rf_addr;
    reg [31:0] mem_addr;

    // Outputs
    wire [31:0] rf_data;
    wire [31:0] mem_data;
    wire [31:0] IF_pc;
    wire [31:0] IF_inst;
    wire [31:0] ID_pc;
    wire [31:0] EXE_pc;
    wire [31:0] MEM_pc;
    wire [31:0] WB_pc;
    wire [31:0] cpu_5_valid;

    // Instantiate the Unit Under Test (UUT)
    pipeline_cpu uut (
        .clk(clk), 
        .resetn(resetn), 
        .rf_addr(rf_addr), 
        .mem_addr(mem_addr), 
        .rf_data(rf_data), 
        .mem_data(mem_data), 
        .IF_pc(IF_pc), 
        .IF_inst(IF_inst), 
        .ID_pc(ID_pc), 
        .EXE_pc(EXE_pc), 
        .MEM_pc(MEM_pc), 
        .WB_pc(WB_pc), 
        .cpu_5_valid(cpu_5_valid),
        .HI_data(HI_data),
        .LO_data(LO_data)
    );
    
    integer file;

    initial begin
        // Initialize Inputs
        clk = 0;
        resetn = 0;
        rf_addr = 0;
        mem_addr = 0;
        
         // Open file for writing
        file = $fopen("D:/vivado/pipeline_cpu/pipeline_cpu.sim/simulation_results_old.txt", "w");
        if (file == 0) begin
            $display("Error opening file!");
            $finish;
        end

        // Write header
        $fwrite(file, "Time\tresetn\tIF_pc\tIF_inst\tID_pc\tEXE_pc\tMEM_pc\tWB_pc\tcpu_5_valid\tHI_data\tLO_data\trf_data\tmem_data\n");
        
        // Wait 100 ns for global reset to finish
        #100;
        resetn = 1;
        
        // Run simulation for a while
        #1000;
        
        // Write final values
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
        
        // Close file
        $fclose(file);
        $finish;
    end
   always #5 clk=~clk;
   
   always @(posedge clk) begin
        $fwrite(file, "%t\t%b\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\n", 
                $time, resetn, IF_pc, IF_inst, ID_pc, EXE_pc, MEM_pc, WB_pc, 
                cpu_5_valid, HI_data, LO_data, rf_data, mem_data);
    end
endmodule

