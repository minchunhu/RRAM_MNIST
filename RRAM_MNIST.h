#ifndef RRAM_MNIST_H
#define RRAM_MNIST_H

#include <systemc>
#include <iostream>
#include <fstream>
#include <cmath>
#include "Config.h"
#include "neuron.h"

using namespace std;
using namespace sc_core;
using namespace sc_dt;

SC_MODULE(RRAM_MNIST)
{
	sc_in<bool> clk_p;
	sc_in<bool> cs_p;
	sc_inout< sc_lv<DATA_WIDTH> > io_p;

	bool data[NUM_OF_ROWS][NUM_OF_COLS];
	bool cs_low;
	bool cs_val;
	int pixels_read;
	int cycles_read_weight;
	float weight_float;
	sc_lv<INS_WIDTH> instruction;
	sc_lv<ADDR_WIDTH> address;
	sc_lv<DATA_WIDTH> zero;
	sc_lv<DATA_WIDTH> read_value;
	sc_lv<DATA_WIDTH> write_value;
	sc_lv<DATA_WIDTH> weight;
	sc_lv<DATA_WIDTH> pixel;
	sc_lv<10*DATA_WIDTH> weight_page_buffer;

	neuron nerve; 

	ifstream f;

	sc_lv<STATUS_REG_WIDTH> status_register_1;
	sc_lv<STATUS_REG_WIDTH> status_register_2;

	sc_event begin_read_instruction;
	sc_event begin_read;
	sc_event begin_write_enable;
	sc_event begin_page_write;
	sc_event begin_weight_write;
	sc_event begin_page_erase;
	sc_event begin_inference;
	sc_event begin_read_neuron_value;
	sc_event begin_read_class_register;
	sc_event begin_read_status_register;
	sc_event begin_add_update_neuron;

	sc_time time_page_write;
	sc_time time_page_erase;
	sc_time time_weight_write;

	sc_signal<bool> enable_lc;
	sc_signal<bool> reset_lc;
	sc_signal<bool> valid_lc;
	sc_signal<sc_lv<10*DATA_WIDTH> > weight_lc;
 	sc_fifo<sc_lv<DATA_WIDTH> > pixel_lc;

	sc_lv<DATA_WIDTH> pixel_high_impedance;
	sc_lv<10*DATA_WIDTH> weight_high_impedance;	

	SC_HAS_PROCESS(RRAM_MNIST);
	RRAM_MNIST(sc_module_name name,int weight_read_delay,int fifo_size):
		sc_module(name),
		nerve("nerve"),
		cs_low(false),
		cs_val(true),
		pixels_read(0),
		cycles_read_weight(weight_read_delay),
		pixel_lc(fifo_size),
		status_register_1("00000000"),
		time_page_write(1,SC_MS),
		time_page_erase(1,SC_MS),
		time_weight_write(100,SC_NS)
	{
		cout << "Attaching ports of neuron block" << endl;
		nerve.en_p(enable_lc);
		nerve.reset_p(reset_lc);
		nerve.valid_p(valid_lc);
		nerve.pixel_p(pixel_lc);
		nerve.weight_p(weight_lc);
		cout << "Ports of neuron block attached" << endl;		

		enable_lc.write(true);
		reset_lc.write(true);
		valid_lc.write(false);		
		for(int i=0;i<10*DATA_WIDTH;i++)
		{
			weight_high_impedance[i]=SC_LOGIC_Z;
		}
		weight_lc.write(weight_high_impedance);
		for(int i=0;i<DATA_WIDTH;i++)	
		{
			pixel_high_impedance[i] = SC_LOGIC_Z;
			zero[i] = SC_LOGIC_0;
		}
		
		SC_THREAD(read_cs);
			sensitive << cs_p.value_changed();
		SC_THREAD(read_instruction);
			sensitive << begin_read_instruction;
			dont_initialize();
		SC_THREAD(page_read);
			sensitive << begin_read;
		SC_THREAD(write_enable);
			sensitive << begin_write_enable;
		SC_THREAD(page_write);
			sensitive << begin_page_write;
		SC_THREAD(page_erase);
			sensitive << begin_page_erase;
		SC_THREAD(weight_write);
			sensitive << begin_weight_write;
		SC_THREAD(read_neuron_value);
			sensitive << begin_read_neuron_value;
		SC_THREAD(read_class_register);
			sensitive << begin_read_class_register;
		SC_THREAD(read_status_register);
			sensitive << begin_read_instruction;
		SC_THREAD(inference);
			sensitive << begin_inference;
		SC_THREAD(add_update_neuron);
		
		
		for (int i=0; i<NUM_OF_ROWS; i++)
		{
			for(int j=0; j<NUM_OF_COLS; j++)
			{
				data[i][j] = true;
			}
		}
	}

	void read_cs(void);
	void read_instruction(void);
	void page_read(void);
	void write_enable(void);
	void page_write(void);
	void page_erase(void);
	void weight_write(void);
	void read_neuron_value(void);
	void read_class_register(void);
	void read_status_register(void);
	void inference(void);
	void add_update_neuron(void);
};

