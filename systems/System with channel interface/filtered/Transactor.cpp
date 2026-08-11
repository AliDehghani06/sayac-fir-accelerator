
// ---------------------- Transactor.cpp ----------------------
#include "Transactor.h"

//template<int N>
void Transactor::transactor_process() {
    dataBus = 0;
    while (true) {
        
        wait();
        memReady.write(SC_LOGIC_0);

        //int masterID = 0;
        sc_lv<16> addr = addrBus.read();
        sc_lv<16> writeData = dataBusOut.read();
        sc_lv<16> readValue;
        //sc_lv<16> readData;
        //std::cout << "[Transactor] writeMem =  " << writeMem.read() << std::endl;
        if (writeMem.read() == SC_LOGIC_1 || readMem.read() == SC_LOGIC_1) {
            std::cout << "[Transactor] Sending MM request by [ " << masterID << " ] : addr=" << addr << ", data=" << writeData << std::endl;
            bus->masterMMreq(addr, writeData, readValue, masterID, readMem.read(), writeMem.read());
            //wait(SC_ZERO_TIME);
            
            dataBus.write(readValue);
            //dataBus = readData;
            //cout << "" << endl;
            wait(SC_ZERO_TIME);
            cout<<" readValue by master [ "<< masterID <<" ] = " << readValue << endl;
            cout<<" dataBus by master [ "<< masterID <<" ] = " << dataBus << endl;
            std::cout
                << "Transactor write "
                << readValue
                << " time="
                << sc_time_stamp()
                << std::endl;
            std::cout << "[Transactor] End of masterMMreq." << std::endl;
            
            memReady.write(SC_LOGIC_1);
        }
    }
}

// Explicit instantiation
//template class Transactor<16>;
// because of
// template<int N>
// SC_MODULE(Transactor) {