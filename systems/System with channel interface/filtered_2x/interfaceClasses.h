// ---------------------- interfaceClasses.h ----------------------
#pragma once

#include <systemc.h>

template <int N>
class masterSide_if : virtual public sc_interface {
public:
    virtual void masterMMreq(sc_lv<N> addr, sc_lv<N> writeData, sc_lv<16>& raeadData, int masterID,
        sc_logic readMem, sc_logic writeMem) = 0;
};

template <int N>
class slaveSide_if : virtual public sc_interface {
public:
    virtual void slaveMMcollection(int slaveID,
        sc_lv<16>& addr,
        sc_lv<16>& data,
        sc_logic& readEnable,
        sc_logic& writeEnable) = 0;

    virtual void slaveMMresponse( sc_lv<16>& readData) = 0;

};