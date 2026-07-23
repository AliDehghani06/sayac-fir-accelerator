#include <iostream>
#include <fstream>
#include <cstdint>

using namespace std;

struct WAVHeader {

    char riff[4];
    uint32_t size;
    char wave[4];

    char fmt[4];
    uint32_t fmtSize;

    uint16_t format;
    uint16_t channels;

    uint32_t sampleRate;
    uint32_t byteRate;

    uint16_t blockAlign;
    uint16_t bits;

    char data[4];
    uint32_t dataSize;
};



int main()
{

    ifstream input("noisy.wav", ios::binary);


    ofstream output("noisy.txt");

    WAVHeader header;


    input.read((char*)&header, sizeof(header));

    int samples_per_channel = header.sampleRate / 2;

    int total_samples       = header.dataSize / 2;

    int total_frames        = total_samples / header.channels;

    if(samples_per_channel > total_frames)
        samples_per_channel = total_frames;


    // save left , drop right
    for(int i=0; i<samples_per_channel; i++){
        int16_t left;

        input.read((char*)&left, sizeof(int16_t));

        output << left << "\n";

        for(int ch=1; ch<header.channels; ch++)
        {
            int16_t dummy;
            input.read((char*)&dummy, sizeof(int16_t));
        }

    }

    input.close();
    output.close();

    cout<<"Saved "<<samples_per_channel<<" samples to noisy.txt\n";

    return 0;
}