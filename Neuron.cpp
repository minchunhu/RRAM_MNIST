#include "Neuron.h"

void Neuron::read_reset(void)
{
	for(;;)
	{
		wait(reset_p->negedge_event());
		//cout << "Starting to reset at " << sc_time_stamp() << endl;
		//cout << "Reset bit set high at " << sc_time_stamp() << endl;
		activation[NUM_OF_OUTPUT_NEURONS][6] = 1;
		//cout << "Reset bit is " << activation[NUM_OF_OUTPUT_NEURONS][6] << " at time " <<  sc_time_stamp() << endl; 
		for(int i=0;i<NUM_OF_OUTPUT_NEURONS;i++)
		{
			sc_int<DATA_WIDTH> zero_sc_int = 0;
			sc_lv<DATA_WIDTH> zero = zero_sc_int;
			activation[i] = zero;
			
		}
		activation[NUM_OF_OUTPUT_NEURONS].range(3,0) = "0000";
		wait(time_neuron_reset);
		activation[NUM_OF_OUTPUT_NEURONS][6] = 0;
		//cout << "Reset complete at " << sc_time_stamp() << endl;
		wait(reset_p->posedge_event());
	}
}

void Neuron::add_update(void)
{
	for(;;)
	{
		wait(en_p->negedge_event());
		
		//cout << "Starting the inference process in the Neuron " << sc_time_stamp() << endl;
		activation[NUM_OF_OUTPUT_NEURONS][5] = 1;

		for(int i=0;i<NUM_OF_INPUT_PIXELS;i++)
		{
			//cout  << "Waiting for valid to go high in the Neuron at time " << sc_time_stamp() << endl;
			wait(valid_p->posedge_event());
			weight = weight_p->read();
			//cout  << "Weight read by Neuron at time " << sc_time_stamp() << endl;
			if (pixel_p->num_available())
			{
				pixel = pixel_p->read();
				//cout << "Pixel value read by Neuron as " << pixel << " at time " << sc_time_stamp() << endl;  
			}
			else
			{
				wait(pixel_p->data_written_event() | en_p->default_event());
				//cout << "Waiting for pixel value to be written at time " << sc_time_stamp() << endl;
				if(en_p->event())
				{
					break;
				}
				pixel = pixel_p->read();
				//cout << "Pixel value read by Neuron from fifo as " << pixel  <<  " at time " << sc_time_stamp() << endl;
				
			}
			
			sc_int<DATA_WIDTH> pixel_sc_int = pixel;
			long pixel_long = pixel_sc_int;
			float *pixel_pointer = (float *)&pixel_long;
			float pixel_float = *pixel_pointer;
			//cout << "Pixel read as " << pixel_float << endl;
			int num = 0;
			
			for(int j=NUM_OF_OUTPUT_NEURONS*DATA_WIDTH-1;j>=DATA_WIDTH-1;j-=DATA_WIDTH)
			{
				sc_lv<DATA_WIDTH> weight_j = weight(j,j-DATA_WIDTH+1);
				sc_int<DATA_WIDTH> weight_sc_int = weight_j;
				long weight_long = weight_sc_int;
				float *weight_pointer = (float *)&weight_long;
				float weight_float = *weight_pointer;
				
				//cout << "Weight read as " << weight_float << " for pixel " << i  <<endl;
				
				sc_lv<DATA_WIDTH> activate = activation[num];
				sc_int<DATA_WIDTH> activate_sc_int = activate;
				long activate_long = activate_sc_int;
				float *activate_pointer =(float *)&activate_long;
				float activate_float = *activate_pointer;

				activate_float += weight_float*pixel_float;

				long *update_pointer = (long *)&activate_float;
				sc_int<DATA_WIDTH> update_sc_int = *update_pointer;
				sc_lv<DATA_WIDTH> update = update_sc_int;
				activation[num] = update;

				num++;
			}
		}

		sc_lv<DATA_WIDTH> activate = activation[0];
		sc_int<DATA_WIDTH> activate_sc_int = activate;
		long activate_long = activate_sc_int;
		float *activate_pointer =(float *)&activate_long;
		float activate_float = *activate_pointer;
		int pred_int = 0;
		float max_val = activate_float;
		for(int i=1;i<NUM_OF_OUTPUT_NEURONS;i++)
		{
			sc_lv<DATA_WIDTH> activate = activation[i];
			sc_int<DATA_WIDTH> activate_sc_int = activate;
			long activate_long = activate_sc_int;
			float *activate_pointer =(float *)&activate_long;
			float activate_float = *activate_pointer;
			//cout << "Activation of Neuron " << i << " is " << activate_float << endl;
			if(activate_float>max_val)
			{
				max_val = activate_float;
				pred_int = i;
			}
		}
		cout << "Predicted class is " << pred_int << endl;
		sc_int<4> pred_sc_int = pred_int;
		sc_lv<4> pred = pred_sc_int;
		activation[NUM_OF_OUTPUT_NEURONS].range(3,0) = pred;
		activation[NUM_OF_OUTPUT_NEURONS][5] = 0;	
	}
}
