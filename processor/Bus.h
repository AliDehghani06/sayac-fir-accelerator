#include <systemc.h>
#include <iostream>

#ifndef BUS_INCLUDE
#define BUS_INCLUDE

template <int N, int M, int S>
SC_MODULE(Bus)
{
    //TODO separate databusses 
public:
    int priorities[M];
    sc_signal<sc_logic> reqsOred;
    sc_signal<sc_logic> gntsOred;

    sc_in<sc_logic> clk;

    // Target ports
    sc_out<sc_logic> t_cs[S];   // Chip Select, target is selected
    sc_out<sc_lv<N>> t_addr[S]; // Address in target's local address space
    sc_out<sc_lv<N>> t_in[S];   // Data going to target
    sc_in<sc_lv<N>> t_out[S];   // Data coming from target
    sc_out<sc_logic> t_wr[S];   // Write operation
    sc_out<sc_logic> t_rd[S];   // Read operation
    sc_in<sc_logic> t_ready[S]; // Operation is done

    sc_lv<N> startAddress[S]; // Address space start
    sc_lv<N> sizeAddress[S];  // Address space size

    sc_signal<sc_lv<N>> bubbleReg[M];

    // Initiator ports
    sc_in<sc_lv<N>> i_addr[M];   // Address
    sc_out<sc_lv<N>> i_in[M];    // Data going to initiator
    sc_in<sc_lv<N>> i_out[M];    // Data coming from initiator
    sc_in<sc_logic> i_wr[M];     // Write operation
    sc_in<sc_logic> i_rd[M];     // Read operation
    sc_out<sc_logic> i_ready[M]; // Operation is done


    sc_signal<sc_logic> req[M];    // Request for bus access
    sc_signal<sc_logic> gnt[M];   // Bus access granted
    sc_signal<sc_logic> ready[M];   // Bus access granted
    sc_signal<sc_logic> cs[S];   // Bus access granted


    sc_signal<sc_lv<N>> addressBus;
    sc_signal<sc_lv<N>,SC_MANY_WRITERS> dataBus;
    sc_signal<sc_logic> readBus;
    sc_signal<sc_logic> writeBus;
    sc_signal<sc_logic> targetReadyBus;
    sc_signal<sc_lv<N>> localAddressBus;

    SC_CTOR(Bus)
    {
        // Set the initial priorities
        for (int i = 0; i < M; i++)
            priorities[i] = i;

        SC_THREAD(initiatorBubbleCombProc);
        for (int i = 0; i < M; i++)
        {
            sensitive << i_wr[i] << i_rd[i] << i_out[i] << i_addr[i] << bubbleReg[i] << gnt[i] << ready[i];
        }

        SC_THREAD(reqProc);
        for (int i = 0; i < M; i++)
        {
            sensitive << req[i] << gnt[i];
        }



        SC_THREAD(targetBubbleProc);
        for (int i = 0; i < S; i++)
        {
            sensitive << cs[i] << t_ready[i] << t_out[i];
        }
                    //cout << "wee" << endl;

        sensitive << dataBus << localAddressBus << readBus << writeBus;
            //cout << "wee" << endl;

        SC_THREAD(addressDecoderProc);
        sensitive << addressBus << gntsOred;

        SC_THREAD(arbiter);
        sensitive << clk;

        SC_THREAD(initiatorBubbleSeqProc);
        sensitive << clk;

    }

  

    void targetBubbleProc()
    {
        while(1)
        {
            wait();
            cout << "Target Bubble Proc" << endl;
            for (int i = 0; i < S; i++)
            {
                t_in[i] = dataBus;
                t_rd[i] = readBus;
                t_wr[i] = writeBus;
                t_addr[i] = localAddressBus;
                t_cs[i] = cs[i];
                if(cs[i] == '1')
                {
                    targetReadyBus = t_ready[i];
                    if(readBus == '1')
                    {
                        dataBus = t_out[i];
                    }
                }
            }
        }
    }

    void addressDecoderProc()
    {
        while(1)
        {
            wait();

            // Find the target that initiator is addressing
            int currenttarget = -1;
            for (int i = 0; i < S; i++)
            {
                cs[i] = sc_logic_0; // Deselect all targets
            }
            for (int i = 0; i < S; i++)
            {
                if (gntsOred == '1' && addressBus.read().to_uint() >= startAddress[i].to_uint() && addressBus.read().to_uint() < startAddress[i].to_uint() + sizeAddress[i].to_uint())
                {
                    
                    currenttarget = i;
                    break;
                }
            }
            cout << "Selected target : " << currenttarget << endl;
            if(currenttarget!=-1){
                // Set the target's address
                localAddressBus = addressBus.read().to_uint() - startAddress[currenttarget].to_uint();
                // Enable cs, selecting the target
                cs[currenttarget] = sc_logic_1;
            }
            else
            {
                localAddressBus.write(sc_lv<N>(SC_LOGIC_Z));
            }
        }
    }

    void initiatorBubbleCombProc()
    {
        while(1)
        {
            wait();
            cout << "Initiator Bubble Comb Proc" << endl;
            writeBus = SC_LOGIC_Z;
            readBus = SC_LOGIC_Z;
            addressBus = sc_lv<N>(SC_LOGIC_Z);
         
            for (int i = 0; i < M; i++)
            {
                req[i] = i_wr[i] | i_rd[i];
                i_in[i] = bubbleReg[i];
                i_ready[i] = ready[i] | ~(i_wr[i] | i_rd[i]);//Check
                if(gnt[i] == '1')
                {
                    writeBus = i_wr[i];
                    readBus = i_rd[i];
                    addressBus = i_addr[i];
                    if(i_wr[i] == '1')
                    {
                        dataBus = i_out[i];
                    }
                }
            }
            
        }
    }
    void initiatorBubbleSeqProc()
    {
        for (int i = 0; i < M; i++)
        {
            
                bubbleReg[i] = sc_lv<N>(SC_LOGIC_0);
            
        }
        while(1)
        {
            wait(clk->posedge_event());
            for (int i = 0; i < M; i++)
            {
                if((i_rd[i] & targetReadyBus) == '1')
                {
                    bubbleReg[i] = dataBus;
                }
            }
            
        }
    }

    void reqProc()
    {
        reqsOred = sc_logic_0;
        gntsOred = sc_logic_0;
        while (1)
        {
            wait();
            cout << "ReqProc" << endl;
            sc_logic _req = sc_logic_0;

            for (int i = 0; i < M; i++)
            {
                _req = _req | req[i];
            }

            reqsOred = _req;

            sc_logic _gnt = sc_logic_0;

            for (int i = 0; i < M; i++)
            {
                _gnt = _gnt | gnt[i];
            }

            gntsOred = _gnt;
        }
    }

    void arbiter()
    {
        for (int i = 0; i < M; i++)
            {
                gnt[i] = SC_LOGIC_0;
                ready[i] = sc_logic_0;
            }
        while (1)
        {
            
            // Wait for a request
            do
            {
                wait(clk->posedge_event());
            } while (reqsOred != '1');

            for (int i = 0; i < M; i++)
            {
                gnt[i] = SC_LOGIC_0;
                ready[i] = sc_logic_0;
            }
            
            cout << "Arbiting" << endl;
            // Find the requesting initiator with the most priority
            int currentinitiator = 0;
            for (int i = 0; i < M; i++)
            {
                cout << "prior[" << i << "]  : " << priorities[i] << endl;
                if (req[priorities[i]] == '1')
                {
                    currentinitiator = priorities[i];
                    cout << "cand : "<< currentinitiator<< endl;
                    for (int j = i; j < M - 1; j++)
                        priorities[j] = priorities[j + 1];
                    priorities[M - 1] = currentinitiator;
                    break;
                }
            }
            gnt[currentinitiator] = SC_LOGIC_1;
            ready[currentinitiator] = sc_logic_0;
            cout << "Req granted for : " << currentinitiator << endl;
            do
            {
                wait(clk->posedge_event());
            } while (targetReadyBus != '1');
   
            
            // Tell the initiator the operation is done
            ready[currentinitiator] = sc_logic_1;
            wait(clk->posedge_event());
            ready[currentinitiator] = sc_logic_0;

            gnt[currentinitiator] = SC_LOGIC_0;
            
        }
    }
};

#endif
