#include "./src/battle.hh"

#include <fstream>

void rollout_write(
    BattleTypes::State &state,
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt")
{
    remove(path.data());
    BattleTypes::PRNG device{};
    std::fstream file;
    file.open(path, std::ios::binary | std::ios::app);

    // header
    file << uint8_t{1} << uint8_t{1} << uint8_t{64} << uint8_t{0};
    file.write(reinterpret_cast<char *>(state.battle.bytes), 384);

    for (int frame = 0; !state.is_terminal(); ++frame)
    {
        const int row_idx = device.random_int(state.row_actions.size());
        const int col_idx = device.random_int(state.col_actions.size());

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
        file.write(reinterpret_cast<char *>(state.battle.bytes), 384);
        file.write(result_, 1);
        file.write(row_action_, 1);
        file.write(col_action_, 1);

        // after to not overwrite .row_actions, col_actions
        state.get_actions();
        ++frame;

        // print obs
        // for (int j = 0; j < 16; ++j)
        // {
        //     std::cout << (int)state.obs.get().bytes[j] << ' ';
        // }
        // std::cout << std::endl;
    }
    file.close();
}

struct ArrayHash
{
    template <typename T, std::size_t N>
    std::size_t operator()(const std::array<T, N> &arr) const
    {
        std::size_t hash_value = 0;
        for (const auto &element : arr)
        {
            hash_value ^= std::hash<T>{}(element) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
        }
        return hash_value;
    }
};

// print number of observed chance branches
void count_branches(
    const BattleTypes::State &state)
{
    const size_t tries = 1 << 15;
    BattleTypes::PRNG device{0};

    const size_t rows = state.row_actions.size();
    const size_t cols = state.col_actions.size();

    std::cout << rows << ' ' << cols << std::endl;

    typename BattleTypes::MatrixInt counts{rows, cols};

    for (int row_idx = 0; row_idx < rows; ++row_idx)
    {
        for (int col_idx = 0; col_idx < cols; ++col_idx)
        {
            const auto row_action = state.row_actions[row_idx];
            const auto col_action = state.col_actions[col_idx];

            std::unordered_map<std::array<uint8_t, 376>, int, ArrayHash> hash_map;

            for (int i = 0; i < tries; ++i)
            {
                auto state_ = state;
                state_.randomize_transition(device);
                state_.apply_actions(row_action, col_action);

                std::array<uint8_t, 376> fast;
                memcpy(fast.data(), state_.battle.bytes, 376);
                int& count = hash_map[fast];
                ++count;
            }

            counts.get(row_idx, col_idx) = hash_map.size();
        }
    }

    counts.print();
}

int main(int argc, char **argv)
{
    BattleTypes::State state{};

    
    // auto state_ = state;
    // rollout_write(state_);

    using Types = TreeBandit<Exp3<MonteCarloModel<BattleTypes>>>;
    Types::Model model{0};
    Types::PRNG device{2};
    Types::Search search{};
    Types::MatrixNode root{};
    search.run_for_iterations(10000, device, state, model, root);
    size_t count = root.count_matrix_nodes();
    std::cout << "count: " << count << std::endl;

    // state.apply_actions(state.row_actions[0], state.col_actions[0]);
    // state.get_actions();

    // count_branches(state);
}
