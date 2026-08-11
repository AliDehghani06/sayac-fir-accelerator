// ---------------------- Transactor.h ----------------------
#include <systemc.h>
#include "busChannel.h"

//template<int N>
SC_MODULE(Transactor) {
    int masterID;
    sc_in<sc_lv<16>> addrBus;
    sc_in<sc_lv<16>> dataBusOut;
    sc_in<sc_logic> readMem, writeMem;
    sc_out<sc_logic> memReady;
    sc_out<sc_lv<16>> dataBus;

    sc_port<masterSide_if<16>> bus;

    SC_CTOR(Transactor) {
        SC_THREAD(transactor_process);
        sensitive << readMem.pos() << writeMem.pos();
    }

    void transactor_process();
};
