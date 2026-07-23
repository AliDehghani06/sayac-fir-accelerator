#include <iostream>
#include <systemc.h>
#include "power.h"

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
class sayacInstruction : public sc_module
{
public:

	sc_in <sc_logic> clk;
	sc_in <sc_logic> memReady;

	sc_in<sc_logic> interrupt;

	sc_in <sc_lv<N>> dataBus;
	sc_out <sc_lv<N>> dataBusOut;

	sc_out <sc_logic> readMem, writeMem; //*****************************
	sc_out <sc_logic> readIO, writeIO;
	sc_out <sc_lv<N>> addrBus;

	powerM* powModule;

	int adrSpace;


	sc_lv<N> PCregister, ADRregister, IRregister, flagRegister;
	sc_lv<N> powerAccumulator;

	///////////////////TRF
	sc_lv <N>* TRF;
	sc_logic TRF_wrReg, TRF_rdReg;
	sc_lv <regFileAdrBit> TRF_RdADR1, TRF_RdADR2, TRF_WrADR;
	sc_lv <N> TRF_WrData, TRF_RdData1, TRF_RdData2;
	///////////////////

	///////////////////TRB
	sc_lv <N>* TRB;
	sc_logic TRB_wrReg, TRB_rdReg;
	sc_lv <regFileAdrBit> TRB_RdADR1, TRB_RdADR2, TRB_WrADR;
	sc_lv <N> TRB_WrData, TRB_RdData1, TRB_RdData2;
	///////////////////

	//////signals used for interrupt
	sc_lv<N> interruptPC;		//To save PC during interrupt
	sc_lv<N> interruptStatus;	//To save R(15) during interrupt
	sc_logic InterruptHandlerActive;		//Currently handling interrupt
	//for 8259 pic:
	sc_out<sc_logic> INTA_bar;
	/////// <summary>
	/// 
	/// </summary>
	/// <param name=""></param>

	SC_HAS_PROCESS(sayacInstruction);

	enum opCodes
	{
		ORSV0, ORSV1,
		OIns2,
		OIANR, OIANI,
		OIMSI, OIMHI,
		OISLR, OISAR,
		OIADR, OISUR,
		OIADI, OISUI,
		OIMUL, OIDIV,
		OIns15
	};
	enum instructions2
	{
		LDR, STR,
		JMR_JMB, JMI
	};
	/*	enum instructions15
		{
			CMR, CMI,
			BRC, BRR,
			SHI,
			NTR, NTD
		};*/

	enum I15_cmp
	{
		CMR, CMI
	};

	enum I15_brc
	{
		BRC, BRR
	};

	enum I15_not
	{
		NTR, NTD
	};
	enum new_enum
	{
		COMP, BRANCH,SHI, NOT
	};
	sayacInstruction(const sc_module_name)
	{
		adrSpace = int(pow(2, regFileAdrBit));
		TRF = new sc_lv<N>[adrSpace];

		adrSpace = int(pow(2, regFileAdrBit));
		TRB = new sc_lv<N>[adrSpace];

		// *********************only for testing interrupt: Initialize R15 with bit 3 set to 1 (interrupt enabled)
		TRB[7] = 27;
		sc_lv<N> r15_init = 0x0000;
		r15_init[3] = SC_LOGIC_1;
		TRF[15] = r15_init;
		INTA_bar.initialize(SC_LOGIC_1);
		//////////////////////////////////////////////////////////

		powModule = new powerM("pow");

		InterruptHandlerActive = SC_LOGIC_0;

		SC_THREAD(abstractSimulation);
		sensitive << clk.pos();
		//SC_METHOD(display);
		//sensitive << active;
	}

	/*	SC_CTOR(sayacInstruction)
		{
			adrSpace = int(pow(2,regFileAdrBit));
			TRF = new sc_lv<N>[adrSpace];

			SC_THREAD(abstractSimulation);
			sensitive << clk.pos();
		}
		*/
		//void display();
	void abstractSimulation();
	void writeRegTRF(sc_lv <regFileAdrBit> WrADR, sc_lv <N> WrData);
	void readRegTRF(
		sc_lv <regFileAdrBit> address1, sc_lv <regFileAdrBit> address2
	);
	void writeRegTRB(sc_lv <regFileAdrBit> WrADR, sc_lv <N> WrData);
	void readRegTRB(
		sc_lv <regFileAdrBit> address1, sc_lv <regFileAdrBit> address2
	);
	sc_lv<N> nBitSignExtension(sc_lv<N> inputSignal, int nBit, bool sign);
	sc_lv<N> shiftFunction(sc_lv<5> shiftNumber, sc_lv<N> input, bool logical);


