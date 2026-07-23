#include <iostream>
#include <systemc.h>
#include <string>
#include "IssPowerV2.h"

SC_MODULE(SayacInterface)
{

    sc_in<sc_logic> clk;

    // Master port for accessing the bus
    sc_out<sc_lv<16>> i_addr;
    sc_in<sc_lv<16>> i_in;
    sc_out<sc_lv<16>> i_out;
    sc_out<sc_logic> i_wr;
    sc_out<sc_logic> i_rd;
    sc_in<sc_logic> i_ready;

    sc_in<sc_logic> INT;
    sc_out<sc_logic> INTA_bar;

    sayacInstruction<16, 4, 16, 3> *sayac;

    sc_signal <sc_logic> memReady;


	sc_signal <sc_lv<16>> dataBus;
	sc_signal <sc_lv<16>> dataBusOut;

	sc_signal <sc_logic> readMem, writeMem;
	sc_signal <sc_logic> readIO, writeIO;
	sc_signal <sc_lv<16>> addrBus;

    SC_CTOR(SayacInterface)
    {
        sayac = new sayacInstruction<16, 4, 16, 3>("sayac");
        sayac->clk(clk);
        sayac->memReady(i_ready);
        sayac->interrupt(INT);
        sayac->dataBus(i_in);
        sayac->dataBusOut(i_out);
        sayac->readMem(i_rd);
        sayac->writeMem(i_wr);
        sayac->addrBus(i_addr);
        sayac->INTA_bar(INTA_bar);

        sayac->readIO(readIO);
        sayac->writeIO(writeIO);
    }

};