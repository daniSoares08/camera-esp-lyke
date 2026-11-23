#include <cassert>
#include <string>
#include "../../esp32cam_integrada/lyke/answer_pager.h"

int main()
{
    char rows[6][22];
    std::string input = "\nDADOS\n\nR=220 ohm\nL=.001 H\n  \nf=35000 Hz\n\n"
                        "XL=2*pi*f*L\nXL=219.911 ohm\n\nZ=220+j*219.911\n"
                        "\n\nTokens entrada: 9179\nTokens saida: 2616";
    assert(answer_page_rows(input.c_str(), input.size(), 0, 21, 6, true, rows));
    const char *expected[] = {"DADOS", "R=220 ohm", "L=.001 H", "f=35000 Hz", "XL=2*pi*f*L", "XL=219.911 ohm"};
    for (int i=0; i<6; ++i) assert(std::string(rows[i]) == expected[i]);
    assert(!answer_page_rows(input.c_str(), input.size(), 1, 21, 6, true, rows));
    assert(std::string(rows[0]) == "Z=220+j*219.911");
    for (int i=1; i<6; ++i) assert(!rows[i][0]);
    assert(answer_page_rows(input.c_str(), input.size(), 0, 21, 4, false, rows));
    assert(!rows[0][0] && std::string(rows[1]) == "DADOS" && !rows[2][0]);
    input = "1234567890123456789012345678901234567890123456789012345678901234567\n\n";
    assert(!answer_page_rows(input.c_str(), input.size(), 0, 21, 6, true, rows));
    std::string rebuilt;
    for (auto &row: rows) { assert(strlen(row)<=21); rebuilt += row; }
    assert(rebuilt == input.substr(0,67));
    input = "1\n2\n3\n4\n5\n6\n\n\n";
    assert(!answer_page_rows(input.c_str(), input.size(), 0, 21, 6, true, rows));
    return 0;
}
