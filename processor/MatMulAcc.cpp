#include <systemc.h>
#include <iostream>
#include "Bus.h"
template <int N>
SC_MODULE(MatMulAcc)
{
    const int MAX_SIZE = (1 << N);
    // mat1 : n*k
    // mat2 : k*m
    // TODO : Change to use one buffer for all
    //int *mat1;   // Offset : 0
    //int *mat2;   // Offset : MAX_SIZE
    //int *outMat; // Offset : 2*MAX_SIZE
    int *buff;

    sc_in<sc_logic> clk;

    // Config port
    sc_in<sc_logic> t_cs;    
    sc_in<sc_lv<N>> t_addr; 
    sc_in<sc_lv<N>> t_in; 
    sc_out<sc_lv<N>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;

    // Interrupt to PIC
    sc_out<sc_logic> interrupt;

    // Signals between DMA and MMA
    sc_in<sc_lv<N>> dmaAddr;
    sc_in<sc_lv<N>> dmaIn;
    sc_out<sc_lv<N>> dmaOut;
    sc_in<sc_logic> dmaWR;
    sc_in<sc_logic> dmaRD;

    // Start
    sc_lv<16> controlReg;
    // Done
    sc_lv<16> statusReg;
    // Matrix size
    sc_lv<16> n, k, m;

    SC_CTOR(MatMulAcc)
    {
        controlReg = 0;
        statusReg = 0;

       
    
        //mat1 = new int[MAX_SIZE];
        //mat2 = new int[MAX_SIZE];
        //outMat = new int[MAX_SIZE];
        buff = new int[MAX_SIZE];

        //memset(mat1, 0, sizeof(int) * MAX_SIZE);
        //memset(mat2, 0, sizeof(int) * MAX_SIZE);
        //memset(outMat, 0, sizeof(int) * MAX_SIZE);
        memset(buff, 0, sizeof(int) * MAX_SIZE);

 
        SC_THREAD(evalConfigReg);
        sensitive << clk;
        SC_THREAD(eval);
        sensitive << clk;
        SC_THREAD(evalMem);
        sensitive << clk;
    }


    void evalConfigReg()
    {
        while (true)
        {
            
            t_ready = sc_logic_1;
            // Wait for CS
            do
            {
                wait(clk->posedge_event());
            } while (t_cs != '1');
            t_out = 0;
            t_ready = sc_logic_0;
            if (t_wr == '1') // Write
            {
                wait(clk->posedge_event());
                if (t_addr.read().to_uint() == 0) // Address 0 => control register
                {
                    controlReg = t_in;
                }
                else if (t_addr.read().to_uint() == 1) // Address 1 => n register
                {
                    n = t_in;
                }
                else if (t_addr.read().to_uint() == 2) // Address 2 => k register
                {
                    k = t_in;
                }
                else if (t_addr.read().to_uint() == 3) // Address 3 => m register
                {
                    m = t_in;
                }
  
            }
            else if (t_rd == '1')
            {
                if (t_addr.read().to_uint() == 0) // Address 0 => control register
                {
                    t_out = controlReg;
                }
                else if (t_addr.read().to_uint() == 1) // Address 1 => n register
                {
                    t_out = n;
                }
                else if (t_addr.read().to_uint() == 2) // Address 2 => k register
                {
                    t_out = k;
                }
                else if (t_addr.read().to_uint() == 3) // Address 3 => m register
                {
                    t_out = m;
                }
                else if (t_addr.read().to_uint() == 4) // Address 4 => status register
                {
                    t_out = statusReg;
                }
            }
            
            t_ready = sc_logic_1;
        }
    }

    void evalMem()
    {
        dmaOut = 0;
        while (true)
        {
            
            // Wait for clock
            wait(clk->posedge_event());
            int localAddress = dmaAddr.read().to_uint();
            if (dmaRD == '1') // Read operation pending
            {
                // If outMat is addressed, return the data
                //if (localAddress >= 2 * MAX_SIZE && localAddress < 3 * MAX_SIZE)
                //    dmaOut = outMat[localAddress - 2 * MAX_SIZE];
                dmaOut = buff[localAddress];
            }
            else if (dmaWR == '1') // Write operation pending
            {

                //if (localAddress < MAX_SIZE) // Mat1 being addressed
                //    mat1[localAddress] = dmaIn.read().to_int();
                //else if (localAddress >= MAX_SIZE && localAddress < 2 * MAX_SIZE) // Mat2 being addressed
                //    mat2[localAddress - MAX_SIZE] = dmaIn.read().to_int();
                buff[localAddress] = dmaIn.read().to_int();
            }
        }
    }

    void eval()
    {
        while (true)
        {
            interrupt = sc_logic_0;
            // Wait for start
            do
            {
                wait(clk->posedge_event());
                //cout << controlReg.to_uint() << endl;
            } while (controlReg.to_uint() == 0);

            // Set start,done and interrupt to zero
            controlReg = 0;
            statusReg = 0;
            interrupt = sc_logic_0;
            
            
            int mat1Size = n.to_uint() * k.to_uint();
            int mat2Size = k.to_uint() * m.to_uint();
            int outMatSize = n.to_uint() * m.to_uint();



            //Debug
            for (int i = 0; i < mat1Size; i++)
            {
                cout << "mat1[" << i << "] = " << buff[i] << endl;
            }
            for (int i = 0; i < mat2Size; i++)
            {
                cout << "mat2[" << i << "] = " << buff[i+mat1Size] << endl;
            }


            for (int x = 0; x < n.to_uint(); x++)
            {
                for (int y = 0; y < m.to_uint(); y++)
                {
                    int sum = 0;
                    for (int z = 0; z < k.to_uint(); z++)
                    {
                        // MAC operation
                        sum += buff[x * k.to_uint() + z] * buff[mat1Size + m.to_uint() * z + y];
                        // Wait a clock, assuming each MAC operation takes one clock to complete.
                        wait(clk->posedge_event());
                    }
                    // Store the result
                    buff[mat1Size + mat2Size + x * m.to_uint() + y] = sum;
                }
            }

            //Debug
            for (int i = 0; i < outMatSize; i++)
            {
                cout << "outMat[" << i << "] = " << buff[i+mat1Size+mat2Size] << endl;
            }
            
            // Set done to one
            statusReg = 1;

            // Issue interrupt for one clock
            interrupt = sc_logic_1;
            wait(clk->posedge_event());
            interrupt = sc_logic_0;
        }
    }
};