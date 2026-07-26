#include <systemc.h>
#include <iostream>
#include <vector>
#include "Bus.h"

// Register map:
//   0        control   : bit0 = START.
//   1        status    : bit0 = DONE.
//   2        chunkSize : number of valid samples in the chunk.
//   3..33    coeff[0..30] : the 31 FIR tap coefficients.

template <int N>
SC_MODULE(FIRAcc)
{
    static const int TAPS = 31;
    static const int MAX_CHUNK = 512;

    sc_in<sc_logic> clk;

    sc_in<sc_logic> t_cs;
    sc_in<sc_lv<N>> t_addr;
    sc_in<sc_lv<N>> t_in;
    sc_out<sc_lv<N>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;

    // Interrupt to PIC
    sc_out<sc_logic> interrupt;

    sc_fifo_in<sc_lv<N>> dataIn;   // samples in, pushed by the DMA
    sc_fifo_out<sc_lv<N>> dataOut; // filtered samples out, pulled by the DMA

    // Registers
    sc_lv<16> controlReg;
    sc_lv<16> statusReg;
    sc_lv<16> chunkSizeReg;

    int16_t coeffs[TAPS];
    int16_t history[TAPS - 1]; // last 30 samples carried over from the previous chunk

    SC_CTOR(FIRAcc)
    {
        controlReg = 0;
        statusReg = 0;
        chunkSizeReg = MAX_CHUNK;
        for (int i = 0; i < TAPS; i++)
            coeffs[i] = 0;
        for (int i = 0; i < TAPS - 1; i++)
            history[i] = 0; // zero history before the very first chunk

        SC_THREAD(evalConfigReg);
        sensitive << clk;
        SC_THREAD(eval);
        sensitive << clk;
    }

    void evalConfigReg()
    {
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

            if (t_wr == '1')
            {
                wait(clk->posedge_event());
                int addr = t_addr.read().to_uint();
                if (addr == 0)
                {
                    controlReg = t_in;
                }
                else if (addr == 2)
                {
                    chunkSizeReg = t_in;
                }
                else if (addr >= 3 && addr < 3 + TAPS)
                {
                    coeffs[addr - 3] = t_in.read().to_int();
                }
            }
            else if (t_rd == '1')
            {
                int addr = t_addr.read().to_uint();
                if (addr == 0)
                {
                    t_out = controlReg;
                }
                else if (addr == 1)
                {
                    t_out = statusReg;
                }
                else if (addr == 2)
                {
                    t_out = chunkSizeReg;
                }
                else if (addr >= 3 && addr < 3 + TAPS)
                {
                    t_out = coeffs[addr - 3];
                }
            }

            t_ready = sc_logic_1;
        }
    }

    void eval()
    {
        interrupt = sc_logic_0;
        while (true)
        {
            // Wait for start
            do
            {
                wait(clk->posedge_event());
            } while (controlReg[0] == '0');

            controlReg[0] = sc_logic_0;
            statusReg = 0;
            interrupt = sc_logic_0;

            int len = chunkSizeReg.to_uint();
            if (len > MAX_CHUNK)
                len = MAX_CHUNK;
            if (len < 0)
                len = 0;

            cout << "FIR: starting chunk of " << len << " sample" << endl;

            std::vector<int16_t> chunk(len);
            for (int i = 0; i < len; i++)
                chunk[i] = dataIn.read().to_int();

            // Extended window: [30 history samples | this chunk]
            std::vector<int16_t> ext(TAPS - 1 + len);
            for (int i = 0; i < TAPS - 1; i++)
                ext[i] = history[i];
            for (int i = 0; i < len; i++)
                ext[TAPS - 1 + i] = chunk[i];

            std::vector<int16_t> yChunk(len);
            for (int n = 0; n < len; n++)
            {
                int64_t acc = 0;
                int center = TAPS - 1 + n;
                for (int k = 0; k < TAPS; k++)
                    acc += (int32_t)ext[center - k] * (int32_t)coeffs[k];

                acc >>= 15;
                if (acc > 32767)
                    acc = 32767;
                if (acc < -32768)
                    acc = -32768;
                yChunk[n] = (int16_t)acc;
            }

            for (int i = 0; i < len; i++)
            {
                sc_lv<N> word;
                word = yChunk[i];
                dataOut.write(word);
            }

            for (int i = 0; i < TAPS - 1; i++)
                history[i] = ext[(int)ext.size() - (TAPS - 1) + i];

            statusReg = 1;
            cout << "FIR: chunk done, interrupting" << endl;

            interrupt = sc_logic_1;
            wait(clk->posedge_event());
            interrupt = sc_logic_0;
        }
    }
};
