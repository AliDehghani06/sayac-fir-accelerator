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
#include "System_X2.h"

/*
g++ TB_X2.cpp -I$HOME/systemc-3.0.2/install/include -L$HOME/systemc-3.0.2/install/lib \
    -Wl,-rpath,$HOME/systemc-3.0.2/install/lib -lsystemc -o x2_sim

./x2_sim > result_x2.txt
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

    // 44 chunks + a final 22050-sample extraction loop need far more
    // simulated time than the Phase 3 smoke test did.
    sc_start(20000000, SC_NS);

    return 0;
}