	void InterruptHandler();
};

/*template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> :: display()
{
	if (active == SC_LOGIC_1)
			cout << "Power is:  " << powerAccumulator << endl;
}*/

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::writeRegTRF
(sc_lv <regFileAdrBit> WrADR, sc_lv <N> WrData)
{
	sc_uint <regFileAdrBit> wrAd;
	if (TRF_wrReg == '1')
	{
		wrAd = WrADR;
		TRF[wrAd] = WrData;
		cout << "TRF Write Data: " << "TRF[" << wrAd << "]= " << TRF[wrAd] << "	Time :   " << sc_time_stamp() << endl;
	}
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::readRegTRF
(sc_lv <regFileAdrBit> address1, sc_lv <regFileAdrBit> address2)
{
	sc_uint<regFileAdrBit> ad1, ad2;
	ad1 = address1;
	ad2 = address2;
	TRF_RdData1 = TRF[ad1];
	TRF_RdData2 = TRF[ad2];
	if (ad1 != ad2) {
		cout << "TRF read Data: " << "TRF[" << ad1 << "]= " << TRF_RdData1 << endl;
		cout << "TRF read Data: " << "TRF[" << ad2 << "]= " << TRF_RdData2 << endl;
	}
	else {
		cout << "TRF read Data: " << "TRF[" << ad1 << "]= " << TRF_RdData1 << endl;
	}
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::writeRegTRB
(sc_lv <regFileAdrBit> WrADR, sc_lv <N> WrData)
{
	sc_uint <regFileAdrBit> wrAd;
	if (TRB_wrReg == '1')
	{
		wrAd = WrADR;
		TRB[wrAd] = WrData;
		cout << "TRB Write Data: " << "TRB[" << wrAd << "]= " << TRB[wrAd] << "	Time :   " << sc_time_stamp() << endl;
	}
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::readRegTRB
(sc_lv <regFileAdrBit> address1, sc_lv <regFileAdrBit> address2)
{
	sc_uint<regFileAdrBit> ad1, ad2;
	ad1 = address1;
	ad2 = address2;
	TRB_RdData1 = TRB[ad1];
	TRB_RdData2 = TRB[ad2];
	if (ad1 != ad2) {
		cout << "TRB read Data: " << "TRB[" << ad1 << "]= " << TRB_RdData1 << endl;
		cout << "TRB read Data: " << "TRB[" << ad2 << "]= " << TRB_RdData2 << endl;
	}
	else {
		cout << "TRB read Data: " << "TRB[" << ad1 << "]= " << TRB_RdData1 << endl;
	}
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
sc_lv<N> sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::nBitSignExtension
(sc_lv<N> inputSignal, int leftRange, bool sign)
{
	sc_lv <N> temp, signEximmOut;

	if (sign == true) {
		for (int i = 0; i <= (N - leftRange); i++)
		{
			temp[i] = inputSignal[leftRange];
		}
		signEximmOut = (temp.range(N - leftRange, 0), inputSignal.range(leftRange, 0));
	}
	else {
		for (int i = 0; i <= (N - leftRange); i++)
		{
			temp[i] = SC_LOGIC_0;
		}
		signEximmOut = (temp, inputSignal.range(leftRange, 0));
	}
	cout << "signEximm is:  " << signEximmOut << endl;
	return signEximmOut;
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
sc_lv<N> sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::shiftFunction
(sc_lv<5> shiftNumber, sc_lv<N> input, bool logical)
{
	sc_lv<N> result;
	switch (logical)
	{
	case true:
	{
		cout << "LOGICAL" << endl;
		cout << "ShiftNumber   " << shiftNumber[4] << endl;
		if (shiftNumber[4] == 1)
		{
			result = input.to_uint() << shiftNumber.range(3, 0).to_uint();
			cout << "LEFT SHIFT" << endl;
			cout << "input is  " << input.to_uint() << endl;
			cout << "Num is  " << shiftNumber.range(3, 0).to_uint() << endl;
			cout << "Result is" << result << endl;
		}
		else if (shiftNumber[4] == 0)
		{
			result = input.to_uint() >> shiftNumber.range(3, 0).to_uint();
			cout << "right SHIFT" << endl;

		}
		break;
	}
	case false:
	{
		cout << "False" << endl;
		if (shiftNumber[4] == 1)
		{
			result = input.to_uint() << shiftNumber.range(3, 0).to_uint();
		}
		else if (shiftNumber[4] == 0)
		{
			cout << "Positive" << endl;
			for (int i = N - 1; i >= N - (shiftNumber.range(3, 0).to_uint()); i--)
			{
				result[i] = input[N - 1];
				cout << "result[i]    " << result[i] << endl;
			}
			result.range(N - (shiftNumber.range(3, 0).to_uint()) - 1, 0) =
				input.range(N - 1, shiftNumber.range(3, 0).to_uint());
		}
	}
	default:
		break;
	}
	cout << "Result is   " << result << endl;
	return result;
}

template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction <N, regFileAdrBit, opcodeBit, cyclesMDU> ::abstractSimulation()
{
	sc_lv <2 * N> multRes;
	sc_lv <N> complement;
	sc_lv <N> tempWrite;
	sc_lv <2> IR2;
	sc_lv <2> IR15;
	sc_lv <4> opcode;
	sc_lv <4> rdOrrs1;
	sc_lv <4> rs1ADR;
	sc_lv <4> rs2ADR;
	sc_lv <N> tempPC;
	sc_lv <N> signEximm;
	sc_lv <8> imm;
	sc_lv <N> Quo, Rem;
	//sc_lv<8> flags;
	sc_lv<3> RFI;
	sc_logic IR8, IR9;
	PCregister = 0x0000;

	while (true)
	{
		//check for interrupts
		if (interrupt == SC_LOGIC_1)
		{
			//InterruptHandler();
			//continue;
		}

		writeMem = SC_LOGIC_0;
		cout << "MEM Ready before  is: " << memReady << endl;
		do
		{
			readMem = SC_LOGIC_1;
			addrBus = PCregister;
			cout << "PCregister   " << PCregister << endl;
			wait();

			cout << "PCregister   " << addrBus << endl;
		}while (memReady != '1');
		cout << "MEM Ready before IR is:" << memReady << endl;
		IRregister = dataBus->read();
		cout << "Ir IS:    " << IRregister << endl;

		opcode = IRregister.range(15, 12);
		IR2 = IRregister.range(11, 10);
		IR15 = IRregister.range(11, 10);
		imm = IRregister.range(11, 4);
		RFI = IRregister.range(6, 4);
		rdOrrs1 = IRregister.range(3, 0);
		rs1ADR = IRregister.range(7, 4);
		rs2ADR = IRregister.range(11, 8);
		IR8 = IRregister[8];
		IR9 = IRregister[9];
		readMem = SC_LOGIC_0;

		//	cout << "opcode.to_uint()    " << opcode.to_uint() << endl;
			//cout << "ir9    "  << IR9  << "    ir9 char    " << IR9.to_char() << endl;
		addrBus.write('Z');// = "ZZZZZZZZZZZZZZZZ";

		switch (opcode.to_uint())
		{
		case ORSV0:
		{
			PCregister = PCregister.to_uint() + 1;
			wait();
			break;
		}
		case ORSV1:
		{
			PCregister = PCregister.to_uint() + 1;
			wait();
			break;
		}
		case OIns2:
		{
			switch (IR2.to_uint())
			{
			case LDR:
			{
				cout << "**************Ldr Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rs1ADR);
				ADRregister = TRF_RdData1;
				wait();
				if (IRregister[9] == '0')
					do
					{
						readMem = SC_LOGIC_1;
						addrBus = ADRregister;
						wait();
					}while (memReady != '1');
				else if (IRregister[9] == '1')
				{
					readIO = SC_LOGIC_1;
					addrBus = ADRregister;
					wait();
				}
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, dataBus);
				readMem = SC_LOGIC_0;
				readIO = SC_LOGIC_0;
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(0x2);
				wait();
				break;
			}
			case STR:
			{
				cout << "**************STR Instruction************** " << endl;
				readRegTRF(rdOrrs1, rs1ADR);
				ADRregister = TRF_RdData1;
				wait();
				//	addrBus = ADRregister;
				if (IRregister[9] == '0')
				{
					do
					{
						cout << "MemReady != 1" << endl;
						writeMem = SC_LOGIC_1;
						addrBus = ADRregister;
						dataBusOut = TRF_RdData2;
						wait();
					}
					while (memReady != '1');
					cout << "memReady STR is" << memReady << endl;
				}
				else if (IRregister[9] == '1')
				{
					writeIO = SC_LOGIC_1;
					addrBus = ADRregister;
					dataBusOut = TRF_RdData2;
					wait();
				}
				writeMem = SC_LOGIC_0;
				writeIO = SC_LOGIC_0;
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(0x03);
				wait();
				break;
			}
			case JMR_JMB:
			{
				bool ir8_bool = (IR8 == '1');
				switch (ir8_bool)
				{
				case false: //JMR
					cout << "**************JMR Instruction************** " << endl;
					TRF_wrReg = SC_LOGIC_0;
					readRegTRF(rs1ADR, rs1ADR);
					tempPC = PCregister.to_uint() + TRF_RdData1.to_uint();
					TRF_wrReg = SC_LOGIC_1;
					if (IRregister[9] == '1')
					{
						tempWrite = PCregister.to_uint() + 1;
						writeRegTRF(rdOrrs1, tempWrite);
					}
					PCregister = tempPC;
					//(*powModule).display(0x4);
					wait();
					break;
				case true: //JMB
					cout << "**************JMB Instruction************** " << endl;
					TRB_wrReg = SC_LOGIC_0;
					readRegTRB(rdOrrs1, rdOrrs1);
					tempPC = rs1ADR.to_uint() + TRB_RdData1.to_uint();
					TRB_wrReg = SC_LOGIC_1;
					if (IRregister[9] == '1')
					{
						tempWrite = TRB_RdData1.to_uint();
						writeRegTRF(15, tempWrite);
					}
					PCregister = tempPC;
					//(*powModule).display(0x4);
					wait();
					cout << "PC register is : " << PCregister << endl;
					break;
				}
			}
			case JMI:
			{
				cout << "**************JMI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				signEximm = nBitSignExtension(imm, 5, true);
				tempPC = PCregister.to_uint() + signEximm.to_int();
				tempWrite = PCregister.to_uint() + 1;
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = tempPC;
				//(*powModule).display(0x5);
			
				wait();
				break;
			}
			default:
				break;
			}
			break;
		}
		case OIANR:
		{
			cout << "**************ANR Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rs1ADR, rs2ADR);
			tempWrite = TRF_RdData1 & TRF_RdData2;
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, tempWrite);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(ANR);
			wait();
			break;
		}
		case OIANI:
		{
			if (flagRegister[15] != '1')
			{
				cout << "**************ANI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rdOrrs1, rdOrrs1);
				signEximm = nBitSignExtension(imm, 7, false);
				tempWrite = TRF_RdData1 & signEximm;
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(ANI);
				wait();
			}
			else if (flagRegister[15] == '1')
			{
				cout << "**************Shadow ANI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rdOrrs1);
				signEximm = nBitSignExtension(imm, 3, false);
				tempWrite = TRF_RdData1 & signEximm;
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(ANI);
				wait();

			}
			break;
		}
		case OIMSI:
		{
			cout << "**************MSI Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			signEximm = nBitSignExtension(imm, 7, true);
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, signEximm);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(MSI);
			wait();
			break;
		}
		case OIMHI:
		{
			cout << "**************MHI Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rdOrrs1, rdOrrs1);
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, (imm, TRF_RdData1.range(7, 0)));
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(MHI);
			wait();
			break;
		}
		case OISLR:
		{
			cout << "**************SLR Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rs1ADR, rs2ADR);
			tempWrite = shiftFunction(TRF_RdData2.range(4, 0), TRF_RdData1, true);
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, tempWrite);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(SLR);
			wait();
			break;
		}
		case OISAR:
		{
			cout << "**************SAR Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rs1ADR, rs2ADR);
			tempWrite = shiftFunction(TRF_RdData2.range(4, 0), TRF_RdData1, false);
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, tempWrite);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(SAR);
			wait();
			break;
		}
		case OIADR:
		{
			cout << "**************ADR Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rs1ADR, rs2ADR);
			tempWrite = TRF_RdData1.to_uint() + TRF_RdData2.to_uint();
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, tempWrite);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(ADR);
			wait();
			break;
		}
		case OISUR:
		{
			cout << "**************SUR Instruction************** " << endl;
			TRF_wrReg = SC_LOGIC_0;
			readRegTRF(rs1ADR, rs2ADR);
			tempWrite = TRF_RdData1.to_uint() - TRF_RdData2.to_uint();
			TRF_wrReg = SC_LOGIC_1;
			writeRegTRF(rdOrrs1, tempWrite);
			PCregister = PCregister.to_uint() + 1;
			//(*powModule).display(SUR);
			wait();
			break;
		}
		case OIADI:
		{
			if (flagRegister[15] != '1')
			{
				cout << "**************ADI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				//cout << "Time :   " << sc_time_stamp()<<endl;
				readRegTRF(rdOrrs1, rdOrrs1);
				signEximm = nBitSignExtension(imm, 7, true);
				tempWrite = TRF_RdData1.to_uint() + signEximm.to_int();
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(ADI);
				wait();
			}
			else if (flagRegister[15] == '1')
			{
				cout << "**************Shadow ADI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				//	cout << "Time :   " << sc_time_stamp()<<endl;
				readRegTRF(rs1ADR, rdOrrs1);
				signEximm = nBitSignExtension(imm, 3, true);
				tempWrite = TRF_RdData1.to_uint() + signEximm.to_int();
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(ADI);
				wait();
			}
			break;
		}
		case OISUI:
		{
			if (flagRegister[15] != '1')
			{
				cout << "**************SUI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				//cout << "Time :   " << sc_time_stamp()<<endl;
				readRegTRF(rdOrrs1, rdOrrs1);
				signEximm = nBitSignExtension(imm, 7, true);
				tempWrite = TRF_RdData1.to_uint() - signEximm.to_int();
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(SUI);
				wait();
			}
			else if (flagRegister[15] == '1')
			{
				cout << "**************Shadow SUI Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				//cout << "Time :   " << sc_time_stamp()<<endl;
				readRegTRF(rs1ADR, rdOrrs1);
				signEximm = nBitSignExtension(imm, 3, true);
				tempWrite = TRF_RdData1.to_uint() - signEximm.to_int();
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(SUI);
				wait();
			}
			break;
		}
		case OIMUL:
		{
			if (flagRegister[13] != '1')
			{
				cout << "**************MUL Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rs2ADR);
				multRes = TRF_RdData1.to_uint() * TRF_RdData2.to_uint();
				for (int i = 0; i < cyclesMDU; i++)
					wait();
				tempWrite = multRes.range(N - 1, 0);
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				tempWrite = multRes.range(2 * N - 1, N);
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF((rdOrrs1.to_uint() + 1), tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(MUL);
				wait();
			}
			else if (flagRegister[13] == '1')
			{
				cout << "**************Shadow MUL Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rs2ADR);
				multRes = TRF_RdData1.to_uint() * TRF_RdData2.to_uint();
				for (int i = 0; i < cyclesMDU; i++)
					wait();
				tempWrite = multRes.range(N - 1, 0);
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(MUL);
				wait();
			}
			break;
		}
		case OIDIV:
		{
			if (flagRegister[11] != '1')
			{
				cout << "*************DIV Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rs2ADR);
				Quo = TRF_RdData1.to_uint() / TRF_RdData2.to_uint();
				for (int i = 0; i < cyclesMDU; i++)
					wait();
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, Quo);
				Rem = TRF_RdData1.to_uint() % TRF_RdData2.to_uint();;
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF((rdOrrs1.to_uint() + 1), Rem);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(DIV);
				wait();
			}
			else if (flagRegister[11] == '1')
			{
				cout << "*************Shadow DIV Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rs1ADR, rs2ADR);
				Quo = TRF_RdData1.to_uint() / TRF_RdData2.to_uint();
				for (int i = 0; i < cyclesMDU; i++)
					wait();
				//tempWrite = multRes.range(N-1,0);
				TRF_wrReg = SC_LOGIC_1;
				writeRegTRF(rdOrrs1, Quo);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(DIV);
				wait();
			}
			break;
		}
		case OIns15:
		{ 
			cout << "=-=-=-=-=-=-=-=-=OIns15=-=-=-=-=-=-=-=" << endl;
			cout << "IR15 is:   " << IR15.to_uint() << endl;
			switch (IR15.to_uint())
			{
			case COMP:
			{
				switch (IR9.to_bool())
				{
				case CMR:
				{
					cout << "*************CMR Instruction************** " << endl;
					readRegTRF(rs1ADR, rdOrrs1);
					if (TRF_RdData1.to_uint() > TRF_RdData2.to_uint())
						flagRegister[5] = SC_LOGIC_1;
					else
						flagRegister[5] = SC_LOGIC_0;
					flagRegister[4] = (TRF_RdData1 == TRF_RdData2) ? sc_logic_1 : sc_logic_0;
					cout << "flags is:   " << flagRegister << endl;
					PCregister = PCregister.to_uint() + 1;
					//(*powModule).display(0x18);
					wait();
					break;
				}
				case CMI:
				{
					cout << "*************CMI Instruction************** " << endl;
					readRegTRF(rdOrrs1, rdOrrs1);
					signEximm = nBitSignExtension(imm, 4, true);
					if (TRF_RdData1.to_uint() > signEximm.to_uint())
						flagRegister[5] = SC_LOGIC_1;
					else
						flagRegister[5] = SC_LOGIC_0;
					flagRegister[4] = (TRF_RdData1 == signEximm) ? sc_logic_1 : sc_logic_0;
					cout << "flags is:   " << flagRegister << endl;
					PCregister = PCregister.to_uint() + 1;
					//(*powModule).display(0x19);
					wait();
					break;
				}
				default:
					break;
				}
				break;
			}
			case BRANCH:
			{
				//cout << "Bool is   " <<  IR9.to_bool() << endl;
				switch (IR9.to_bool())
				{
				case BRC:
				{
					cout << "*************BRC Instruction************** " << endl;
					readRegTRF(rdOrrs1, rdOrrs1);
					switch (RFI.to_uint())
					{
					case 0:
					{
						cout << "Case 0" << endl;
						if (flagRegister[4] == 1)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					case 1:
					{
						cout << "Case 1" << endl;
						if (flagRegister[5] == 0)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					case 2:
					{
						cout << "Case 2" << endl;
						if (flagRegister[5] == 1)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					case 3:
					{
						cout << "Case 3" << endl;
						if (flagRegister[4] == 1 || flagRegister[5] == 1)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					case 4:
					{
						cout << "Case 4" << endl;
						if (flagRegister[4] == 1 || flagRegister[5] == 0)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					case 5:
					{
						cout << "Case 5" << endl;
						if (flagRegister[4] == 0)
							PCregister = TRF_RdData1;
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x20);
						wait();
						break;
					}
					default:
						break;
					}
					break;
				}
				case BRR:
				{
					cout << "*************BRR Instruction************** " << endl;
					readRegTRF(rdOrrs1, rdOrrs1);
					switch (RFI.to_uint())
					{
					case 0:
					{
						cout << "Case 0 " << endl;
						if (flagRegister[4] == 1)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					case 1:
					{
						cout << "Case 1" << endl;
						if (flagRegister[5] == 0)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					case 2:
					{
						cout << "Case 2" << endl;
						if (flagRegister[5] == 1)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					case 3:
					{
						cout << "Case 3" << endl;
						if (flagRegister[4] == 1 || flagRegister[5] == 1)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					case 4:
					{
						cout << "Case 4" << endl;
						if (flagRegister[4] == 1 || flagRegister[5] == 0)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					case 5:
					{
						cout << "Case 5" << endl;
						if (flagRegister[4] == 0)
							PCregister = PCregister.to_uint() + TRF_RdData1.to_uint();
						else
							PCregister = PCregister.to_uint() + 1;
						//(*powModule).display(0x21);
						wait();
						break;
					}
					default:
						break;
					}
					break;
				}
				default:
					break;
				}
				break;
			}
			case SHI:
			{
				//if (IR9.to_bool()== false)
				//{
				cout << "*************SHLogical Instruction************** " << endl;
				TRF_wrReg = SC_LOGIC_0;
				readRegTRF(rdOrrs1, rdOrrs1);
				if (IR9.to_bool() == false)
					tempWrite = shiftFunction(IRregister.range(8, 4), TRF_RdData1, true);
				else if (IR9.to_bool() == true)
					tempWrite = shiftFunction(IRregister.range(8, 4), TRF_RdData1, false);
				TRF_wrReg = SC_LOGIC_1;
				cout << "Temp write   " << tempWrite << endl;
				writeRegTRF(rdOrrs1, tempWrite);
				PCregister = PCregister.to_uint() + 1;
				//(*powModule).display(0x22);
				wait();
				break;
			}
			case NOT:
			{
				cout << "=-=-=-=-=-=-=NOT=-=-=-=-=-=" << endl;
				switch (IR9.to_bool())
				{
				case NTR:
				{
					cout << "*************NTR Instruction************** " << endl;
					TRF_wrReg = SC_LOGIC_0;
					readRegTRF(rs2ADR, rs2ADR);
					complement = ~(TRF_RdData2);
					tempWrite = (IRregister[8] == '1') ? (complement.to_uint() + 1) : complement;
					TRF_wrReg = SC_LOGIC_1;
					writeRegTRF(rdOrrs1, tempWrite);
					PCregister = PCregister.to_uint() + 1;
					//(*powModule).display(0x23);
					wait();
					break;
				}
				case NTD:
				{
					cout << "*************NTD Instruction************** " << endl;
					TRF_wrReg = SC_LOGIC_0;
					readRegTRF(rdOrrs1, rdOrrs1);
					complement = ~(TRF_RdData1);
					tempWrite = (IRregister[8] == '1') ? complement.to_uint() + 1 : complement;
					TRF_wrReg = SC_LOGIC_1;
					writeRegTRF(rdOrrs1, tempWrite);
					PCregister = PCregister.to_uint() + 1;
					//(*powModule).display(0x24);
					wait();
					break;
				}
				default:
					break;
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

	}
}




template <int N, int regFileAdrBit, int opcodeBit, int cyclesMDU>
void sayacInstruction<N, regFileAdrBit, opcodeBit, cyclesMDU>::InterruptHandler()
{
	//check if interrupts are enabled
	sc_lv<regFileAdrBit> r15_addr = 15;
	readRegTRF(r15_addr, r15_addr);
	sc_lv<N> r15_value = TRF_RdData1;
	if (r15_value[3] != SC_LOGIC_1 || InterruptHandlerActive != SC_LOGIC_0) {
		return;
	}

	cout << "interrupt handler:-----------interrupt handler is called" << "		Time :   " << sc_time_stamp() << endl;
	//Save R15 in IHBA
	sc_lv<regFileAdrBit> IHBA = 5;
	TRB_wrReg = SC_LOGIC_1;
	writeRegTRB(IHBA, r15_value);
	INTA_bar = SC_LOGIC_0;
	wait();

	TRB_wrReg = SC_LOGIC_1;
	cout << "interrupt handler:------------contents of R15 is stored into IHBA" << "	Time :   " << sc_time_stamp() << endl;

	//change R15 in TRF
	InterruptHandlerActive = SC_LOGIC_1;
	cout << "interrupt handler:-----------Set interrupt servicing bit in R15" << "	Time :   " << sc_time_stamp() << endl;
	sc_lv<N> new_R15 = TRF_RdData1;
	new_R15[3] = SC_LOGIC_0;  //Clear interrupt enable bit
	new_R15[7] = SC_LOGIC_1;  //Set interrupt servicing 
	writeRegTRF(r15_addr, new_R15);

	//save PC in IHBA+1
	TRB_wrReg = SC_LOGIC_1;
	writeRegTRB(IHBA.to_uint() + 1, PCregister);

	INTA_bar = SC_LOGIC_1;
	wait();
	TRB_wrReg = SC_LOGIC_0;
	cout << "interrupt handler:------------PC is stored into IHBA+1" << "		Time :   " << sc_time_stamp() << endl;


	//PC<=IHBA+2
	TRB_rdReg = SC_LOGIC_1;
	///////readRegTRB(IHBA.to_uint() + 2, IHBA.to_uint() + 2);
	sc_lv<N> ISR_ADDR;
	ISR_ADDR.range(7, 0) = dataBusOut.read();      // lower 8 bits
	ISR_ADDR.range(N - 1, 8) = "00000000";         // upper 8 bits
	PCregister = ISR_ADDR;

	wait();
	/////////////////////reset control bits:
	TRB_rdReg = SC_LOGIC_0;
	InterruptHandlerActive = SC_LOGIC_0;
	new_R15[3] = SC_LOGIC_1;  //set interrupt enable bit
	writeRegTRF(r15_addr, new_R15);
	///////////////////////////////////////
	cout << "interrupt handler:------------PC iS set to the location of ISR" << "	Time :   " << sc_time_stamp() << endl;
	cout << "Jumped to ISR at address: " << PCregister << endl;
	cout << "interrupt handler:------------end of interrupt handling" << "		Time :   " << sc_time_stamp() << endl;
}