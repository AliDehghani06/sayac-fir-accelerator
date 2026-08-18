
#include "SlaveTransactor.h"

void SlaveTransactor::slave_process() {
    std::cout << "[SlaveTransactor] Listening on Slave ID " << slaveID << std::endl;

    while (true) {
        // منتظر فعال‌سازی از سوی master
        //cs.write(SC_LOGIC_0);
        bus_port->slaveMMcollection(slaveID, addr, data, r_en, w_en);
        std::cout << "[SlaveTransactor =  "<< slaveID<<"] Received request : " << std::endl;
        std::cout << "  addr = " << addr << std::endl;
        std::cout << "  data = " << data << std::endl;
        std::cout << "  r_en = " << r_en << ", w_en = " << w_en << std::endl;
        // ارسال مقادیر به سمت حافظه
        addrBus.write(addr);
        writeData.write(data);
       // dataBus.write(data);
        readMem.write(r_en);
        writeMem.write(w_en);
        cs.write(SC_LOGIC_1);
        wait(SC_ZERO_TIME);
        std::cout << "  cs = " << cs.read() <<  std::endl;

        wait(slave_done);
        cs.write(SC_LOGIC_0);
        readMem.write(SC_LOGIC_0);
        writeMem.write(SC_LOGIC_0);
    }
}

void SlaveTransactor::on_mem_ready() {
    if (memReady.read() == SC_LOGIC_1) {
        
        std::cout << "[SlaveTransactor] memReady = 1 detected, calling slaveMMresponse() from : " << slaveID << std::endl;

        
        sc_lv<16> read_value = dataBus.read();

        std::cout << "[SlaveTransactor] data from memory = " << read_value << std::endl;

        
        bus_port->slaveMMresponse(read_value);

        slave_done.notify();
    }
}

