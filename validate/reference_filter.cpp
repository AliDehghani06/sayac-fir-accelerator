#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

/*
g++ reference_filter.cpp -o reference_filter

./reference_filter

diff filtered.txt filtered_reference.txt
diff filtered_2x.txt filtered_2x_reference.txt
diff filtered_half.txt filtered_half_reference.txt
*/

static int16_t sat16(int64_t v)
{
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    return (int16_t)v;
}

static vector<int16_t> readInts(const string &path)
{
    vector<int16_t> out;
    ifstream in(path);
    if (!in.is_open())
    {
        cerr << "[ERROR] cannot open " << path << endl;
        exit(1);
    }
    int v;
    while (in >> v)
        out.push_back((int16_t)v);
    return out;
}

static void writeInts(const string &path, const vector<int16_t> &data)
{
    ofstream out(path);
    for (int16_t v : data)
        out << v << "\n";
}

int main()
{
    vector<int16_t> x = readInts("noisy.txt");
    vector<int16_t> h = readInts("coefficients.txt");

    const int N = (int)x.size();
    const int TAPS = (int)h.size();
    cout << "Loaded " << N << " input samples, " << TAPS << " coefficients." << endl;

    // Part 1: FIR filter, direct formula
    vector<int16_t> y(N);
    for (int n = 0; n < N; n++)
    {
        int64_t acc = 0;
        for (int k = 0; k < TAPS; k++)
        {
            if (n >= k)
                acc += (int32_t)x[n - k] * (int32_t)h[k];
        }
        acc >>= 15;
        y[n] = sat16(acc);
    }
    writeInts("filtered_reference.txt", y);
    cout << "Wrote filtered_reference.txt (" << N << " samples)" << endl;

    // Part 2: x2
    int N2 = N / 2;
    vector<int16_t> y2x(N2);
    for (int i = 0; i < N2; i++)
        y2x[i] = y[2 * i];
    writeInts("filtered_2x_reference.txt", y2x);
    cout << "Wrote filtered_2x_reference.txt (" << N2 << " samples)" << endl;

    // Part 3: x0.5
    int Nh = N * 2;
    vector<int16_t> yhalf(Nh);
    for (int i = 0; i < N; i++)
    {
        int16_t xi = y[i];
        int16_t xnext = (i + 1 < N) ? y[i + 1] : xi;
        int16_t mean = (int16_t)(((int32_t)xi + (int32_t)xnext) / 2);
        yhalf[2 * i] = xi;
        yhalf[2 * i + 1] = mean;
    }
    writeInts("filtered_half_reference.txt", yhalf);
    cout << "Wrote filtered_half_reference.txt (" << Nh << " samples)" << endl;

    cout << "Done." << endl;
    return 0;
}
