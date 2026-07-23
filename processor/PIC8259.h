#include <iostream>
#include <systemc.h>
#include <string>

// Enum for priority policies
enum PriorityPolicy {
    FIXED_PRIORITY,
    ROTATING_PRIORITY
};

SC_MODULE(PIC8259) {
    // Input:
    sc_in<sc_logic> clk;
    sc_in<sc_lv<8>> IR;
    sc_in<sc_logic> INTA_bar; //interrupt acknowledge
    sc_in<sc_logic> RD_bar, WR_bar, CS_bar;
    sc_in<sc_lv<16>> PIC_input;

    // Output:
    sc_out<sc_logic> INT;
    sc_out<sc_lv<16>> PIC_output;

    // registers for ISR address
    sc_lv<5> ISR_address[8];
    // Mask Register // if any interrupt pin is masked, then it is disabled
    sc_lv<8> IMR;

    int reg_index = 0;
    int priority_start_num = 0;
    PriorityPolicy policy;
    int ready_interrupt;

    SC_CTOR(PIC8259) : policy(ROTATING_PRIORITY), ready_interrupt(-1) {
        SC_THREAD(init);

        SC_THREAD(register_write);
        sensitive << clk.pos();

        SC_THREAD(handler);
        sensitive << clk.pos();
        dont_initialize();
    }

    int priority_resolver();
    void init();
    void register_write();
    void handler();
};

void PIC8259::init() {
    IMR = "00000000";
    priority_start_num = 0;
    ready_interrupt = -1;
    for (int i = 0; i < 8; i++) {
        ISR_address[i] = "00000011";
    }
}

int PIC8259::priority_resolver() {
    if (policy == FIXED_PRIORITY) {
        for (int i = 0; i < 8; ++i) {
            if (IR.read()[i] == SC_LOGIC_1 && IMR[i] == SC_LOGIC_0) {
                return i;
            }
        }
    }
    else {
        for (int i = priority_start_num; i < priority_start_num + 8; ++i) {
            int idx = i % 8;
            if (IR.read()[idx] == SC_LOGIC_1 && IMR[idx] == SC_LOGIC_0) {
                priority_start_num = (idx + 1) % 8;
                return idx;
            }
        }
    }
    return -1;
}


void PIC8259::register_write() {
    while (true) {
        wait();

        if (CS_bar.read() == SC_LOGIC_0 && WR_bar.read() == SC_LOGIC_0) {
            sc_lv<16> input_data = PIC_input.read();

            //write in ISR registers. only the 5 lower bits of the 16 bit inputs are used for ISR address
            if (input_data[6] == SC_LOGIC_1) {
                if (reg_index < 8) {
                    sc_lv<8> data_5bits = "00000000";
                    for (int i = 0; i < 5; i++) {
                        data_5bits[i] = input_data[i];
                    }
                    ISR_address[reg_index] = data_5bits;

                    cout << "PIC8259: ISR_address[" << reg_index << "] set to " << ISR_address[reg_index]
                        << " at " << sc_time_stamp() << endl;

                    reg_index++;
                }
            }
            //write in interrupt mask register(IMR)
            else if (input_data[6] == SC_LOGIC_0 && input_data[7] == SC_LOGIC_1) {
                sc_lv<8> data_5bits = "00000000";
                for (int i = 0; i < 5; i++) {
                    data_5bits[i] = input_data[i];
                }
                IMR = data_5bits;

                cout << "PIC8259: IMR set to " << IMR << " at " << sc_time_stamp() << endl;
            }
        }
    }
}


void PIC8259::handler() {
    while (true) {
        wait();
        int int_index = priority_resolver();
        if (int_index != -1) {
            ready_interrupt = int_index;
            INT.write(SC_LOGIC_1);
            cout << "PIC8259: INT asserted for IRQ " << int_index << " at " << sc_time_stamp() << endl;

            while (INTA_bar.read() == SC_LOGIC_1) {
                wait();
            }

            sc_lv<16> interrupt_address = 0;
            interrupt_address.range(2, 0) = sc_lv<3>(int_index);
            interrupt_address.range(7, 3) = ISR_address[int_index];
            PIC_output.write(interrupt_address);
            wait();

            cout << "PIC8259: ISR is sent to the processor at " << sc_time_stamp() << endl;

            INT.write(SC_LOGIC_0);
            PIC_output.write("0000000000000000");
            ready_interrupt = -1;
        }
        else {
            INT.write(SC_LOGIC_0);
        }
    }
}


