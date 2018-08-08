#include "sender.h"
#include <ctime>

void sender::test_script(void)
{
	cs_lc.write(true);
	io.write(high_impedance);
	
	wait(clk.posedge_event());
	wait(5,SC_NS);
	cout << "Writing CS low" << endl;
	cs_lc.write(false);
	wait(clk.posedge_event());
	//sc_lv<64> zero = "0000000000000000000000000000000000000000000000000000000000000000";
	sc_lv<DATA_WIDTH> write_enable = "00000000000000000000000000000110";
	//zero(31,0) = write_enable;
	io.write(write_enable);
	wait(clk.posedge_event());
	io.write(high_impedance);
	wait(5,SC_NS);
	cs_lc.write(true);

	wait(clk.posedge_event());
	wait(5,SC_NS);
	cs_lc.write(false);
	wait(clk.posedge_event());
	sc_lv<DATA_WIDTH> ins = "00000000000000000000000000010101";
	//zero(31,0) = ins;
	io.write(ins); 
	wait(clk.posedge_event());
	io.write(high_impedance);
	wait(5,SC_NS);
	cs_lc.write(true);

	wait(clk.posedge_event());
	wait(5,SC_NS);
	cs_lc.write(false);
	wait(clk.posedge_event());
	ins = "00000000000000000000000000000101";
	//zero(31,0) = ins;
	io.write(ins);
	wait(SC_ZERO_TIME);
	ins = io.read();
	while (ins[0] != SC_LOGIC_0)
	{
		cout << "Waiting for BUSY bit to go LOW" << endl;
		wait(clk.negedge_event());
		wait(SC_ZERO_TIME);
		ins = io.read();
	}
	cout << "Weights have been written at time " << sc_time_stamp()  << endl; 
	wait(clk.posedge_event());	
	wait(5,SC_NS);
	cs_lc.write(true);

	wait(clk.posedge_event());
	wait(5,SC_NS);
	cs_lc.write(false);
	wait(clk.posedge_event());
	ins = "00000000000000000000000000000011";
	//zero(31,0) = ins;
	io.write(ins);
	sc_lv<DATA_WIDTH> address = "00000000000000000000000000000000";
	wait(clk.posedge_event());
	io.write(address);
	for(int i=0;i<10;i++)
	{
		wait(clk.negedge_event());
		wait(SC_ZERO_TIME);
	
		sc_lv<DATA_WIDTH> weight = io.read();
		cout << "Weight read as in bit format " << weight << endl;
		sc_int<DATA_WIDTH> weight_sc_int = weight;
		long weight_int = weight_sc_int;
		float *weight_p = (float *)&weight_int;
		float weight_float = *weight_p;
		cout << "Weight " << i+1 << " read as " << weight_float << " at time " << sc_time_stamp() << endl;  
	}
	wait(clk.posedge_event());
	wait(5,SC_NS);
	cs_lc.write(true);

	f.open("pixels.txt");
	int num_images = 0;
	float accuracy = 0;
	f >> num_images;
	time_t t1 = time(0);
	for(int i=0;i<num_images;i++)
	{
		cout << "\n Writing pixel values for image " << i+1 << " at time " << sc_simulation_time()/1000.0/1000.0 << "\n"  << endl;
		wait(clk.posedge_event());
		wait(5,SC_NS);
		cs_lc.write(false);
		wait(clk.posedge_event());
		ins = "00000000000000000000000000000110";
		io.write(ins);
		wait(clk.posedge_event());
		io.write(high_impedance);
		wait(5,SC_NS);
		cs_lc.write(true);
		wait(clk.posedge_event());
		wait(5,SC_NS);
		cs_lc.write(false);
		wait(clk.posedge_event());
		ins = "00000000000000000000000000010100";
		io.write(ins);
		for(int j=0;j<NUM_OF_INPUT_PIXELS;j++)
		{
			float pix_float = 0.0;
			f >> pix_float;
			long *pix_pointer = (long *)&pix_float;
			sc_int<DATA_WIDTH> pix_sc_int = *pix_pointer;
			sc_lv<DATA_WIDTH> pix  = pix_sc_int;
			wait(clk.posedge_event());
			io.write(pix);
		}
		wait(clk.posedge_event());
		io.write("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
		wait(5,SC_NS);
		cs_lc.write(true);
		wait(clk.posedge_event());
		wait(5,SC_NS);
		cs_lc.write(false);
		wait(clk.posedge_event());
		ins = "00000000000000000000000000010111";
		io.write(ins);
		wait(clk.negedge_event());
		wait(SC_ZERO_TIME);
		sc_lv<DATA_WIDTH> done = io.read();
		while(done[5]==SC_LOGIC_1)
		{
			wait(clk.negedge_event());
			wait(SC_ZERO_TIME);
			done = io.read();
		}
		int pred = 0;
		for(int b=0;b<4;b++)
		{
			if (done[b]==SC_LOGIC_1)
			{
				pred += (int)pow(2,(int)b);
			}
		}
		int actual = 0;
		f >> actual;
		cout << "Actual class is " << actual << endl;
		if (actual == pred )
		{
			accuracy++;
		}
		wait(clk.posedge_event());
		io.write(high_impedance);
		wait(5,SC_NS);
		cs_lc.write(true);
	}
	time_t t2 = time(0);
	float num_img = (float)num_images;
	accuracy = 100.0*accuracy/num_img;
	cout << "Accuracy calculated for " << num_images << " images is " << accuracy << " after simulation time " << sc_simulation_time()/1000.0/1000.0 << "ms and actual time " << t2-t1  << endl;
	
}
