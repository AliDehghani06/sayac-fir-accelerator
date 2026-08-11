#include <systemc.h>
#include <iostream>
#include "Memory.h"
#include "DMA.cpp"
#include "MatMulAcc.cpp"
#include "FIR.h"
#include "DummyProcessor_Filter.h"
#include "PIC.h"
#include "busChannel.h"
#include "Transactor.h"
#include "SlaveTransactor.h"

#define NumberOfTargets 5
#define NumberOfInitiators 2
template <int N>
SC_MODULE(EmbeddedSystem)
{
public:
    sc_in<sc_logic> clk;

    sc_signal<sc_logic> zero_wire;

    busChannel *channel;
    Transactor *initTransactor[NumberOfInitiators];
    SlaveTransactor *targetTransactor[NumberOfTargets];

    MatMulAcc<N> *mma;
    DMA *dma;
    Memory<N> *memory;
    DummyProcessor<N> *dp;
    PIC<N, 4> *pic;
    FIRAcc<N> *fir;

    // Initiator-side signals
    sc_signal<sc_lv<N>> i_addr[NumberOfInitiators];
    sc_signal<sc_lv<N>> i_in[NumberOfInitiators];
    sc_signal<sc_lv<N>> i_out[NumberOfInitiators];
    sc_signal<sc_logic> i_wr[NumberOfInitiators];
    sc_signal<sc_logic> i_rd[NumberOfInitiators];
    sc_signal<sc_logic> i_ready[NumberOfInitiators];

    // Target-side signals
    sc_signal<sc_logic> t_cs[NumberOfTargets];
    sc_signal<sc_lv<N>> t_addr[NumberOfTargets];
    sc_signal<sc_lv<N>> t_in[NumberOfTargets];
    sc_signal<sc_lv<N>> t_out[NumberOfTargets];
    sc_signal<sc_logic> t_wr[NumberOfTargets];
    sc_signal<sc_logic> t_rd[NumberOfTargets];
    sc_signal<sc_logic> t_ready[NumberOfTargets];

    // MMA <-> DMA signals
    sc_signal<sc_lv<N>> dmammaAddr;
    sc_signal<sc_lv<N>> dmammaIn;
    sc_signal<sc_lv<N>> dmammaOut;
    sc_signal<sc_logic> dmammaWR;
    sc_signal<sc_logic> dmammaRD;

    // FIR <-> DMA data path
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

        channel = new busChannel("channel");

        // 0 -> cpu
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

        initTransactor[0] = new Transactor("cpuTransactor");
        initTransactor[0]->addrBus(i_addr[0]);
        initTransactor[0]->dataBusOut(i_out[0]);
        initTransactor[0]->readMem(i_rd[0]);
        initTransactor[0]->writeMem(i_wr[0]);
        initTransactor[0]->memReady(i_ready[0]);
        initTransactor[0]->dataBus(i_in[0]);
        initTransactor[0]->bus(*channel);
        initTransactor[0]->masterID = 0;

        //1 -> dma
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

        initTransactor[1] = new Transactor("dmaTransactor");
        initTransactor[1]->addrBus(i_addr[1]);
        initTransactor[1]->dataBusOut(i_out[1]);
        initTransactor[1]->readMem(i_rd[1]);
        initTransactor[1]->writeMem(i_wr[1]);
        initTransactor[1]->memReady(i_ready[1]);
        initTransactor[1]->dataBus(i_in[1]);
        initTransactor[1]->bus(*channel);
        initTransactor[1]->masterID = 1;

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

        targetTransactor[0] = new SlaveTransactor("mmaSlave");
        targetTransactor[0]->addrBus(t_addr[0]);
        targetTransactor[0]->writeData(t_in[0]);
        targetTransactor[0]->dataBus(t_out[0]);
        targetTransactor[0]->readMem(t_rd[0]);
        targetTransactor[0]->writeMem(t_wr[0]);
        targetTransactor[0]->cs(t_cs[0]);
        targetTransactor[0]->memReady(t_ready[0]);
        targetTransactor[0]->bus_port(*channel);
        targetTransactor[0]->slaveID = 0;

        // DMA
        targetTransactor[1] = new SlaveTransactor("dmaSlave");
        targetTransactor[1]->addrBus(t_addr[1]);
        targetTransactor[1]->writeData(t_in[1]);
        targetTransactor[1]->dataBus(t_out[1]);
        targetTransactor[1]->readMem(t_rd[1]);
        targetTransactor[1]->writeMem(t_wr[1]);
        targetTransactor[1]->cs(t_cs[1]);
        targetTransactor[1]->memReady(t_ready[1]);
        targetTransactor[1]->bus_port(*channel);
        targetTransactor[1]->slaveID = 1;

        // Memory
        memory = new Memory<N>("mem");
        memory->clk(clk);
        memory->t_cs(t_cs[2]);
        memory->t_addr(t_addr[2]);
        memory->t_in(t_in[2]);
        memory->t_out(t_out[2]);
        memory->t_wr(t_wr[2]);
        memory->t_rd(t_rd[2]);
        memory->t_ready(t_ready[2]);

        targetTransactor[2] = new SlaveTransactor("memSlave");
        targetTransactor[2]->addrBus(t_addr[2]);
        targetTransactor[2]->writeData(t_in[2]);
        targetTransactor[2]->dataBus(t_out[2]);
        targetTransactor[2]->readMem(t_rd[2]);
        targetTransactor[2]->writeMem(t_wr[2]);
        targetTransactor[2]->cs(t_cs[2]);
        targetTransactor[2]->memReady(t_ready[2]);
        targetTransactor[2]->bus_port(*channel);
        targetTransactor[2]->slaveID = 2;

        // PIC
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

        targetTransactor[3] = new SlaveTransactor("picSlave");
        targetTransactor[3]->addrBus(t_addr[3]);
        targetTransactor[3]->writeData(t_in[3]);
        targetTransactor[3]->dataBus(t_out[3]);
        targetTransactor[3]->readMem(t_rd[3]);
        targetTransactor[3]->writeMem(t_wr[3]);
        targetTransactor[3]->cs(t_cs[3]);
        targetTransactor[3]->memReady(t_ready[3]);
        targetTransactor[3]->bus_port(*channel);
        targetTransactor[3]->slaveID = 3;

        // FIR
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

        targetTransactor[4] = new SlaveTransactor("firSlave");
        targetTransactor[4]->addrBus(t_addr[4]);
        targetTransactor[4]->writeData(t_in[4]);
        targetTransactor[4]->dataBus(t_out[4]);
        targetTransactor[4]->readMem(t_rd[4]);
        targetTransactor[4]->writeMem(t_wr[4]);
        targetTransactor[4]->cs(t_cs[4]);
        targetTransactor[4]->memReady(t_ready[4]);
        targetTransactor[4]->bus_port(*channel);
        targetTransactor[4]->slaveID = 4;
    }
};
