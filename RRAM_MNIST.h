#ifndef _RRAM_MNIST_H_
#define _RRAM_MNIST_H_

#include <systemc>
#include <iostream>
#include <fstream>
#include <cmath>
#include "Config.h"
#include "Neuron.h"

using namespace std;
using namespace sc_core;
using namespace sc_dt;

SC_MODULE(RRAM_MNIST)
{
	sc_in<bool> clk_p;
	sc_in<bool> cs_p;
	sc_inout< sc_lv<DATA_WIDTH> > io_p;
	
	bool data[NUM_OF_ROWS][NUM_OF_COLS];
	bool cs_active;
	bool cs;
	int pixels_read;
	int cycles_read_weight;
	float weight_float;
	
	sc_lv<INS_WIDTH> instruction;
	sc_lv<ADDR_WIDTH> address;
	sc_lv<DATA_WIDTH> read_value;
	sc_lv<DATA_WIDTH> write_value;
	sc_lv<DATA_WIDTH> weight;
	sc_lv<DATA_WIDTH> pixel;
	sc_lv<NUM_OF_OUTPUT_NEURONS*DATA_WIDTH> weight_page_buffer;

	Neuron nerve; 

	ifstream f;

	sc_lv<STATUS_REG_WIDTH> status_register_1;
	sc_lv<STATUS_REG_WIDTH> status_register_2;

	sc_event begin_get_instruction;
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
	sc_signal<sc_lv<NUM_OF_OUTPUT_NEURONS*DATA_WIDTH> > weight_lc;
 	sc_fifo<sc_lv<DATA_WIDTH> > pixel_fifo;

	sc_lv<DATA_WIDTH> io_high_impedance;
	sc_lv<NUM_OF_OUTPUT_NEURONS*DATA_WIDTH> weight_high_impedance;	

	SC_HAS_PROCESS(RRAM_MNIST);
	RRAM_MNIST(sc_module_name name,int weight_read_delay,int fifo_size):
		sc_module(name),
		nerve("nerve"),
		cs_active(false),
		cs(true),
		pixels_read(0),
		cycles_read_weight(weight_read_delay),
		pixel_fifo(fifo_size),
		status_register_1("00000000"),
		time_page_write(1,SC_MS),
		time_page_erase(1,SC_MS),
		time_weight_write(100,SC_NS),
		io_high_impedance(SC_LOGIC_Z)
	{
		// cout << "Attaching ports of Neuron block" << endl;
		nerve.en_p(enable_lc);
		nerve.reset_p(reset_lc);
		nerve.valid_p(valid_lc);
		nerve.pixel_p(pixel_fifo);
		nerve.weight_p(weight_lc);
		// cout << "Ports of Neuron block attached" << endl;		

		enable_lc.write(true);
		reset_lc.write(true);
		valid_lc.write(false);		
		for(int i=0;i<NUM_OF_OUTPUT_NEURONS*DATA_WIDTH;i++)
		{
			weight_high_impedance[i]=SC_LOGIC_Z;
		}
		weight_lc.write(weight_high_impedance);
		
		SC_THREAD(read_cs);
			sensitive << cs_p.value_changed();
		SC_THREAD(get_instruction);
			sensitive << begin_get_instruction;
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
			sensitive << begin_read_status_register;
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
	void get_instruction(void);
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

#endif
