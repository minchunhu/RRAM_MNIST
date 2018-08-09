#include "RRAM_MNIST.h"

void RRAM_MNIST::read_cs(void)
{
	while(true)
	{
		wait(SC_ZERO_TIME);
		cs = cs_p->read();
		if (!cs_active && cs == false)
		{
			// cout << name() << ": cs is active" << endl;
			cs_active = true;
			begin_get_instruction.notify();
		}
		else if (cs_active && cs == true)
		{
			// cout << name() << ": cs is inactive" << endl;
			cs_active = false;
		}
		wait();
	}
}

void RRAM_MNIST::get_instruction()
{
	while(1)
	{
		wait();
		
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event()) {
			continue;
		}
		
		instruction = io_p->read().range(7,0);
		
		if (status_register_1[0] != 0) {
			cout << name() << ": busy ... instruction " << instruction << " ignored" << endl;
			continue;
		}
		
		if (instruction.to_string(SC_BIN) == INS_READ) {
			cout << name() << ": receive READ instruction " << endl;
			begin_read.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_WRITE_ENABLE) {
			cout << name() << ": receive WRITE_ENABLE instruction " << endl;
			begin_write_enable.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_PAGE_WRITE) {
			cout << name() << ": receive PAGE_WRITE instruction " << endl;
			begin_page_write.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_WEIGHT_WRITE) {
			cout << name() << ": receive WEIGHT_WRITE instruction " << endl;
			begin_weight_write.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_PAGE_ERASE) {
			cout << name() << ": receive PAGE_ERASE instruction " << endl;
			begin_page_erase.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_INFERENCE) {
			cout << name() << ": receive INFERENCE instruction " << endl;
			begin_inference.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_READ_NEURON_VALUE) {
			cout << name() << ": receive READ_NEURON_VALUE instruction " << endl;
			begin_read_neuron_value.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_READ_CLASS_REG) {
			cout << name() << ": receive READ_CLASS_REG instruction " << endl;
			begin_read_class_register.notify();
		}
		else if (instruction.to_string(SC_BIN) == INS_READ_STATUS_REG) {
			cout << name() << ": receive READ_STATUS_REG instruction " << endl;
			begin_read_status_register.notify();
		}
		else {
			cout << name() << ": instruction not recognized" << endl;
		}
	}
}

void RRAM_MNIST::page_read(void)
{
	while(true)
	{
		wait();
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event()) {
			continue;
		}
		
		address = io_p->read().range(ADDR_WIDTH-1,0);
		
		int address_int = 0;
		for(int i=0; i<ADDR_WIDTH; i++) {
			if (address[i] == true) {
				address_int += pow(2,i);
			}
		}
		
		int row = address_int / NUM_OF_ROWS;
		int col = address_int % NUM_OF_ROWS;
		
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if(cs_p->event()) {
				break;
			}
			
			bool read_row_finish = false;
			for(int j=0; j<DATA_WIDTH; j++)
			{
				if ((col+j) >= NUM_OF_COLS)
				{
					row++;
					if (row >= NUM_OF_ROWS)
					{
						row = 0;
					}
					col = 0;
					read_row_finish = true;
					break;
				}
				
				read_value[DATA_WIDTH-j-1] = (sc_logic)data[row][col+j];
			}
			
			if (read_row_finish == false)
			{
				col += DATA_WIDTH;
				io_p->write(read_value);
			}
		}
		wait(clk_p->negedge_event());
		io_p->write(io_high_impedance);
	}
}

void RRAM_MNIST::write_enable(void)
{
	while(true)
	{
		wait();
		// Set write enable bit = 1
		status_register_1[1] = 1;
	}
}

