// ---------------------- busChannel.cpp ----------------------
#include "busChannel.h"

// Return the master ID with the highest priority that is currently requesting the bus
int busChannel::getPriorityMaster() {
    if (requesting[0]) return 0;  // Master 0 has highest priority
    if (requesting[1]) return 1;  // Master 1 has lower priority
    return -1;                    // No master is requesting
}

// Master sends memory request to the bus
void busChannel::masterMMreq(sc_lv<16> addr, sc_lv<16> writeData, sc_lv<16>& raeadData, int masterID,
    sc_logic readMem, sc_logic writeMem) {
    requesting[masterID] = true; // Mark this master as requesting access

    // Wait until this master becomes the highest priority requesting master
    while (getPriorityMaster() != masterID) {
        wait(slaveOprCompleted); // Wait until another master finishes
    }

    // Now this master is allowed to access the bus
    busBusy.lock();              // Lock the bus
    requesting[masterID] = false; // Clear the request flag

    // Set data on the shared bus
    addrOut = addr;
    writeDataOut = writeData;
    readMemOut = readMem;
    writeMemOut = writeMem;
    masterIDOut = masterID;

    // Decode target slave based on address
    goingToSlave = decodeSlave(addr.to_uint());

    if (goingToSlave >= 0 && goingToSlave <= 4) {
        std::cout << "[busChannel] goingToSlave " << goingToSlave << std::endl;
        requestMM[goingToSlave].notify(SC_ZERO_TIME); // Notify the selected slave

        // Wait for slave to respond
        wait(slaveOprCompleted);
        raeadData = readDataIn; // Return read data back to master
        std::cout << "[busChannel] slaveOprCompleted notified (memory ready). raeadData = " << raeadData << std::endl;
    }
    else {
        std::cout << "[busChannel] Invalid slave ID, skipping wait." << std::endl;
    }

    // Release the bus 
    busBusy.unlock();
    //slaveOprCompleted.notify(SC_ZERO_TIME);
}

// Slave retrieves the request sent by the master
void busChannel::slaveMMcollection(int slaveID,
    sc_lv<16>& addr,
    sc_lv<16>& data,
    sc_logic& readEnable,
    sc_logic& writeEnable) {
    std::cout << "[busChannel] waiting for slave ID = " << slaveID << ", goingToSlave = "<<goingToSlave << std::endl;
    if (slaveID != goingToSlave) {
        wait(requestMM[slaveID]); // Wait for request notification
    }
    // Read request details from the bus
    int base = (slaveID >= 0 && slaveID < 5) ? slaveBaseAddress[slaveID] : 0;
    addr = (int)(addrOut.to_uint() - base);
    data = writeDataOut;
    readEnable = readMemOut;
    writeEnable = writeMemOut;
    goingToSlave = -1;
    
}

// Map address to a specific slave (address decoding logic)
int busChannel::decodeSlave(sc_uint<16> addrInt) {
    const int MMA_BASE = 0xC000, MMA_END = 0xC007;
    const int DMA_BASE = 0xC008, DMA_END = 0xC00F;
    const int PIC_BASE = 0xC010, PIC_END = 0xC014;
    const int FIR_BASE = 0xC015, FIR_END = 0xC036;
    const int MEM_BASE = 0x0000, MEM_END = 0xBFFF;

    if (addrInt >= MMA_BASE && addrInt <= MMA_END) {
        std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
            << " -> Slave 0 (MMA)" << std::dec << std::endl;
        return 0;
    }
    else if (addrInt >= DMA_BASE && addrInt <= DMA_END) {
        std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
            << " -> Slave 1 (DMA)" << std::dec << std::endl;
        return 1;
    }
    else if (addrInt >= MEM_BASE && addrInt <= MEM_END) {
        std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
            << " -> Slave 2 (Memory)" << std::dec << std::endl;
        return 2;
    }
    else if (addrInt >= PIC_BASE && addrInt <= PIC_END) {
        std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
            << " -> Slave 3 (PIC)" << std::dec << std::endl;
        return 3;
    }
    else if (addrInt >= FIR_BASE && addrInt <= FIR_END) {
        std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
            << " -> Slave 4 (FIR)" << std::dec << std::endl;
        return 4;
    }

    std::cout << "[decodeSlave] Address 0x" << std::hex << addrInt
        << " -> Invalid (no slave match)" << std::dec << std::endl;
    return -1;
}

// Slave sends read data back to master
void busChannel::slaveMMresponse(sc_lv<16>& readData) {
    std::cout << "[busChannel::slaveMMresponse] Read Data = " << readData << std::endl;

    readDataIn = readData;               // Save data into bus channel
    slaveOprCompleted.notify(SC_ZERO_TIME);  // Notify master waiting for the response
    
}
