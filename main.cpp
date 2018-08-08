#include "RRAM_MNIST.h"
#include "sender.h"

int sc_main(int argc, char* argv[])
{
	sc_clock clk("clk", 20, SC_NS, 0.5, 10, SC_NS, true);	
	sc_signal<bool> cs_lc;
	sc_signal<sc_lv<32>, SC_MANY_WRITERS > io;
	
	RRAM_MNIST RRAM("RRAM",1,8);
		RRAM.clk_p(clk);
		RRAM.cs_p(cs_lc);
		RRAM.io_p(io);
		
	sender sender("Sender");
		sender.clk(clk);
		sender.cs_lc(cs_lc);
		sender.io(io);

	sc_start();
	
	// sc_trace_file* tracefile;
	// tracefile = sc_create_vcd_trace_file("waveform");
	// sc_trace(tracefile,clk, "clk");
	// sc_trace(tracefile,cs_lc, "CS");
	// sc_trace(tracefile,io, "IO");
	
	// sender.write_enable();
	
	// sc_stop();

	return 0;
}