void RRAM_MNIST::page_write(void)
{
	while(true)
	{
		wait();
		
		// Check write enable/disable
		if (status_register_1[1] != 1) {
			cout << name() << ": write disable" << endl;
			continue;
		}
		
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event()) {
			continue;
		}
		
		address = io_p->read().range(ADDR_WIDTH-1,0);
		
		int address_int = 0;
		for(int i=0;i<ADDR_WIDTH;i++)
		{
			if (address[i] == true)
			{
				address_int += pow(2,i);
			}
		}
		
		int row = address_int / NUM_OF_ROWS;
		int col = address_int % NUM_OF_ROWS;
		
		while(true)
		{
			wait(clk_p->posedge_event() | cs_p->default_event());
			if (cs_p->event()) {
				break;
			}

			write_value =  io_p->read();

			for(int j=0; j<DATA_WIDTH; j++)
			{
				if(col >= NUM_OF_COLS)
				{
					col = 0;
				}
				if (write_value[DATA_WIDTH-j-1] == 1)
				{
					data[row][col] = true;
				}
				else
				{
					data[row][col] = false;
				}
				col++;
			}
		}
		
		status_register_1[0] = 1;
		wait(time_page_write);
		status_register_1[1] = 0;
		status_register_1[0] = 0;
	}
}

void RRAM_MNIST::page_erase(void)
{
	while(true)
	{
		wait();
		
		// Check write enable/disable
		if (status_register_1[1] != 1) {
			cout << name() << ": write disable" << endl;
			continue;
		}
		
		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event()) {
			continue;
		}

		address = io_p->read().range(ADDR_WIDTH-1,0);

		int address_int = 0;
		for(int i=0;i<ADDR_WIDTH;i++)
		{
			if (address[i] == true)
			{
				address_int += pow(2,i);
			}
		}

		int row = address_int / NUM_OF_ROWS;
		// int col = address_int % NUM_OF_ROWS;

		int col = 0;

		while(col<NUM_OF_COLS)
		{
			data[row][col] = false;
			col++;
		}
		
		status_register_1[0] = 1;
		wait(time_page_erase);
		status_register_1[1] = 0;
		status_register_1[0] = 0;
	}
}

void RRAM_MNIST::weight_write(void)
{
	while(true)
	{
		wait();
		
		// status_register_1[0] = 1;
		
		f.open("weights.txt");
		
		int row = 0;
		int col = 0;
	
		int num_bit_pixel = NUM_OF_OUTPUT_NEURONS * DATA_WIDTH;
		int beg = 0;
		int num_weights = NUM_OF_OUTPUT_NEURONS*NUM_OF_INPUT_PIXELS;
		for (int i=0;i<num_weights;i++)
		{
			f >> weight_float;
	
			// cout << "Reading weight " << i+1 << " as " << weight_float <<endl;
	
			long *weight_pointer = (long *)&weight_float;
			sc_int<DATA_WIDTH> weight_sc_int = *weight_pointer;
			weight = weight_sc_int;
	
			for(int j=0;j<DATA_WIDTH;j++)
			{
				if (weight[DATA_WIDTH-j-1] == 1)
				{
					data[row][col] = true;
				}
				else
				{
					data[row][col] = false;
				}
				col++;
			}
	
			if ((i+1)%10==0 || col>=NUM_OF_COLS)
			{
				// cout << "Moving to next row" << endl; 
				col = beg;
				row++;
				// cout << "Row " << row << " col " << col << endl; 
			}
			if (row>=NUM_OF_ROWS)
			{
				// cout << "Wrapping the weights" << endl;
				row = 0;
				beg += num_bit_pixel;
				col = beg;
				// cout << "Row " << row << " col " << col << endl;
			}
		}
		// wait(time_weight_write);
		// cout << "Weights written to memory" << endl;
		// status_register_1[0] = 0;
		// cout  << "Busy bit set to 0 at time " << sc_time_stamp() << endl;
		// status_register_1[1] = 0;
		// cout << "Write enable set to 0" << endl;
	}
}

void RRAM_MNIST::read_neuron_value(void)
{
	while(true)
	{
		wait();
		int act = 0;
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event()) {
				break;
			}
			io_p->write(nerve.activation[act]);
			act++;
			if (act >= NUM_OF_OUTPUT_NEURONS) act = 0;
		}
		io_p->write(io_high_impedance);
	}
}