void RRAM_MNIST::read_cs(void)
{
	for(;;)
	{
		cs_val = cs_p->read();
		//cout << "CS read as " << cs_val << endl;
		if (!cs_low & cs_val==false)
		{
			//cout << "CS set low" << endl;
			cs_low= true;
			begin_read_instruction.notify();
		}
		else if (cs_low & cs_val==true)
		{
			//cout << "CS set high" << endl;
			cs_low=false;
		}
		//cout << "Waiting for CS to change value" << endl;
		wait();
	}
}

void RRAM_MNIST::read_instruction()
{
	for (;;)
	{
		//cout << "Reading instruction" << endl;
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event())
		{
			wait(SC_ZERO_TIME);
		}
		
		else
		{
			wait(SC_ZERO_TIME);
			//cout << "Reading instruction" << endl;
			instruction = io_p->read().range(7,0);
			//cout  << "Instruction read as " << instruction << endl;
			if (instruction.to_string(SC_BIN)==INS_READ)
			{	
				if (status_register_1[0]==SC_LOGIC_0)
				{
					begin_read.notify();
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}			
			}
			else if (instruction.to_string(SC_BIN)==INS_WRITE_ENABLE)
			{
				if (status_register_1[0]==SC_LOGIC_0)
				{
					begin_write_enable.notify();
					//cout << "Write enable instruction read" << endl;
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}
			} 
			else if (instruction.to_string(SC_BIN)==INS_PAGE_WRITE)
			{
				if (status_register_1[0]==SC_LOGIC_0 && status_register_1[1]==1)
				{
					begin_page_write.notify();
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}
			}
			else if (instruction.to_string(SC_BIN)==INS_WEIGHT_WRITE)
			{
				if (status_register_1[1]==1 && status_register_1[0]==SC_LOGIC_0)
				{
					begin_weight_write.notify();
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}
			} 
			else if (instruction.to_string(SC_BIN)==INS_PAGE_ERASE)
			{
				if (status_register_1[1]==1 && status_register_1[0]==SC_LOGIC_0)
				{
					begin_page_erase.notify();
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}
			}
			else if (instruction.to_string(SC_BIN)==INS_INFERENCE)
			{
				if (status_register_1[1]==1 && status_register_1[0]==SC_LOGIC_0)
				{
					begin_inference.notify();
				}
				else
				{
					cout << "Instruction ignored" << endl;
				}
			} 
			else if (instruction.to_string(SC_BIN)==INS_READ_NEURON_VALUE)
			{	
				begin_read_neuron_value.notify();
			}
			else if (instruction.to_string(SC_BIN)==INS_READ_CLASS_REG)
			{
				begin_read_class_register.notify();
			} 
			else if (instruction.to_string(SC_BIN)==INS_READ_STATUS_REG)
			{
				begin_read_status_register.notify();
			}
			else
			{
				cout << "Instruction not recognized, ignoring instruction" << endl;
			} 
		}
		wait();
	}
}

void RRAM_MNIST::page_read(void)
{
	for(;;)
	{
		wait(begin_read);
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event())
		{
			wait(SC_ZERO_TIME);
		}

		else 
		{
			wait(SC_ZERO_TIME);

			address = io_p->read().range(15,0);

			int address_int = 0;
			for(int i=0;i<16;i++)
			{
				if (address[i] == true)
				{
					address_int += pow(2,i);
				}
			}

			int row = address_int/NUM_OF_ROWS;
			int col = address_int%NUM_OF_ROWS;

			int cell = col;
			while (true)
			{
				wait(clk_p->negedge_event() | cs_p->default_event());
				
				if(cs_p->event())
				{
					break;
				}

				bool flag = false;
				for(int j=0;j<DATA_WIDTH;j++)
				{
					if ((cell+j)>=NUM_OF_COLS)
					{
						row++;
						if (row>=NUM_OF_ROWS)
						{
							row = 0;
						}
						cell = 0;
						flag = true;
						break;
					}

					read_value[DATA_WIDTH-j-1] = (sc_logic)data[row][cell+j];
				}

				if (flag==false)
				{
					cell += DATA_WIDTH;
					io_p->write(read_value);
				}
			}
			wait(clk_p->posedge_event());
			io_p->write(pixel_high_impedance);
			cout << "CS set high, ending READ operation" << endl;
		}
	}
}

