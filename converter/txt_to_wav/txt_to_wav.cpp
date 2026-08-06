#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;


struct WAVHeader {

    char riff[4] = {'R','I','F','F'};
    uint32_t size;

    char wave[4] = {'W','A','V','E'};

    char fmt[4] = {'f','m','t',' '};
    uint32_t fmtSize = 16;

    uint16_t format = 1;
    uint16_t channels = 1;

    uint32_t sampleRate = 44100;
    uint32_t byteRate;

    uint16_t blockAlign;
    uint16_t bits = 16;

    char data[4] = {'d','a','t','a'};
    uint32_t dataSize;

};



int main()
{

    ifstream input("filtered.txt");
    
    ofstream output("filtered.wav", ios::binary);

    vector<int16_t> samples;

    int value;

    while(input >> value){
        samples.push_back((int16_t)value);
    }

    input.close();

    WAVHeader header;


    header.dataSize = samples.size()*sizeof(int16_t);


    header.size = 36 + header.dataSize;

    header.blockAlign = header.channels * header.bits / 8;

    header.byteRate = header.sampleRate * header.blockAlign;

    output.write((char*)&header, sizeof(header));

    output.write((char*)samples.data(), header.dataSize);

    output.close();

    cout<<"Created filtered.wav\n";


    return 0;
}