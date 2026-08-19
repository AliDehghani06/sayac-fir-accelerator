#pragma once

#include <systemc.h>
#include "interfaceClasses.h"

SC_MODULE(SlaveTransactor) {
    // Connection to busChannel
    sc_port<slaveSide_if<16>> bus_port;

    // Memory interface signals
    sc_out<sc_logic> readMem, writeMem, cs;
    sc_in<sc_logic> memReady;
    sc_out<sc_lv<16>> addrBus, writeData;
    sc_in<sc_lv<16>> dataBus;

    // Temporary data holders
    sc_lv<16> addr, data;
    sc_logic r_en, w_en;

    //events
    sc_event slave_done;

    // Slave ID
    int slaveID;

    SC_CTOR(SlaveTransactor) {
        slaveID = 1; // Default slave ID
        SC_THREAD(slave_process);
        SC_METHOD(on_mem_ready);     // Watch for memReady signal
        sensitive << memReady;
        dont_initialize();
    }

    void slave_process();    // Pull request from channel
    void on_mem_ready();     // React to memReady
};
