#include <systemc.h>
#include <iostream>
#include "Bus.h"
template <int N, int NUM_PORTS>
SC_MODULE(PIC)
{

    sc_in<sc_logic> clk;

    // Config port
    sc_in<sc_logic> t_cs;    
    sc_in<sc_lv<N>> t_addr; 
    sc_in<sc_lv<N>> t_in;
    sc_out<sc_lv<N>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;

    // interrupt interface with accelerators
    sc_in<sc_logic> interrupts[NUM_PORTS];

    // interrupt interface with processor
    sc_in<sc_logic> INTA_bar;
    sc_out<sc_logic> INT;

    sc_lv<N> ISR_address[NUM_PORTS];
    sc_lv<NUM_PORTS> interrupted;
    sc_lv<N> chosen_ISR;
    sc_logic interrupts_or;

    SC_CTOR(PIC)
    {
        for (int i = 0; i < NUM_PORTS; i++){
            ISR_address[i] = 0;
        }

        interrupted = 0;
 
        SC_THREAD(evalConfigReg);
        sensitive << clk;
        SC_THREAD(eval);
        sensitive << clk;
    }


    void evalConfigReg()
    {
        int address;
        while (true)
        {
            
            t_ready = sc_logic_1;
            // Wait for CS
            do
            {
                wait(clk->posedge_event());
            } while (t_cs != '1');
            t_out = 0;
            t_ready = sc_logic_0;
            if (t_wr == '1') // Write
            {
                wait(clk->posedge_event());
                address = t_addr.read().to_uint();
                
                if(address < NUM_PORTS){
                    ISR_address[address] = t_in;
                }
  
            }
            else if (t_rd == '1')
            {
                address = t_addr.read().to_uint();
                if(address < NUM_PORTS){
                    t_out = ISR_address[address];
                }
                else if(address == NUM_PORTS)
                {
                    t_out = chosen_ISR;
                }
            }
            
            t_ready = sc_logic_1;
        }
    }

    void eval()
    {
        int chosen_interrupt;
        
        INT = sc_logic_0;
        
        while (true)
        {

            do{
                wait(clk->posedge_event());
                interrupts_or = 0;
                for(int i = 0; i < NUM_PORTS; i++)
                {
                    interrupts_or = interrupts_or | interrupts[i] | interrupted[i]; 
                }
            }while(interrupts_or == 0);

            chosen_interrupt = -1;
            for(int i = 0; i < NUM_PORTS; i++) 
            {
                if(interrupts[i] == "1")
                {
                    interrupted[i] = "1";
                }
                if(interrupted[i] == "1")
                {
                    chosen_interrupt = i;
                }
            }
            if(chosen_interrupt == -1)
                continue;
            chosen_ISR = ISR_address[chosen_interrupt];
            INT = sc_logic_1;

            do{
                wait(clk->posedge_event());
            }while(INTA_bar == sc_logic_1);
            INT = sc_logic_0;
            interrupted[chosen_interrupt] = 0;
            
        }
    }
};