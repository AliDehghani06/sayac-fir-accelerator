#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Bus.h"

template <int N>
SC_MODULE(DummyProcessor)
{
    sc_in<sc_logic> clk;
    

    sc_out<sc_lv<16>> i_addr;  
    sc_in<sc_lv<16>> i_in;    
    sc_out<sc_lv<16>> i_out; 
    sc_out<sc_logic> i_wr; 
    sc_out<sc_logic> i_rd;  
    sc_in<sc_logic> i_ready;

    SC_CTOR(DummyProcessor)
	{
		SC_THREAD(doCPUStuff);
		sensitive << clk;
	}
    void writeToBus(uint16_t address,uint16_t value)
    {
        i_addr = address;
        i_out  = value;
        i_wr = sc_logic_1;
         i_rd = sc_logic_0;


        wait(clk->posedge_event());
        do
        {
            wait(clk->posedge_event());
        }while(i_ready != '1');

        i_rd = sc_logic_0;
         i_wr = sc_logic_0;
        i_out  = 0;
        i_addr = 0;

    }

    void doCPUStuff()
    {
         i_rd = sc_logic_0;
         i_wr = sc_logic_0;
        i_out  = 0;
        i_addr = 0;
        for(int i=0;i<5;i++)
            wait(clk->posedge_event());
        
        wait(clk->posedge_event());
        writeToBus(0x8008  + 1,0);
        
        wait(clk->posedge_event());
        writeToBus(0x8008  + 2,0);

        wait(clk->posedge_event());
        writeToBus(0x8008  + 3,10);

        wait(clk->posedge_event());
        writeToBus(0x8008  + 0,0b0000000000000011);

        for(int i=0;i<11;i++)
            wait(clk->posedge_event());
        writeToBus(0x0050,0);

  
    }
};