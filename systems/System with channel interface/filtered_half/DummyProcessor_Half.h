#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>

// The 0.5x-stage bracket program

// Memory layout this program assumes:
// address 0 .. 22049 : the 22050 noisy audio samples
// address 22050 .. 22080 : the 31 FIR coefficients

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

    sc_in<sc_logic> INT;
    sc_out<sc_logic> INTA_bar;

    static const int SRC_BASE = 0;
    static const int TOTAL_SAMPLES = 22050;
    static const int OUT_SAMPLES = TOTAL_SAMPLES * 2;

    SC_CTOR(DummyProcessor)
    {
        SC_THREAD(doCPUStuff);
        sensitive << clk;
    }

    void writeToBus(uint16_t address, uint16_t value)
    {
        i_addr = address;
        i_out = value;
        i_wr = sc_logic_1;
        i_rd = sc_logic_0;

        wait(clk->posedge_event());
        do
        {
            wait(clk->posedge_event());
        } while (i_ready != '1');

        i_rd = sc_logic_0;
        i_wr = sc_logic_0;
        i_out = 0;
        i_addr = 0;

        wait(clk->posedge_event());
    }

    uint16_t readFromBus(uint16_t address)
    {
        i_addr = address;
        i_rd = sc_logic_1;
        i_wr = sc_logic_0;

        wait(clk->posedge_event());
        do
        {
            wait(clk->posedge_event());
        } while (i_ready != '1');

        uint16_t v = i_in.read().to_uint();

        i_rd = sc_logic_0;
        i_addr = 0;

        wait(clk->posedge_event());

        return v;
    }

    void doCPUStuff()
    {
        INTA_bar = sc_logic_1;
        i_rd = sc_logic_0;
        i_wr = sc_logic_0;
        i_out = 0;
        i_addr = 0;

        for (int i = 0; i < 5; i++)
            wait(clk->posedge_event());

        for (int i = TOTAL_SAMPLES - 1; i >= 0; i--)
        {
            int16_t xi = (int16_t)readFromBus(SRC_BASE + i);
            int16_t xnext = (i + 1 < TOTAL_SAMPLES) ? (int16_t)readFromBus(SRC_BASE + i + 1) : xi;
            int16_t mean = (int16_t)(((int)xi + (int)xnext) / 2);

            writeToBus(SRC_BASE + 2 * i, (uint16_t)xi);
            writeToBus(SRC_BASE + 2 * i + 1, (uint16_t)mean);
        }
        std::cout << "[CPU] interpolation done (" << OUT_SAMPLES << " samples)" << std::endl;

        // extraction
        std::ofstream outFile("filtered_half.txt");
        for (int i = 0; i < OUT_SAMPLES; i++)
        {
            int16_t v = (int16_t)readFromBus(SRC_BASE + i);
            outFile << v << "\n";
        }
        outFile.close();

        std::cout << "[CPU] wrote filtered_half.txt (" << OUT_SAMPLES << " samples)" << std::endl;
        std::cout << "[CPU] 0.5x stage complete." << std::endl;
        sc_stop();
    }
};
