#include <iostream>
#include <systemc.h>
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include "System_Filter.h"

/*
g++ TB_Filter.cpp \
    -I$HOME/systemc-3.0.2/install/include \
    -L$HOME/systemc-3.0.2/install/lib \
    -Wl,-rpath,$HOME/systemc-3.0.2/install/lib \
    -lsystemc \
    -o filter_sim

./filter_sim > result_filter.txt
*/

SC_MODULE(SystemTester)
{
    sc_signal<sc_logic> clk;

    EmbeddedSystem<16> *systemModule;

    SC_CTOR(SystemTester)
    {
        systemModule = new EmbeddedSystem<16>("systemModule");
        (*systemModule)(clk);

        SC_THREAD(clocking);
    }

    void clocking();
};
void SystemTester::clocking()
{
    clk = SC_LOGIC_0;
    while (true)
    {
        wait(2.5, SC_NS);
        clk = SC_LOGIC_0;
        wait(2.5, SC_NS);
        clk = SC_LOGIC_1;
    }
}

int sc_main(int argc, char *argv[])
{
    SystemTester *TOP = new SystemTester("systemTest_TB");
    TOP->systemModule->memory->Loading = 1;

    sc_start(20000000, SC_NS);

    return 0;
}
