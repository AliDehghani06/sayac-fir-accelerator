#include <systemc.h>
#include <iostream>
#include "Bus.h"
#include "Memory.h"
#include "DMA.cpp"
#include "MatMulAcc.cpp"
#include "DummyProcessor.h"
#include "PICWrapper.h"
#include "SayacInterface.h"

#define NumberOfTargets 4
#define NumberOfInitiators 2
template <int N>
SC_MODULE(EmbeddedSystem)
{
public:
    sc_in<sc_logic> clk;

    Bus<N, NumberOfInitiators, NumberOfTargets> *bus;
    MatMulAcc<N> *mma;
    DMA *dma;
    Memory<N> *memory;
    DummyProcessor<N> *dp;
    PICWrapper* pic;
    SayacInterface* cpu;


    sc_signal<sc_lv<N>> i_addr[NumberOfInitiators];
    sc_signal<sc_lv<N>> i_in[NumberOfInitiators];
    sc_signal<sc_lv<N>> i_out[NumberOfInitiators];
    sc_signal<sc_logic> i_wr[NumberOfInitiators];
    sc_signal<sc_logic> i_rd[NumberOfInitiators];
    sc_signal<sc_logic> i_ready[NumberOfInitiators];

    sc_signal<sc_logic> t_cs[NumberOfTargets];
    sc_signal<sc_lv<N>> t_addr[NumberOfTargets];
    sc_signal<sc_lv<N>> t_in[NumberOfTargets];
    sc_signal<sc_lv<N>> t_out[NumberOfTargets];
    sc_signal<sc_logic> t_wr[NumberOfTargets];
    sc_signal<sc_logic> t_rd[NumberOfTargets];
    sc_signal<sc_logic> t_ready[NumberOfTargets];

    // MMA to DMA signals
    sc_signal<sc_lv<N>> dmammaAddr;
    sc_signal<sc_lv<N>> dmammaIn;
    sc_signal<sc_lv<N>> dmammaOut;
    sc_signal<sc_logic> dmammaWR;
    sc_signal<sc_logic> dmammaRD;

    // PIC signals
    sc_signal<sc_logic> dmaInterrupt;
    sc_signal<sc_logic> mmaInterrupt;
    sc_signal<sc_logic> INTA_bar;
    sc_signal<sc_logic> INT;
   

    SC_CTOR(EmbeddedSystem)
    {
        bus = new Bus<N, NumberOfInitiators, NumberOfTargets>("bus");
        bus->clk(clk);

        // 0 -> cpu
        bus->i_addr[0](i_addr[0]);
        bus->i_in[0](i_in[0]);
        bus->i_out[0](i_out[0]);
        bus->i_wr[0](i_wr[0]);
        bus->i_rd[0](i_rd[0]);
        bus->i_ready[0](i_ready[0]);

        //1 -> dma
        bus->i_addr[1](i_addr[1]);
        bus->i_in[1](i_in[1]);
        bus->i_out[1](i_out[1]);
        bus->i_wr[1](i_wr[1]);
        bus->i_rd[1](i_rd[1]);
        bus->i_ready[1](i_ready[1]);


        // dp = new DummyProcessor<N>("CPU");
        // dp->clk(clk);
        // dp->i_addr(i_addr[0]);
        // dp->i_in(i_in[0]);
        // dp->i_out(i_out[0]);
        // dp->i_wr(i_wr[0]);
        // dp->i_rd(i_rd[0]);
        // dp->i_ready(i_ready[0]);
        

        cpu = new SayacInterface("CPU");
        cpu->clk(clk);
        cpu->i_addr(i_addr[0]);
        cpu->i_in(i_in[0]);
        cpu->i_out(i_out[0]);
        cpu->i_wr(i_wr[0]);
        cpu->i_rd(i_rd[0]);
        cpu->i_ready(i_ready[0]);
        cpu->INT(INT);
        cpu->INTA_bar(INTA_bar);

    

        memory = new Memory<N>("mem");
        memory->clk(clk);
        memory->t_cs(t_cs[2]);
        memory->t_addr(t_addr[2]);
        memory->t_in(t_in[2]);
        memory->t_out(t_out[2]);
        memory->t_wr(t_wr[2]);
        memory->t_rd(t_rd[2]);
        memory->t_ready(t_ready[2]);
        // Starting at 0x0000
        bus->t_cs[2](t_cs[2]);
        bus->t_addr[2](t_addr[2]);
        bus->t_in[2](t_in[2]);
        bus->t_out[2](t_out[2]);
        bus->t_wr[2](t_wr[2]);
        bus->t_rd[2](t_rd[2]);
        bus->t_ready[2](t_ready[2]);
        bus->startAddress[2] = 0x0000; //MMLocation
        bus->sizeAddress[2] = 0x8000; //MMSize

        // MMA
        mma = new MatMulAcc<N>("mma");
        mma->clk(clk);
        mma->dmaAddr(dmammaAddr);
        mma->dmaIn(dmammaIn);
        mma->dmaOut(dmammaOut);
        mma->dmaWR(dmammaWR);
        mma->dmaRD(dmammaRD);
        mma->interrupt(mmaInterrupt);

        mma->t_cs(t_cs[0]);
        mma->t_addr(t_addr[0]);
        mma->t_in(t_in[0]);
        mma->t_out(t_out[0]);
        mma->t_wr(t_wr[0]);
        mma->t_rd(t_rd[0]);
        mma->t_ready(t_ready[0]);
        // Starting at 0x8000
        bus->t_cs[0](t_cs[0]);
        bus->t_addr[0](t_addr[0]);
        bus->t_in[0](t_in[0]);
        bus->t_out[0](t_out[0]);
        bus->t_wr[0](t_wr[0]);
        bus->t_rd[0](t_rd[0]);
        bus->t_ready[0](t_ready[0]);

        bus->startAddress[0] = 0x8000;
        bus->sizeAddress[0] = 8; 

        // DMA
        dma = new DMA("dma");
        dma->clk(clk);
        dma->mmaAddr(dmammaAddr);
        dma->mmaIn(dmammaIn);
        dma->mmaOut(dmammaOut);
        dma->mmaWR(dmammaWR);
        dma->mmaRD(dmammaRD);
        dma->interrupt(dmaInterrupt);


        dma->i_addr(i_addr[1]);
        dma->i_in(i_in[1]);
        dma->i_out(i_out[1]);
        dma->i_wr(i_wr[1]);
        dma->i_rd(i_rd[1]);
        dma->i_ready(i_ready[1]);

        dma->t_cs(t_cs[1]);
        dma->t_addr(t_addr[1]);
        dma->t_in(t_in[1]);
        dma->t_out(t_out[1]);
        dma->t_wr(t_wr[1]);
        dma->t_rd(t_rd[1]);
        dma->t_ready(t_ready[1]);
        // Starting at 0x8008
        bus->t_cs[1](t_cs[1]);
        bus->t_addr[1](t_addr[1]);
        bus->t_in[1](t_in[1]);
        bus->t_out[1](t_out[1]);
        bus->t_wr[1](t_wr[1]);
        bus->t_rd[1](t_rd[1]);
        bus->t_ready[1](t_ready[1]);

        bus->startAddress[1] = 0x8008;
        bus->sizeAddress[1] = 8;

        pic = new PICWrapper("picWrapper");
        pic->clk(clk);
        pic->INTA_bar(INTA_bar);
        pic->INT(INT);
        pic->dmaInterrupt(dmaInterrupt);
        pic->mmaInterrupt(mmaInterrupt);

        pic->t_cs(t_cs[3]);
        pic->t_addr(t_addr[3]);
        pic->t_in(t_in[3]);
        pic->t_out(t_out[3]);
        pic->t_wr(t_wr[3]);
        pic->t_rd(t_rd[3]);
        pic->t_ready(t_ready[3]);
        // Starting at 0x8010
        bus->t_cs[3](t_cs[3]);
        bus->t_addr[3](t_addr[3]);
        bus->t_in[3](t_in[3]);
        bus->t_out[3](t_out[3]);
        bus->t_wr[3](t_wr[3]);
        bus->t_rd[3](t_rd[3]);
        bus->t_ready[3](t_ready[3]);

        bus->startAddress[3] = 0x8010;
        bus->sizeAddress[3] = 1;
    }
};
