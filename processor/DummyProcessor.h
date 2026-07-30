#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Bus.h"

// Phase 3 changes vs. the original DummyProcessor.h:
//  - intr_DMA / intr_MMA (direct point-to-point interrupt wires) are gone.
//  - INT (in) / INTA_bar (out) added -- the same handshake PIC.h already
//    implements and SayacInterface already used. All three accelerators'
//    interrupts (DMA, MMA, FIR) now go through the PIC instead of wiring
//    straight into this module.
//  - readFromBus() added (mirrors writeToBus() exactly; this is the first
//    time a bus read is exercised by this processor, please keep an eye on
//    it when you compile/simulate).
//  - doCPUStuff() rewritten to: (1) program the PIC's ISR_address registers
//    once so we can tell sources apart later, (2) replay the original
//    MMA+DMA scenario but waiting on the PIC instead of the old direct
//    wires, (3) run a small FIR+DMA smoke test (8 samples) to prove the
//    new wiring works end-to-end. The full 44-chunk audio pass is Phase 4,
//    not this file.
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

    // Phase 3: shared interrupt line + acknowledge, routed through PIC
    sc_in<sc_logic> INT;
    sc_out<sc_logic> INTA_bar;

    // Memory-map bases the bracket program knows about (must match System.h)
    static const int MMA_BASE = 0xC000;
    static const int DMA_BASE = 0xC008;
    static const int PIC_BASE = 0xC010;
    static const int FIR_BASE = 0xC015;
    static const int PIC_NUM_PORTS = 4; // must match PIC<N,4> in System.h

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

    // Phase 3 addition: same shape as writeToBus, just reading i_in instead
    // of driving i_out.
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

    // Phase 3 addition: block until the PIC raises INT, read back which
    // source it was (via the PIC's chosen_ISR register at address
    // PIC_BASE + PIC_NUM_PORTS), then acknowledge. The returned value is
    // whatever identifier we ourselves programmed into that source's
    // ISR_address during setupInterruptSources() -- since this is a
    // bracket (no real ISR dispatch), it's just a software-chosen tag.
    int waitForInterruptAndIdentify()
    {
        wait(INT.posedge_event());
        uint16_t source = readFromBus(PIC_BASE + PIC_NUM_PORTS);
        INTA_bar = sc_logic_0; // acknowledge (PIC.h waits for this to drop)
        wait(clk->posedge_event());
        INTA_bar = sc_logic_1;
        return (int)source;
    }

    // Phase 3 addition: program the PIC once so later interrupts can be
    // told apart (PIC.h port index i on the bus = ISR_address[i]).
    void setupInterruptSources()
    {
        writeToBus(PIC_BASE + 0, 1); // interrupts[0] = DMA -> tag 1
        writeToBus(PIC_BASE + 1, 2); // interrupts[1] = MMA -> tag 2
        writeToBus(PIC_BASE + 2, 3); // interrupts[2] = FIR -> tag 3
    }

    void doCPUStuff()
    {
        INTA_bar = sc_logic_1; // idle = not acknowledging
        i_rd = sc_logic_0;
        i_wr = sc_logic_0;
        i_out = 0;
        i_addr = 0;

        for (int i = 0; i < 5; i++)
            wait(clk->posedge_event());

        setupInterruptSources();

        // -----------------------------------------------------------
        // Smoke test #1: MMA + DMA, routed through PIC.
        //
        // BUGFIX (this is the likely segfault cause): the previous version
        // used n=4,k=5,m=3 (needs 4*5 + 5*3 + 4*3 = 47 ints of MMA scratch
        // space) and then reconfigured to n=4,k=3,m=2 (4*3 + 3*2 + 4*2 = 26
        // ints) -- but MatMulAcc's buff[] is only MAX_SIZE=(1<<4)=16 ints.
        // Both configurations wrote far past the end of that heap array.
        // Switched to n=1,k=3,m=2 (1*3 + 3*2 + 1*2 = 11 ints, safely under
        // 16 -- also the same shape as the P3.cpp/P4.cpp reference
        // examples). MatMulAcc.cpp itself is untouched.
        //
        // Also now explicitly preloads memory with known sample values via
        // writeToBus before triggering the DMA transfer, instead of
        // assuming Memory's file-based init() already put something there
        // -- it currently can't, since meminit.txt doesn't exist yet.
        // -----------------------------------------------------------
        const int N_DIM = 1, K_DIM = 3, M_DIM = 2;
        const int MAT1_SIZE = N_DIM * K_DIM; // 3
        const int MAT2_SIZE = K_DIM * M_DIM; // 6
        const int OUT_SIZE = N_DIM * M_DIM;  // 2

        int16_t mat1[MAT1_SIZE] = {1, 2, 3};
        int16_t mat2[MAT2_SIZE] = {1, 0, 0, 1, 1, 1};
        for (int i = 0; i < MAT1_SIZE; i++)
            writeToBus(i, (uint16_t)mat1[i]);
        for (int i = 0; i < MAT2_SIZE; i++)
            writeToBus(MAT1_SIZE + i, (uint16_t)mat2[i]);

        writeToBus(MMA_BASE + 1, N_DIM);
        writeToBus(MMA_BASE + 2, K_DIM);
        writeToBus(MMA_BASE + 3, M_DIM);

        writeToBus(DMA_BASE + 1, 0); // fromAddress (memory)
        writeToBus(DMA_BASE + 2, 0); // toAddress (MMA buff)
        writeToBus(DMA_BASE + 3, MAT1_SIZE + MAT2_SIZE); // byteCount = 9
        writeToBus(DMA_BASE + 0, 0b0000000000000011); // start|WR, target=MMA (bit3=0)

        int src = waitForInterruptAndIdentify();
        std::cout << "[CPU] DMA (mem->MMA) done via PIC, source tag=" << src << std::endl;

        writeToBus(MMA_BASE + 0, 0b0000000000000011);
        src = waitForInterruptAndIdentify();
        std::cout << "[CPU] MMA done via PIC, source tag=" << src << std::endl;

        writeToBus(DMA_BASE + 1, MAT1_SIZE + MAT2_SIZE); // fromAddress (MMA buff, output region)
        writeToBus(DMA_BASE + 2, MAT1_SIZE + MAT2_SIZE); // toAddress (memory)
        writeToBus(DMA_BASE + 3, OUT_SIZE);              // byteCount = 2
        writeToBus(DMA_BASE + 0, 0b0000000000000101);    // start|RD, target=MMA
        src = waitForInterruptAndIdentify();
        std::cout << "[CPU] DMA (MMA->mem) done via PIC, source tag=" << src << std::endl;

        // Verify the actual math via readFromBus (also the first real
        // exercise of that new function): 1x3 row [1,2,3] times the 3x2
        // matrix [[1,0],[0,1],[1,1]] should give [4,5].
        int16_t o0 = (int16_t)readFromBus(MAT1_SIZE + MAT2_SIZE);
        int16_t o1 = (int16_t)readFromBus(MAT1_SIZE + MAT2_SIZE + 1);
        std::cout << "[CPU] MMA result: [" << o0 << "," << o1 << "] expected [4,5]" << std::endl;

        // -----------------------------------------------------------
        // Smoke test #2: new FIR path, a tiny 8-sample chunk through the
        // new DMA target-select bit and the PIC. This only proves the
        // wiring; it is NOT a numerical correctness re-test (that already
        // happened with real data in Phase 1/2) -- the coefficient below
        // is a rough near-passthrough, not the real filter.
        // -----------------------------------------------------------
        const int CHUNK_LEN = 8;
        const int SRC = 100, DST = 200;
        int16_t sample[CHUNK_LEN] = {100, -200, 300, -400, 500, -600, 700, -800};
        for (int i = 0; i < CHUNK_LEN; i++)
            writeToBus(SRC + i, (uint16_t)sample[i]);

        writeToBus(FIR_BASE + 3, 32767); // coeff[0] ~ near-unity, rest 0 -> rough passthrough
        for (int k = 1; k < 31; k++)
            writeToBus(FIR_BASE + 3 + k, 0);
        writeToBus(FIR_BASE + 2, CHUNK_LEN);           // chunkSize
        writeToBus(FIR_BASE + 0, 0b0000000000000001);  // FIR's own start

        writeToBus(DMA_BASE + 1, SRC);
        writeToBus(DMA_BASE + 2, 0);
        writeToBus(DMA_BASE + 3, CHUNK_LEN);
        writeToBus(DMA_BASE + 0, 0b0000000000001011); // start|WR|target=FIR
        src = waitForInterruptAndIdentify();
        std::cout << "[CPU] DMA (mem->FIR) done via PIC, source tag=" << src << std::endl;

        src = waitForInterruptAndIdentify();
        std::cout << "[CPU] FIR chunk done via PIC, source tag=" << src << std::endl;

        writeToBus(DMA_BASE + 1, 0);
        writeToBus(DMA_BASE + 2, DST);
        writeToBus(DMA_BASE + 3, CHUNK_LEN);
        writeToBus(DMA_BASE + 0, 0b0000000000001101); // start|RD|target=FIR
        src = waitForInterruptAndIdentify();
        std::cout << "[CPU] DMA (FIR->mem) done via PIC, source tag=" << src << std::endl;

        std::cout << "[CPU] Phase 3 smoke test finished." << std::endl;
    }
};
