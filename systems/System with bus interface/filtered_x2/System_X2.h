#include <systemc.h>
#include <iostream>
#include "Bus.h"
#include "Memory.h"
#include "DMA.cpp"
#include "MatMulAcc.cpp"
#include "FIR.h"
#include "DummyProcessor_X2.h"
#include "PIC.h"

#define NumberOfTargets 5
#define NumberOfInitiators 2
template <int N>
SC_MODULE(EmbeddedSystem)
{
public:
    sc_in<sc_logic> clk;

    sc_signal<sc_logic> zero_wire;

    Bus<N, NumberOfInitiators, NumberOfTargets> *bus;
    MatMulAcc<N> *mma;
    DMA *dma;
    Memory<N> *memory;
    DummyProcessor<N> *dp;
    PIC<N, 4> *pic;
    FIRAcc<N> *fir;

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

    // FIR <-> DMA data path: SystemC predefined FIFOs
    sc_fifo<sc_lv<N>> memToFir;
    sc_fifo<sc_lv<N>> firToMem;

    // PIC signals
    sc_signal<sc_logic> dmaInterrupt;
    sc_signal<sc_logic> mmaInterrupt;
    sc_signal<sc_logic> firInterrupt;
    sc_signal<sc_logic> INTA_bar;
    sc_signal<sc_logic> INT;

    SC_CTOR(EmbeddedSystem) : memToFir(600), firToMem(600)
    {
        zero_wire = sc_logic_0;

        bus = new Bus<N, NumberOfInitiators, NumberOfTargets>("bus");
        bus->clk(clk);

        // 0 -> cpu
        bus->i_addr[0](i_addr[0]);
        bus->i_in[0](i_in[0]);
        bus->i_out[0](i_out[0]);
        bus->i_wr[0](i_wr[0]);
        bus->i_rd[0](i_rd[0]);
        bus->i_ready[0](i_ready[0]);

        // 1 -> dma
        bus->i_addr[1](i_addr[1]);
        bus->i_in[1](i_in[1]);
        bus->i_out[1](i_out[1]);
        bus->i_wr[1](i_wr[1]);
        bus->i_rd[1](i_rd[1]);
        bus->i_ready[1](i_ready[1]);

        dp = new DummyProcessor<N>("CPU");
        dp->clk(clk);
        dp->i_addr(i_addr[0]);
        dp->i_in(i_in[0]);
        dp->i_out(i_out[0]);
        dp->i_wr(i_wr[0]);
        dp->i_rd(i_rd[0]);
        dp->i_ready(i_ready[0]);
        dp->INT(INT);
        dp->INTA_bar(INTA_bar);

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
        bus->startAddress[2] = 0x0000;
        bus->sizeAddress[2] = 0xC000;

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
        bus->t_cs[0](t_cs[0]);
        bus->t_addr[0](t_addr[0]);
        bus->t_in[0](t_in[0]);
        bus->t_out[0](t_out[0]);
        bus->t_wr[0](t_wr[0]);
        bus->t_rd[0](t_rd[0]);
        bus->t_ready[0](t_ready[0]);

        bus->startAddress[0] = 0xC000;
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

        dma->firDataIn(memToFir);
        dma->firDataOut(firToMem);

        bus->t_cs[1](t_cs[1]);
        bus->t_addr[1](t_addr[1]);
        bus->t_in[1](t_in[1]);
        bus->t_out[1](t_out[1]);
        bus->t_wr[1](t_wr[1]);
        bus->t_rd[1](t_rd[1]);
        bus->t_ready[1](t_ready[1]);

        bus->startAddress[1] = 0xC008;
        bus->sizeAddress[1] = 8;

        pic = new PIC<N, 4>("pic");
        pic->clk(clk);
        pic->INTA_bar(INTA_bar);
        pic->INT(INT);
        pic->interrupts[0](dmaInterrupt);
        pic->interrupts[1](mmaInterrupt);
        pic->interrupts[2](firInterrupt);
        pic->interrupts[3](zero_wire);

        pic->t_cs(t_cs[3]);
        pic->t_addr(t_addr[3]);
        pic->t_in(t_in[3]);
        pic->t_out(t_out[3]);
        pic->t_wr(t_wr[3]);
        pic->t_rd(t_rd[3]);
        pic->t_ready(t_ready[3]);
        bus->t_cs[3](t_cs[3]);
        bus->t_addr[3](t_addr[3]);
        bus->t_in[3](t_in[3]);
        bus->t_out[3](t_out[3]);
        bus->t_wr[3](t_wr[3]);
        bus->t_rd[3](t_rd[3]);
        bus->t_ready[3](t_ready[3]);

        bus->startAddress[3] = 0xC010;
        bus->sizeAddress[3] = 5;

        fir = new FIRAcc<N>("fir");
        fir->clk(clk);
        fir->interrupt(firInterrupt);
        fir->dataIn(memToFir);
        fir->dataOut(firToMem);

        fir->t_cs(t_cs[4]);
        fir->t_addr(t_addr[4]);
        fir->t_in(t_in[4]);
        fir->t_out(t_out[4]);
        fir->t_wr(t_wr[4]);
        fir->t_rd(t_rd[4]);
        fir->t_ready(t_ready[4]);
        bus->t_cs[4](t_cs[4]);
        bus->t_addr[4](t_addr[4]);
        bus->t_in[4](t_in[4]);
        bus->t_out[4](t_out[4]);
        bus->t_wr[4](t_wr[4]);
        bus->t_rd[4](t_rd[4]);
        bus->t_ready[4](t_ready[4]);

        bus->startAddress[4] = 0xC015;
        bus->sizeAddress[4] = 34;
    }
};
