#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Bus.h"


template <int N>
SC_MODULE(Memory)
{
	int DebugON = 0;
	int Loading = 1;
	int StartingLocation = 0;
	int PuttingData = 0;
	std::string file;

	sc_in<sc_logic> clk;

	sc_in<sc_logic> t_cs;    
    sc_in<sc_lv<N>> t_addr; 
    sc_in<sc_lv<N>> t_in; 
    sc_out<sc_lv<N>> t_out;
    sc_in<sc_logic> t_wr;
    sc_in<sc_logic> t_rd;
    sc_out<sc_logic> t_ready;

	int memRange;
	sc_lv<N> *mem;



	SC_CTOR(Memory)
	{

		memRange = int(pow(2, N));
		mem = new sc_lv<N>[memRange];

		SC_THREAD(init);
		SC_METHOD(readMem);
		sensitive << t_addr << t_cs << t_rd;
		SC_METHOD(writeMem);
		sensitive << clk.pos()<< t_wr;
		SC_THREAD(dump);
		SC_METHOD(setMemReady);
		sensitive << t_addr << t_cs << t_rd << t_wr;
	}
	void init() {
		int i = 0;
		sc_lv<N> data;
		std::string str_data;

		std::ifstream initFile("default_meminit.txt");
	
		std::ifstream initFileLoad;
		std::ifstream PutAddr("addr.txt");
		std::ifstream PutData("data.txt");
		file = "mem.txt";
		
		if (Loading ) {

			initFileLoad.open(file);
			if (!initFileLoad.is_open()) {
				std::cerr << "[ERROR] Cannot open file: " << file << std::endl;
				return;
			}

			int count = 0;
			while (getline(initFileLoad, str_data)) {
				if (!str_data.empty()) {
					if (count >= StartingLocation && i < memRange) {
						data = sc_lv<N>(str_data.c_str());
						mem[i++] = data;
					}
					count++;
				}
			}
			initFileLoad.close();
			if (DebugON) {
				std::cout << "[MEM] Loaded memory from file: " << file << std::endl;
			}
		}

		
		else if (PuttingData) {
			std::string addr_str, data_str;
			while (getline(PutAddr, addr_str) && getline(PutData, data_str)) {
				if (!addr_str.empty() && !data_str.empty()) {
					int addr = std::stoi(addr_str);
					if (addr < memRange) {
						data = sc_lv<N>(data_str.c_str());
						mem[addr] = data;
						std::cout << "[MEM] OK. addr = " << addr <<" , data = " << data << std::endl;
					}
				}
			}
			PutAddr.close();
			PutData.close();
			if (DebugON) {
				std::cout << "[MEM] Put data from addr.txt and data.txt" << std::endl;
			}
		}

		else {
			if (!initFile.is_open()) {
				std::cerr << "[ERROR] Cannot open default init file!" << std::endl;
				return;
			}
			while (getline(initFile, str_data)) {
				if (!str_data.empty() && i < memRange) {
					data = sc_lv<N>(str_data.c_str());
					mem[i++] = data;
				}
			}
			initFile.close();
			if (DebugON) {
				std::cout << "[MEM] Initialized memory from default file" << std::endl;
			}
		}
	}


	void readMem()
	{
		sc_lv<N> tempAdr;
		tempAdr = t_addr;
		t_out = 0;
		if (t_cs == '1')
		{
			if (t_rd == '1')
			{
				if (tempAdr.to_uint() < memRange)
				{
					t_out = mem[tempAdr.to_uint()];
				}
			}
		}
	}
	void writeMem()
	{
		sc_lv<N> tempAd;

		if (t_cs == '1')
		{
			tempAd = t_addr;
			if (tempAd.to_uint() < memRange)
			{
				if (t_wr == '1')
				{
					mem[tempAd.to_uint()] = t_in;
					cout<< "[Memory] mem["<< tempAd.to_uint() << "] = "<< mem[tempAd.to_uint()] << endl;
				}
			}
		}
	}
	void dump()
	{
		ofstream out;
		wait(19999999, SC_NS);
		out.open("dump.txt");
		for (int i = 0; i < memRange; i++)
		{
			out << i << "\t" << mem[i] << endl;
		}
		out.close();
	}
	void setMemReady()
	{
		sc_lv<N> tempAd;
		t_ready = SC_LOGIC_0;
		// cout << "ready Ready is " << ready << "\n";
		if (t_cs == '1')
		{
			tempAd = t_addr;
			if (tempAd.to_uint() < memRange)
			{
				if (t_wr == '1' || t_rd == '1')
				{
					t_ready = SC_LOGIC_1;
				}
			}
		}
		
	}
};
