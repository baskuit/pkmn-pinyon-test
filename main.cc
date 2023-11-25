#include "./src/battle.hh"

#include <fstream>

void rollout_write(
    BattleTypes::State &state,
    int n_frames,
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt")
{
    remove(path.data());
    BattleTypes::PRNG device{0};
    std::fstream file;
    file.open(path, std::ios::binary | std::ios::app);

    // header
    file << uint8_t{1} << uint8_t{1} << uint8_t{64} << uint8_t{0};
    file.write(reinterpret_cast<char *>(state.battle.bytes), 384);

    for (int frame = 0; frame < n_frames && !state.is_terminal(); ++frame)
    {
        const int row_idx = device.random_int(state.row_actions.size());
        const int col_idx = device.random_int(state.col_actions.size());
        // zero out log data. Is this a good idea?
        memset(state.log.data(), 0, 64);
        state.apply_actions(
            state.row_actions[row_idx],
            state.col_actions[col_idx]);

        pkmn_result result = state.result;
        pkmn_choice row_action = state.row_actions[row_idx].get();
        pkmn_choice col_action = state.col_actions[col_idx].get();
        char *result_ = reinterpret_cast<char *>(&result);
        char *row_action_ = reinterpret_cast<char *>(&row_action);
        char *col_action_ = reinterpret_cast<char *>(&col_action);

        const size_t len = 64;
        file.write(reinterpret_cast<char *>(state.log.data()), len);
        // char x[1] = {0};
        // file.write(x, 1);
        file.write(reinterpret_cast<char *>(state.battle.bytes), 384);
        file.write(result_, 1);
        file.write(row_action_, 1);
        file.write(col_action_, 1);

        // after to not overwrite .row_actions, col_actions
        state.get_actions();
        ++frame;
    }
    file.close();
}

void read()
{
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt";
    std::ifstream file(path, std::ios::binary);
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    using T = uint8_t;

    int i = 0;
    std::cout << "showdown/gen/log: " << std::endl;
    {
        int x = static_cast<T>(buffer[i++]);

        std::cout << x << ' ';
    }
    {
        int x = static_cast<T>(buffer[i++]);

        std::cout << x << ' ';
    }
    {
        int x = static_cast<T>(buffer[i++]);

        std::cout << x << ' ';
    }
    {
        int x = static_cast<T>(buffer[i++]);

        std::cout << x << ' ';
    }
    std::cout << std::endl;

    std::cout << "Battle: " << std::endl;
    for (int j = 0; j < 384; ++j)
    {
        int x = static_cast<T>(buffer[i++]);
        std::cout << x << ' ';
    }
    std::cout << std::endl;

    while (i < buffer.size())
    {
        std::cout << "Log: " << std::endl;
        for (int j = 0; j < 64; ++j)
        {
            int x = static_cast<T>(buffer[i++]);

            std::cout << x << ' ';
        }
        std::cout << std::endl;

        std::cout << "Battle: " << std::endl;
        for (int j = 0; j < 384; ++j)
        {
            int x = static_cast<T>(buffer[i++]);

            std::cout << x << ' ';
        }
        std::cout << std::endl;

        std::cout << "result/choices: " << std::endl;
        {
            int x = static_cast<T>(buffer[i++]);

            std::cout << x << ' ';
        }
        {
            int x = static_cast<T>(buffer[i++]);

            std::cout << x << ' ';
        }
        {
            int x = static_cast<T>(buffer[i++]);

            std::cout << x << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << "final i: " << i << " vs " << buffer.size() << std::endl;
}

int main(int argc, char **argv)
{
    int number = std::atoi(argv[1]);
    BattleTypes::State state{};
    rollout_write(state, number);
    read();
}