void RRAM_MNIST::read_class_register(void)
{
	while(true)
	{
		wait();
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event()) {
				break;
			}
			io_p->write(nerve.activation[NUM_OF_OUTPUT_NEURONS]);
		}
		io_p->write(io_high_impedance);
	}
}

void RRAM_MNIST::read_status_register(void)
{
	while(true)
	{
		wait();
		cout << "Reading status register" << endl; 
		while (true)
		{
			wait(clk_p->negedge_event() | cs_p->default_event());
			if (cs_p->event()) {
				break;
			}
			io_p->write(status_register_1(STATUS_REG_WIDTH-1,0));
		}
		io_p->write(io_high_impedance);
	}
}

void RRAM_MNIST::inference(void)
{
	while(true)
	{
		wait();
		status_register_1[0] = 1;
		begin_add_update_neuron.notify();
		pixels_read = 0;
		nerve.activation[NUM_OF_OUTPUT_NEURONS][4] = 1;
		while(true && pixels_read < NUM_OF_INPUT_PIXELS)
		{
			wait(clk_p->posedge_event() | cs_p->default_event());
			if (cs_p->event()) {
				break;
			}
			pixel = io_p->read();
			pixel_fifo.write(pixel);
			pixels_read++;
		}
		nerve.activation[NUM_OF_OUTPUT_NEURONS][4] = 0;
	}
}

void RRAM_MNIST::add_update_neuron(void)
{
	while(true)
	{
		wait();

		wait(clk_p->posedge_event() | cs_p->default_event());
		if (cs_p->event()) {
			continue;
		}
		
		reset_lc.write(false);
		
		wait(SC_ZERO_TIME);
		sc_logic reset_bit = (sc_logic) nerve.activation[NUM_OF_OUTPUT_NEURONS][6];
		while(reset_bit == 1)
		{
			wait(clk_p->posedge_event());
			wait(SC_ZERO_TIME);
			reset_bit = nerve.activation[NUM_OF_OUTPUT_NEURONS][6];
		}
		
		reset_lc.write(true);

		int row = 0;
		int col = 0;
		int beg = 0;
		int num_pixel_bit = NUM_OF_OUTPUT_NEURONS * DATA_WIDTH;
		bool flag = false;
		bool wrap = false;
			
		if(cs_active)
		{
			enable_lc.write(false);		
			
			for(int i=0; i<NUM_OF_INPUT_PIXELS; i++)
			{
				for(int j=0; j<cycles_read_weight; j++)
				{
					wait(clk_p->posedge_event() | cs_p->default_event());
					if(cs_p->event() && pixels_read < NUM_OF_INPUT_PIXELS) 
					{
						flag = true;
						break;
					}
				}

				if(pixels_read < NUM_OF_INPUT_PIXELS && flag == true) 
				{
					enable_lc.write(true);
					break;
				}
				else
				{
					for(int k=0; k<num_pixel_bit; k++)
					{
						weight_page_buffer[num_pixel_bit-k-1] = (sc_logic)data[row][col];
						col++;
						if(col >= NUM_OF_COLS)
						{
							wrap = true;
							col = beg;
							row++;
						}
					}
					
					valid_lc.write(true);
					weight_lc.write(weight_page_buffer);
					wait(clk_p->negedge_event());
					valid_lc.write(false);
					weight_lc.write(weight_page_buffer);
					if(wrap == false)
					{
						col = beg;
						row++;
					}
					if (row >= NUM_OF_ROWS)
					{
						row = 0;
						beg += num_pixel_bit;
						col = beg;
					}
				}
			}
			status_register_1[0] = 0;
			status_register_1[1] = 0;
			enable_lc.write(true);				
			valid_lc.write(false);
			weight_lc.write(weight_high_impedance);
		}
	}
}
