# Add ls132r CPU RTL sources (from cpu132_gettrace) into both design and sim filesets.
# Usage in Vivado Tcl Console (project already open):
#   source [file normalize "run_vivado/add_cpu_sources.tcl"]

set this_dir [file dirname [file normalize [info script]]]
# cpu132_gettrace is a sibling directory of mycpu_sram_verify, so hop two levels up
set cpu_dir  [file normalize [file join $this_dir ".." ".." "cpu132_gettrace" "rtl" "CPU_gs132"]]

set cpu_files [glob -nocomplain -directory $cpu_dir *.v]
if {[llength $cpu_files] == 0} {
    puts "No CPU RTL files found under $cpu_dir"
} else {
    puts "Adding CPU RTL files:"
    puts "  $cpu_files"
    add_files -fileset sources_1 $cpu_files
    add_files -fileset sim_1     $cpu_files
    update_compile_order -fileset sources_1
    update_compile_order -fileset sim_1
}
