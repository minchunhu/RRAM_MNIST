#ifndef sender_H
#define sender_H

#include <systemc>
#include <iostream>
#include <fstream>
#include "Config.h"

using namespace std;
using namespace sc_core;
using namespace sc_dt;

SC_MODULE(sender)
{
	sc_in<bool> clk;
	sc_out<bool> cs_lc;
	sc_inout< sc_lv<DATA_WIDTH> > io;
	
	ifstream f;
	
	sc_lv<DATA_WIDTH> high_impedance;
	
	SC_CTOR(sender):
		high_impedance("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ")
	{
		SC_THREAD(test_script);
	}

	void test_script(void);
};
#endif
