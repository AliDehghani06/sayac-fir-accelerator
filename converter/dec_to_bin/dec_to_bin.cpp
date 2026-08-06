#include <fstream>
#include <iostream>
#include <cstdint>
#include <bitset>
#include <string>

// dec_to_bin <input_decimal.txt> <output_binary.txt>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: " << argv[0] << " <input_decimal.txt> <output_binary.txt>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in.is_open())
    {
        std::cerr << "[ERROR] cannot open input file: " << argv[1] << std::endl;
        return 1;
    }

    std::ofstream out(argv[2]);
    if (!out.is_open())
    {
        std::cerr << "[ERROR] cannot open output file: " << argv[2] << std::endl;
        return 1;
    }

    long value;
    int count = 0;
    while (in >> value)
    {
        int16_t v16 = (int16_t)value;   // truncate/clamp to 16-bit signed range
        uint16_t bits = (uint16_t)v16;  // reinterpret as the two's-complement bit pattern
        out << std::bitset<16>(bits).to_string() << "\n";
        count++;
    }

    std::cout << "converted " << count << " values from " << argv[1]
               << " to " << argv[2] << std::endl;
    return 0;
}
