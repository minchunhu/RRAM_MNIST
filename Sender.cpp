#include "Sender.h"
#include <ctime>

void Sender::test_script(void)
{
	sc_lv<DATA_WIDTH> dout;
	
	cs.write(true);
	io.write(high_impedance);
	
	// Write Enable
	cout << name() << ": Write Enable Instruction" << endl;
	wait(clk.negedge_event());
	cs.write(false);
	io.write(INS_WRITE_ENABLE);
	wait(clk.negedge_event());
	cs.write(true);
	io.write(high_impedance);
	
	// Weight Write
	cout << name() << ": Weight Write Instruction" << endl;
	wait(clk.negedge_event());
	cs.write(false);
	io.write(INS_WEIGHT_WRITE); 
	wait(clk.negedge_event());
	cs.write(true);
	io.write(high_impedance);
	
	/*
	// Read Status Register
	cout << name() << ": Read Status Instruction" << endl;
	wait(clk.negedge_event());
	cs.write(false);
	io.write(INS_READ_STATUS_REG);
	wait(SC_ZERO_TIME);
	dout = io.read();
	while (dout[0] != SC_LOGIC_0)
	{
		cout << name() << ": Waiting for BUSY bit to go low" << endl;
		wait(clk.negedge_event());
		wait(SC_ZERO_TIME);
		dout = io.read();
	}
	cout << "Weights have been written at time " << sc_time_stamp()  << endl;
	wait(clk.negedge_event());
	cs.write(true);
	*/
	// Page Read
	wait(clk.negedge_event());
	cs.write(false);
	io.write(INS_READ);
	wait(clk.negedge_event());
	io.write("00000000000000000000000000000000"); // Address
	for(int i=0; i<10; i++)
	{
		wait(clk.posedge_event());
		dout = io.read();
		cout << "Weight read as in bit format " << dout << endl;
		sc_int<DATA_WIDTH> weight_sc_int = dout;
		long weight_int = weight_sc_int;
		float *weight_p = (float *)&weight_int;
		float weight_float = *weight_p;
		cout << "Weight " << i+1 << " read as " << weight_float << " at time " << sc_time_stamp() << endl;  
	}
	wait(clk.negedge_event());
	cs.write(true);
	
	/*
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
		cs.write(false);
		wait(clk.posedge_event());
		ins = "00000000000000000000000000000110";
		io.write(ins);
		wait(clk.posedge_event());
		io.write(high_impedance);
		wait(5,SC_NS);
		cs.write(true);
		wait(clk.posedge_event());
		wait(5,SC_NS);
		cs.write(false);
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
		cs.write(true);
		wait(clk.posedge_event());
		wait(5,SC_NS);
		cs.write(false);
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
		cs.write(true);
	}
	time_t t2 = time(0);
	float num_img = (float)num_images;
	accuracy = 100.0*accuracy/num_img;
	cout << "Accuracy calculated for " << num_images << " images is " << accuracy << " after simulation time " << sc_simulation_time()/1000.0/1000.0 << "ms and actual time " << t2-t1  << endl;
	*/
}
