#pragma once

#include <string>
#include <vector>
#include <iostream>

void print_durations(const uint8_t *data, int *output)
{
    output[0] = ((data[0] >> 0) & 7);
    output[1] = ((data[0] >> 3) & 7);
    output[2] = (int)((data[0] >> 6) & 3) + (int)((data[1] >> 0) & 3) * 4;
    output[3] = ((data[1] >> 2) & 7);
    output[4] = ((data[1] >> 5) & 7);
}

void get_chance_actions_output(
    const uint8_t *data, int *output)
{
    output[0] = ((data[0] >> 0));
    output[1] = ((data[1] >> 0) & 3);
    output[2] = ((data[1] >> 2) & 3);
    output[3] = ((data[1] >> 4) & 3);
    output[4] = ((data[1] >> 6) & 3);
    output[5] = ((data[2] >> 0) & 3);
    output[6] = ((data[2] >> 2) & 3);
    output[7] = ((data[2] >> 4) & 15);
    print_durations(data + 3, output + 8);
    output[13] = ((data[5] >> 0) & 15);
    output[14] = ((data[5] >> 4) & 15);
    output[15] = ((data[6] >> 0));
    output[16] = ((data[7] >> 0));
}

std::string format(const int x)
{

    const static std::string out[3] = {"-", "NO", "YES"};
    return out[x];
}

void print_chance_actions(const uint8_t *data)
{
    const static std::vector<std::string> fields = {
        "dmg", "hit", "crit", "proc",
        "tie", "slfh", "para", "dura",
        "slp d.", "cnf d.", "dis d.", "atk d.",
        "trp d.", "slot", "mult", "psyw", "metr"};

    const static bool type[17] = {
        0, 1, 1, 1,
        0, 1, 1, 0,
        0, 0, 1, 0,
        0, 0, 0, 0, 0};

    int output[17];

    for (int offset = 0; offset <= 8; offset += 8)
    {
        get_chance_actions_output(data + offset, output);
        for (int i = 0; i < 17; ++i)
        {
            std::cout << fields[i] << ": ";
            if (type[i])
            {
                std::cout << format(output[i]) << ", ";
            }
            else
            {
                std::cout << output[i] << ", ";
            }
        }
        std::cout << std::endl;
    }
}

void print_overrides (const uint8_t *data) {
    print_chance_actions(data);
    int duration_ouput[5];
    print_durations(data + 16, duration_ouput);
    for (int i = 0; i < 5; ++i) {
        std::cout << duration_ouput[i] << ", ";
    }
    std::cout << std::endl;
    print_durations(data + 18, duration_ouput);
    for (int i = 0; i < 5; ++i) {
        std::cout << duration_ouput[i] << ", ";
    }
    std::cout << std::endl;
}