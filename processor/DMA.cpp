#include <systemc.h>
#include <iostream>
#include "Bus.h"
SC_MODULE(DMA)
{
    sc_lv<16> tempReg;

    sc_lv<16> fromAddress; // Address to copy from
    sc_lv<16> byteCount;   // Number of bytes to copy
    sc_lv<16> toAddress;   // Address to copy to
    sc_lv<16> controlReg;  // ...|RD|WR|Start, WR : Mem -> MMA , RD : MMA -> Mem
    sc_lv<16> statusReg;   // ...|RD Done|WR Done

    sc_in<sc_logic> clk;

    // Master port for accessing the memory

    sc_out<sc_lv<16>> i_addr;
    sc_in<sc_lv<16>> i_in;
    sc_out<sc_lv<16>> i_out;
    sc_out<sc_logic> i_wr;
    sc_out<sc_logic> i_rd;
    sc_in<sc_logic> i_ready;

    // Config port
    
    sc_in<sc_logic> t_cs;
    sc_in<sc_lv<16>> t_addr;
    sc_in<sc_lv<16>> t_in;
    sc_out<sc_lv<16>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;

    // Signals between DMA and MMA
    sc_out<sc_lv<16>> mmaAddr;
    sc_out<sc_lv<16>> mmaIn;
    sc_in<sc_lv<16>> mmaOut;
    sc_out<sc_logic> mmaWR;
    sc_out<sc_logic> mmaRD;

    // Interrupt to PIC
    sc_out<sc_logic> interrupt;

    SC_CTOR(DMA)
    {

        SC_THREAD(eval);
        sensitive << clk;
        SC_THREAD(evalConf);
        sensitive << clk;
    }

    void evalConf()
    {
        while (true)
        {
            t_ready = sc_logic_1;
            // Wait for CS
            do
            {
                wait(clk->posedge_event());
            } while (t_cs != '1');
            t_ready = sc_logic_0;
            t_out = 0;
            if (t_wr == '1')
            {
                wait(clk->posedge_event());
                cout << "configuring DMA" << endl;
                if (t_addr.read().to_uint() == 0) // Address 0 => control register
                {
                    controlReg = t_in;
                }
                else if (t_addr.read().to_uint() == 1) // Address 1 => from register
                {
                    fromAddress = t_in;
                }
                else if (t_addr.read().to_uint() == 2) // Address 2 => to register
                {
                    toAddress = t_in;
                }
                else if (t_addr.read().to_uint() == 3) // Address 3 => size register
                {
                    byteCount = t_in;
                }

            }
            else if (t_rd == '1')
            {
                if (t_addr.read().to_uint() == 0) // Address 0 => control register
                {
                    t_out = controlReg;
                }
                else if (t_addr.read().to_uint() == 1) // Address 1 => from register
                {
                    t_out = fromAddress;
                }
                else if (t_addr.read().to_uint() == 2) // Address 2 => to register
                {
                    t_out = toAddress;
                }
                else if (t_addr.read().to_uint() == 3) // Address 3 => size register
                {
                    t_out = byteCount;
                }
                else if (t_addr.read().to_uint() == 4) // Address 4 => status register
                {
                    t_out = statusReg;
                }
            }
             
            t_ready = sc_logic_1;
        }
    }

    void eval()
    {
        controlReg = 0;
        statusReg = 0;
        interrupt = sc_logic_0;
        i_addr = 0;
        i_out = 0;
        i_rd = sc_logic_0;
        i_wr = sc_logic_0;
        mmaAddr = 0;
        mmaIn = 0;
        mmaWR = sc_logic_0;
        mmaRD = sc_logic_0;
        while (true)
        {

            // Wait for start
            do
            {
                wait(clk->posedge_event());
            } while (controlReg[0] == '0');

            // Set start,done and interrupt to zero
            controlReg[0] = sc_logic_0;
            statusReg = 0;
            interrupt = sc_logic_0;
            i_addr = 0;
            i_out = 0;
            i_rd = sc_logic_0;
            i_wr = sc_logic_0;
            mmaAddr = 0;
            mmaIn = 0;
            mmaWR = sc_logic_0;
            mmaRD = sc_logic_0;

            // For each byte do the transfer
            for (int addrOffset = 0; addrOffset < byteCount.to_uint(); addrOffset++)
            {
                // cout << "dma : " <<  addrOffset << endl;
                if (controlReg[1] == '1') //WR Write  Mem -> MMA
                {
                    // Set the memory address and control signals
                    i_addr = fromAddress.to_uint() + addrOffset;
                    i_wr = sc_logic_0;
                    i_rd = sc_logic_1;

                    // Wait for the operation to finish
                    do
                    {
                        wait(clk->posedge_event());
                    } while (i_ready != '1');
                    i_wr = sc_logic_0;
                    i_rd = sc_logic_0;
                    // Save the result
                    tempReg = i_in;
                    // Set the destination address and control signals
                    mmaAddr = toAddress.to_uint() + addrOffset;
                    mmaRD = sc_logic_0;
                    mmaWR = sc_logic_1;
                    mmaIn = tempReg;
                    // Write the data in one clock
                    wait(clk->posedge_event());
                 
                }
                else if (controlReg[2] == '1') //RD Read MMA -> Mem
                {
                    // Set the source address and control signals
                    mmaAddr = fromAddress.to_uint() + addrOffset;
                    mmaRD = sc_logic_1;
                    mmaWR = sc_logic_0;
                    // Read the data in one clock
                    wait(clk->posedge_event());  
                    wait(clk->posedge_event());  
                    tempReg = mmaOut;

                    // Set the memory destination address and control signals and data
                    i_addr = toAddress.to_uint() + addrOffset;
                    i_wr = sc_logic_1;
                    i_rd = sc_logic_0;
                    i_out = tempReg;

         

                    // Wait for operation to finish
                    do
                    {
                        wait(clk->posedge_event());
                    } while (i_ready != '1');

                    i_wr = sc_logic_0;
                    i_rd = sc_logic_0;
                  
                }
            }

            if(controlReg[2] == '0')
                statusReg = 1; // If it was WR operation, set status to WR done
            else
                statusReg = 2; // If it was RD operation, set status to RD done
          

            // Issue interrupt for one clock
            interrupt = sc_logic_1;
            wait(clk->posedge_event());
            interrupt = sc_logic_0;
        }
    }
};