void RRAM_MNIST::write_enable(void)
{
	for(;;)
	{
		wait(begin_write_enable);
		//cout << "Write enable bit written to 1" << endl;
		status_register_1[1] = SC_LOGIC_1;
	}
}

void RRAM_MNIST::page_write(void)
{
	for(;;)
	{
		wait(begin_page_write);
		status_register_1[0] = SC_LOGIC_1;

		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event())
		{
			wait(SC_ZERO_TIME);
		}

		else 
		{
			wait(SC_ZERO_TIME);

			address = io_p->read().range(15,0);

			int address_int = 0;
			for(int i=0;i<16;i++)
			{
				if (address[i] == true)
				{
					address_int += pow(2,i);
				}
			}

			int row = address_int / NUM_OF_ROWS;
			int col = address_int % NUM_OF_ROWS;

			int cell = col;

			while(true)
			{
				wait(clk_p->posedge_event() | cs_p->default_event());
				if (cs_p->event())
				{
					break;
				}

				write_value =  io_p->read();

				for(int j=0;j<DATA_WIDTH;j++)
				{
					if(cell>=NUM_OF_COLS)
					{
						cell = 0;
					}
					if (write_value[DATA_WIDTH-j-1] == SC_LOGIC_1)
					{
						data[row][cell] = true;
					}
					else
					{
						data[row][cell] = false;
					}
					cell++;
				}
			}

			wait(time_page_write);
			status_register_1[1] = SC_LOGIC_0;
			status_register_1[0] = SC_LOGIC_0;
		}
	}
}

void RRAM_MNIST::page_erase(void)
{
	for(;;)
	{
		wait(begin_page_erase);

		status_register_1[0] = SC_LOGIC_1;
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event())
		{
			wait(SC_ZERO_TIME);
		}

		else 
		{
			wait(SC_ZERO_TIME);

			address = io_p->read().range(15,0);

			int address_int = 0;
			for(int i=0;i<16;i++)
			{
				if (address[i] == true)
				{
					address_int += pow(2,i);
				}
			}

			int row = address_int/NUM_OF_ROWS;
			int col = address_int%NUM_OF_ROWS;

			int cell = 0;

			while(cell<NUM_OF_COLS)
			{
				data[row][cell] = false;
				cell++;
			}

			wait(time_page_erase);
			status_register_1[1] = SC_LOGIC_0;
			status_register_1[0] = SC_LOGIC_0;
		}
	}
}

void RRAM_MNIST::weight_write(void)
{
	for(;;)
	{
	wait(begin_weight_write);
		
	cout << "Writing weights to RRAM" << endl;
	status_register_1[0] = SC_LOGIC_1;
	cout << "Busy bit set to 1" << endl; 

	f.open("weights.txt");

	int row = 0;
	int cell = 0;

	int num_bit_pixel = 10*DATA_WIDTH;
	int beg = 0;
	int num_weights = 10*NUM_OF_INPUT_PIXELS;
	for (int i=0;i<num_weights;i++)
	{
		f >> weight_float;

		cout << "Reading weight " << i+1 << " as " << weight_float <<endl;

		long *weight_pointer = (long *)&weight_float;
		sc_int<DATA_WIDTH> weight_sc_int = *weight_pointer;
		weight = weight_sc_int;

		for(int j=0;j<DATA_WIDTH;j++)
		{
			if (weight[DATA_WIDTH-j-1] == SC_LOGIC_1)
			{
				data[row][cell] = true;
			}
			else
			{
				data[row][cell] = false;
			}
			cell++;
		}

		if ((i+1)%10==0 || cell>=NUM_OF_COLS)
		{
			cout << "Moving to next row" << endl; 
			cell = beg;
			row++;
			cout << "Row " << row << " col " << cell << endl; 
		}
		if (row>=NUM_OF_ROWS)
		{
			cout << "Wrapping the weights" << endl;
			row = 0;
			beg += num_bit_pixel;
			cell = beg;
			cout << "Row " << row << " col " << cell << endl;
		}
	}
	wait(time_weight_write);
 	cout << "Weights written to memory" << endl;
	status_register_1[0] = SC_LOGIC_0;
	cout  << "Busy bit set to 0 at time " << sc_time_stamp() << endl;
	status_register_1[1] = SC_LOGIC_0;
	cout << "Write enable set to 0" << endl;
	}
}

void RRAM_MNIST::read_neuron_value(void)
{
	for(;;)
	{
		wait(begin_read_neuron_value);
		int act = 0;
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event())
			{
				break;
			}

			else
			{
				io_p->write(nerve.activation[act]);
				act++;
				if (act>=10) act=0;
			}
		}
		io_p->write(pixel_high_impedance);
	}
}

