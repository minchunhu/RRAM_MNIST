#ifndef _NEURON_H_
#define _NEURON_H_

#include <systemc>
#include <iostream>
#include "Config.h"

using namespace std;
using namespace sc_core;
using namespace sc_dt;

SC_MODULE(Neuron)
{
	sc_time time_neuron_reset;
	sc_lv<NUM_OF_OUTPUT_NEURONS * DATA_WIDTH> weight;
	sc_lv<DATA_WIDTH> pixel;
	
	sc_in<bool> en_p;
	sc_in<bool> reset_p;
	sc_in<bool> valid_p;
	sc_in<sc_lv<DATA_WIDTH * NUM_OF_OUTPUT_NEURONS> > weight_p;
	sc_fifo_in<sc_lv<DATA_WIDTH> > pixel_p;

	public:
		sc_lv<DATA_WIDTH> activation[NUM_OF_OUTPUT_NEURONS+1];

	SC_HAS_PROCESS(Neuron);
	Neuron(sc_module_name name):
		sc_module(name),
		time_neuron_reset(100,SC_NS)
	{
		for (int i=0; i<=NUM_OF_OUTPUT_NEURONS; i++)
		{
			for(int j=0; j<DATA_WIDTH; j++)
			{
				activation[i][j] = 0;
			}
		}
		SC_THREAD(read_reset);
		SC_THREAD(add_update);
	}

	void read_reset(void);
	void add_update(void);
};

#endif
