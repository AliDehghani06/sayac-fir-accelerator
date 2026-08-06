#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Bus.h"

// The FILTER-stage bracket program

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

    static const int MMA_BASE = 0xC000;
    static const int DMA_BASE = 0xC008;
    static const int PIC_BASE = 0xC010;
    static const int FIR_BASE = 0xC015;
    static const int PIC_NUM_PORTS = 4;

    static const int AUDIO_BASE = 0;
    static const int COEFF_BASE = 22050;
    static const int TOTAL_SAMPLES = 22050;
    static const int TAPS = 31;
    static const int MAX_CHUNK = 512;

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

        return v;
    }

    int waitForInterruptAndIdentify()
    {
        wait(INT.posedge_event());
        uint16_t source = readFromBus(PIC_BASE + PIC_NUM_PORTS);
        INTA_bar = sc_logic_0;
        wait(clk->posedge_event());
        INTA_bar = sc_logic_1;
        return (int)source;
    }

    void setupInterruptSources()
    {
        writeToBus(PIC_BASE + 0, 1); // DMA  -> tag 1
        writeToBus(PIC_BASE + 1, 2); // MMA  -> tag 2
        writeToBus(PIC_BASE + 2, 3); // FIR  -> tag 3
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

        setupInterruptSources();

        // Initialization phase: coefficients, memory -> FIR
        std::cout << "[CPU] loading " << TAPS << " coefficients from memory into FIR..." << std::endl;
        for (int k = 0; k < TAPS; k++)
        {
            int16_t c = (int16_t)readFromBus(COEFF_BASE + k);
            writeToBus(FIR_BASE + 3 + k, (uint16_t)c);
        }
        std::cout << "[CPU] coefficients loaded." << std::endl;

        // Processing phase: 44 chunks
        int pos = 0;
        int chunkNo = 0;
        while (pos < TOTAL_SAMPLES)
        {
            int remaining = TOTAL_SAMPLES - pos;
            int len = (remaining < MAX_CHUNK) ? remaining : MAX_CHUNK;
            chunkNo++;

            writeToBus(FIR_BASE + 2, (uint16_t)len);          // FIR chunk size
            writeToBus(FIR_BASE + 0, 0b0000000000000001);     // FIR start

            writeToBus(DMA_BASE + 1, AUDIO_BASE + pos);       // fromAddress
            writeToBus(DMA_BASE + 2, 0);                      // toAddress
            writeToBus(DMA_BASE + 3, (uint16_t)len);          // byteCount
            writeToBus(DMA_BASE + 0, 0b0000000000001011);     // start|WR|target=FIR
            waitForInterruptAndIdentify();                    // DMA: mem -> FIR done

            waitForInterruptAndIdentify();                    // FIR: chunk filtered

            writeToBus(DMA_BASE + 1, 0);                      // fromAddress
            writeToBus(DMA_BASE + 2, AUDIO_BASE + pos);       // toAddress: same location
            writeToBus(DMA_BASE + 3, (uint16_t)len);
            writeToBus(DMA_BASE + 0, 0b0000000000001101);     // start|RD|target=FIR
            waitForInterruptAndIdentify();                    // DMA: FIR -> mem done

            std::cout << "[CPU] chunk " << chunkNo << "/44 done (len=" << len << ")" << std::endl;

            pos += len;
        }

        // extraction
        std::ofstream outFile("filtered.txt");
        for (int i = 0; i < TOTAL_SAMPLES; i++)
        {
            int16_t v = (int16_t)readFromBus(AUDIO_BASE + i);
            outFile << v << "\n";
        }
        outFile.close();

        std::cout << "[CPU] wrote filtered.txt (" << TOTAL_SAMPLES << " samples)" << std::endl;
        std::cout << "[CPU] Filter stage complete." << std::endl;
    }
};