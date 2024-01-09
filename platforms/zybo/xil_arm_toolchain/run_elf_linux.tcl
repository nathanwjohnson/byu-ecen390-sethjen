set PLAT_DIR [lindex $argv 0]
puts "connect"
puts [connect -url tcp:127.0.0.1:3121]
puts [targets -set -filter {name =~"APU*"} -index 0]
puts [rst -srst]
#puts [reset_zynqpl]
after 500
puts [configparams force-mem-accesses 1]
puts "fpga -file $PLAT_DIR/330_hw_system.bit"
puts [fpga -file $PLAT_DIR/330_hw_system.bit]
puts [source $PLAT_DIR/ps7_init.tcl]
puts [ps7_init]
puts [ps7_post_config]
#puts [xclearzynqresetstatus 64]
# puts "disconnect"
# puts [catch { disconnect };list]
# puts [exit]

set PROG_ELF [lindex $argv 1]
# puts "connect"
# puts [connect -url tcp:127.0.0.1:3121]
puts [targets -set -filter {name =~"*A9*0"}]
puts [rst -processor]
puts [configparams force-mem-accesses 1]
puts "dow $PROG_ELF"
puts [dow $PROG_ELF]
puts [bpadd -addr &exit]
# use "con -block" to wait for program to end
puts -nonewline "con"
puts [con]
puts "disconnect"
puts [catch { disconnect };list]
puts [exit]
