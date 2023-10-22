#include "./src/battle.hh"

#include <fstream>

void test1()
{
    std::vector<uint8_t> s0{}, s1{};

    BattleTypes::PRNG device{};
    BattleTypes::State state{};
    state.randomize_transition(device);
    BattleTypes::State state_{state};

    uint64_t d_seed = device.uniform_64();
    BattleTypes::PRNG d0{d_seed}, d1{d_seed};
    size_t turn = 0;
    while (!state.is_terminal())
    {
        {
            const size_t rows = state.row_actions.size();
            const size_t cols = state.col_actions.size();
            const int row_idx = d0.random_int(rows);
            const int col_idx = d0.random_int(cols);
            state.apply_actions(
                state.row_actions[row_idx],
                state.col_actions[col_idx],
                s0);
            state.get_actions();
        };
        {
            const size_t rows = state_.row_actions.size();
            const size_t cols = state_.col_actions.size();
            const int row_idx = d1.random_int(rows);
            const int col_idx = d1.random_int(cols);
            state_.apply_actions(
                state_.row_actions[row_idx],
                state_.col_actions[col_idx],
                s1);
            state_.get_actions();
        };

        if (memcmp(state.battle.bytes, state_.battle.bytes, 1))
        {
            std::cout << "battle bytes mismatch: " << turn << std::endl;
        }
        ++turn;
    }

    int m = s0.size();
    int n = s1.size();
    int z = std::min(m, n);
    std::cout << m << ' ' << n << ' ' << z << std::endl;
    for (int i = 0; i < z; ++i)
    {
        if (s0[i] != s1[i])
        {
            std::cout << i << ' ' << i % 8 << ", "
                      << (int)s0[i] << '/' << (int)s1[i] << std::endl;
        }
    }

    volatile uint32_t endian = 0x01234567;
    uint16_t turns0 = (*((uint8_t *)(&endian))) == 0x67
                          ? state.battle.bytes[368] | state.battle.bytes[369] << 8
                          : state.battle.bytes[368] << 8 | state.battle.bytes[369];
    uint16_t turns1 = (*((uint8_t *)(&endian))) == 0x67
                          ? state_.battle.bytes[368] | state_.battle.bytes[369] << 8
                          : state_.battle.bytes[368] << 8 | state_.battle.bytes[369];
    std::cout << "turns: " << (int)turns0 << ' ' << (int)turns1 << std::endl;
}

void move(BattleTypes::State &&state)
{
    state.apply_actions({0}, {0});
}

void rollout_pipe(
    BattleTypes::State &state,
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt")
{
    remove(path.data());
    BattleTypes::PRNG device{0};
    std::fstream file;
    file.open(path, std::ios::binary | std::ios::app);
    // header
    file << uint8_t{1} << uint8_t{1};
    file.write(reinterpret_cast<char *>(state.battle.bytes), 384);
    int i = 0;
    while (!state.is_terminal() && i < 28)
    {
        const int row_idx = device.random_int(state.row_actions.size());
        const int col_idx = device.random_int(state.col_actions.size());
        memset(state.log.data(), 0, 64);
        state.apply_actions(
            state.row_actions[row_idx],
            state.col_actions[col_idx]);
        // + 1 ensures 0x00 is written
        const size_t len = strlen(reinterpret_cast<char *>(state.log.data()));
        std::cout << "len : " << len << std::endl;

        pkmn_result result = state.result;
        pkmn_choice row_action = state.row_actions[row_idx].get();
        pkmn_choice col_action = state.col_actions[col_idx].get();
        char *result_ = reinterpret_cast<char *>(&result);
        char *row_action_ = reinterpret_cast<char *>(&row_action);
        char *col_action_ = reinterpret_cast<char *>(&col_action);

        file.write(reinterpret_cast<char *>(state.log.data()), len);
        char x[1] = {0};
        file.write(x, 1);
        file.write(reinterpret_cast<char *>(state.battle.bytes), 384);
        file.write(result_, 1);
        file.write(row_action_, 1);
        file.write(col_action_, 1);

        // after to not overwrite .row_actions, col_actions
        state.get_actions();
        ++i;
    }
    file.close();
}

void foo()
{
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt";
    std::ifstream file(path, std::ios::binary);
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    int i = 0;
    std::cout << "showdown/gen: " << std::endl;
    {
        int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
        std::cout << x << ' ';
    }
    {
        int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
        std::cout << x << ' ';
    }
    std::cout << std::endl;

    std::cout << "Battle: " << std::endl;
    for (int j = 0; j < 384; ++j)
    {
        int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
        std::cout << x << ' ';
    }
    std::cout << std::endl;

    while (i < buffer.size())
    {
        std::cout << "Log: " << std::endl;
        for (;;)
        {
            int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
            std::cout << x << ' ';
            if (x == 0)
            {
                break;
            }
        }
        std::cout << std::endl;

        std::cout << "Battle: " << std::endl;
        for (int j = 0; j < 384; ++j)
        {
            int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
            std::cout << x << ' ';
        }
        std::cout << std::endl;

        std::cout << "result/choices: " << std::endl;
        {
            int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
            std::cout << x << ' ';
        }
        {
            int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
            std::cout << x << ' ';
        }
        {
            int x = static_cast<int>(static_cast<unsigned char>(buffer[i++]));
            std::cout << x << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << "final i: " << i << " vs " << buffer.size() << std::endl;
}

void test2()
{
    std::vector<uint8_t> s0{}, s1{};

    BattleTypes::PRNG device{};
    BattleTypes::State state{};
    state.randomize_transition(device);
    BattleTypes::State state_{state};

    // rollout_print(state, s0);
    // rollout_print(state_, s1);

    int m = s0.size();
    int n = s1.size();
    int z = std::min(m, n);
    std::cout << m << '!' << n << '!' << z << std::endl;
    for (int i = 0; i < z; ++i)
    {
        if (s0[i] != s1[i])
        {
            std::cout << i << ' ' << i % 8 << ", ";
            std::cout << (int)s0[i] << '/' << (int)s1[i] << std::endl;
            // break
        }
        // std::cout << (int)t0[i] << ", ";
    }

    volatile uint32_t endian = 0x01234567;
    uint16_t turns0 = (*((uint8_t *)(&endian))) == 0x67
                          ? state.battle.bytes[368] | state.battle.bytes[369] << 8
                          : state.battle.bytes[368] << 8 | state.battle.bytes[369];
    uint16_t turns1 = (*((uint8_t *)(&endian))) == 0x67
                          ? state_.battle.bytes[368] | state_.battle.bytes[369] << 8
                          : state_.battle.bytes[368] << 8 | state_.battle.bytes[369];
    std::cout << "turns: " << (int)turns0 << ' ' << (int)turns1 << std::endl;
}

int main()
{
    // test2();
    // move(std::move(state));
    BattleTypes::State state{};
    for (int i = 0; i < 384; ++i)
    {
        std::cout << (int)state.battle.bytes[i] << " ";
    }
    std::cout << std::endl
              << std::endl;
    rollout_pipe(state);
    foo();
    // MonteCarloModel<BattleTypes>::Model model{0};
    // MonteCarloModel<BattleTypes>::ModelOutput output;
    // model.inference(std::move(state), output);
}
