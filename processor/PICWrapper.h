#include <iostream>
#include <systemc.h>
#include <string>
#include "PIC8259.h"

SC_MODULE(PICWrapper)
{
    sc_in<sc_logic> clk;
    sc_in<sc_logic> dmaInterrupt;
    sc_in<sc_logic> mmaInterrupt;
    sc_in<sc_logic> INTA_bar;
    sc_out<sc_logic> INT;


    sc_in<sc_logic> t_cs;    
    sc_in<sc_lv<16>> t_addr; 
    sc_in<sc_lv<16>> t_in; 
    sc_out<sc_lv<16>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;


    sc_signal<sc_lv<8>> IR;
    sc_signal<sc_logic> RD_bar, WR_bar, CS_bar;
    sc_signal<sc_lv<16>> PIC_input;
   
    sc_signal<sc_lv<16>> PIC_output;

    PIC8259* pic;
    SC_CTOR(PICWrapper)
    {
        pic = new PIC8259("pic");
        pic->clk(clk);
        pic->IR(IR);
        pic->INTA_bar(INTA_bar);
        pic->RD_bar(RD_bar);
        pic->WR_bar(WR_bar);
        pic->CS_bar(CS_bar);
        pic->PIC_input(PIC_input);
        pic->INT(INT);
        pic->PIC_output(PIC_output);

        SC_THREAD(eval);
        sensitive << dmaInterrupt << mmaInterrupt << t_cs << t_addr << PIC_output << t_in << t_wr << t_rd;
    }
    void eval()
    {
        while(1)
        {
            IR = (sc_lv<6>("000000"),dmaInterrupt,mmaInterrupt);
            RD_bar = ~t_rd.read();
            WR_bar = ~t_wr.read();
            CS_bar = ~t_cs.read();
            PIC_input = t_in;
            t_out = PIC_output;
            t_ready = sc_logic_1;
            wait();
        }
    }
};