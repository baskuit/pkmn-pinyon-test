#include "./src/battle.hh"

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
                int &count = hash_map[fast];
                ++count;
            }

            counts.get(row_idx, col_idx) = hash_map.size();
        }
    }

    counts.print();
}

void play_good_game_lol(
    const BattleTypes::State &state_)
{
    using Types = TreeBandit<Exp3<MonteCarloModel<BattleTypes>>>;
    Types::PRNG device{0};
    std::cout << "play_good_game device seed: " << device.get_seed() << std::endl;
    Types::State state{state_};
    state.print_log = true;
    Types::Model model{0};
    Types::Search search{};
    int t = 0;
    while (!state.is_terminal())
    {
        std::cout << "t: " << t << std::endl;
        Types::MatrixNode root{};
        search.run_for_iterations(10000, device, state, model, root);
        Types::VectorReal r, c;
        search.get_empirical_strategies(root.stats, r, c);

        const int row_idx = device.sample_pdf(r);
        const int col_idx = device.sample_pdf(c);
        state.apply_actions(
            state.row_actions[row_idx],
            state.col_actions[col_idx]);
        // after to not overwrite .row_actions, col_actions
        state.get_actions();
        ++t;
    }
}

void map_test(
    BattleTypes::State &state
)
{
    using Types = TreeBandit<Exp3<MonteCarloModel<BattleTypes>>>;
    Types::PRNG device{0};
    Types::Model model{0};
    Types::Search search{};

    using MapTypes = MappedState<Types>;

    state.apply_actions(
        state.row_actions[0], state.col_actions[0]);
    state.get_actions();

    MapTypes::State map_state{
        2,
        10,
        1 << 12,
        device,
        state,
        model,
        search};

    std::cout << map_state.node->count_matrix_nodes() << std::endl;
}

int main(int argc, char **argv)
{

    BattleTypes::State state{};
    state.print_log = true;
    state.apply_actions(
        state.row_actions[0], state.col_actions[0]
    );
    state.get_actions();
    BattleTypes::Seed seed;
    BattleTypes::PRNG device;

    // seed = 8046687488802545358;
    // device = BattleTypes::PRNG{seed};
    // state.randomize_transition(device);
    // state.apply_actions(
    //     state.row_actions[0], state.col_actions[0]
    // );
    // state.get_actions();

    // seed = 9054490934513262798;
    // device = BattleTypes::PRNG{seed};
    // state.randomize_transition(device);
    // state.apply_actions(
    //     state.row_actions[0], state.col_actions[5]
    // );
    // state.get_actions();

    // seed = 5999964962042600769;
    // device = BattleTypes::PRNG{seed};
    // state.randomize_transition(device);
    // state.apply_actions(
    //     state.row_actions[5], state.col_actions[6]
    // );
    // state.get_actions();

    map_test(state);
}
