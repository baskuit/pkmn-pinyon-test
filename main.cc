#include "./src/battle.hh"

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

void rollout_print(
    BattleTypes::State &state,
    std::vector<uint8_t> &stream)
{
    BattleTypes::PRNG device{0};
    while (!state.is_terminal())
    {
        const size_t rows = state.row_actions.size();
        const size_t cols = state.col_actions.size();
        const int row_idx = device.random_int(rows);
        const int col_idx = device.random_int(cols);
        state.apply_actions(
            state.row_actions[row_idx],
            state.col_actions[col_idx],
            stream);
        state.get_actions();
    }
}

void test2() {
    std::vector<uint8_t> s0{}, s1{};

    BattleTypes::PRNG device{};
    BattleTypes::State state{};
    state.randomize_transition(device);
    BattleTypes::State state_{state};

    rollout_print(state, s0);
    rollout_print(state_, s1);

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
    test2();
    // move(std::move(state));
    // BattleTypes::State state{};
    // MonteCarloModel<BattleTypes>::Model model{0};
    // MonteCarloModel<BattleTypes>::ModelOutput output;
    // model.inference(std::move(state), output);
}