void RRAM_MNIST::read_class_register(void)
{
	for(;;)
	{
		wait(begin_read_class_register);

		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event())
			{
				break;
			}

			else
			{
				io_p->write(nerve.activation[10]);
			}
		}
		io_p->write(pixel_high_impedance);
	}
}

void RRAM_MNIST::read_status_register(void)
{
	for(;;)
	{
		wait(begin_read_status_register);
		cout << "Reading status register" << endl; 
		sc_lv<DATA_WIDTH> val = pixel_high_impedance;
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event())
			{
				break;
			}

			else
			{
				val(7,0) = status_register_1(7,0); 
				io_p->write(val);
			}
		}
		io_p->write(pixel_high_impedance);
	}
}

void RRAM_MNIST::inference(void)
{
	for(;;)
	{
		wait(begin_inference);
		//cout << "Starting inference process, reading pixels" << endl;
		status_register_1[0] = SC_LOGIC_1;

		begin_add_update_neuron.notify();
		pixels_read = 0;
		nerve.activation[10][4] = SC_LOGIC_1;
		while(true && pixels_read<NUM_OF_INPUT_PIXELS)
		{
			wait(clk_p->posedge_event() | cs_p->default_event());
			if (cs_p->event())
			{
				break;
			}

			wait(SC_ZERO_TIME);
			pixel = io_p->read();
			//cout << "Read pixel " << pixels_read+1 << endl;
			pixel_lc.write(pixel);
			//cout << "Pixel " << pixels_read+1 << " written to fifo block" << endl;

			pixels_read++;
		}

		nerve.activation[10][4] = SC_LOGIC_0;
	}
}

void RRAM_MNIST::add_update_neuron(void)
{
	for(;;)
	{
		wait(begin_add_update_neuron);

		wait(clk_p->posedge_event() | cs_p->default_event());
		if(!cs_p->event())
		{
			//cout << "Writing neuron reset low at " << sc_time_stamp() << endl;
			reset_lc.write(false);
		
			wait(SC_ZERO_TIME);
			//cout  << "Waiting for neuron reset at " << sc_time_stamp() << endl;
			sc_logic reset_bit = (sc_logic) nerve.activation[10][6];
			//cout << "Reset bit read as " << reset_bit << " at time " << sc_time_stamp() << endl;
			while(reset_bit==SC_LOGIC_1) 
			{
				//cout << "Waiting for neuron reset to complete" << endl;	
				wait(clk_p->posedge_event());
				wait(SC_ZERO_TIME);
				reset_bit = nerve.activation[10][6];
			}
				
			//cout << "Neuron reset complete at " << sc_time_stamp() << endl;
			reset_lc.write(true);

			int row = 0;
			int cell = 0;
			int beg = 0;
			int num_pixel_bit = 10*DATA_WIDTH;
			bool flag = false;
			bool wrap = false;
				
			if(cs_low)
			{
				enable_lc.write(false);
				//cout << "Enable set low by controller at time " << sc_time_stamp() << endl;			
		
				for(int i=0;i<NUM_OF_INPUT_PIXELS;i++)
				{
					for(int j=0;j<cycles_read_weight;j++)
					{
						wait(clk_p->posedge_event() | cs_p->default_event());
						if(cs_p->event() && pixels_read<NUM_OF_INPUT_PIXELS) 
						{
							flag = true;
							break;
						}
					}

					if(pixels_read<784 && flag==true) 
					{
						enable_lc.write(true);
						break;
					}
					else
					{
						for(int k=0;k<num_pixel_bit;k++)
						{
							weight_page_buffer[num_pixel_bit-k-1] = (sc_logic)data[row][cell];
							cell++;
							if(cell>=NUM_OF_COLS)
							{
								wrap = true;
								cell = beg;
								row++;
							}
						}
						
						valid_lc.write(true);
						weight_lc.write(weight_page_buffer);
						//cout << "Valid set high and wait written as " << weight_page_buffer << " at time " << sc_time_stamp() << endl;
						wait(clk_p->negedge_event());
						valid_lc.write(false);
						weight_lc.write(weight_page_buffer);
						if(wrap==false)
						{
							cell = beg;
							row++;
						}
						if (row>=NUM_OF_ROWS)
						{
							row = 0;
							beg += num_pixel_bit;
							cell = beg;
						}
					}
				}

				status_register_1[0] = SC_LOGIC_0;
				status_register_1[1] = SC_LOGIC_0;
				enable_lc.write(true);				
				valid_lc.write(false);
				weight_lc.write(weight_high_impedance);
			}
		}
	}
}

#endif

