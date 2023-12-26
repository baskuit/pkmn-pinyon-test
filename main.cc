#include "./src/battle.hh"

void save(
    const BattleTypes::State &state)
{
    std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt";
    remove(path.data());
    std::fstream file;
    file.open(path, std::ios::binary | std::ios::app);
    const size_t n = state.debug_log.size();
    file.write(reinterpret_cast<const char *>(state.debug_log.data()), n);
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
                int &count = hash_map[fast];
                ++count;
            }

            counts.get(row_idx, col_idx) = hash_map.size();
        }
    }

    counts.print();
}

template <typename Types>
void fill(
    typename Types::State &state,
    const typename Types::MatrixNode *matrix_node,
    std::filesystem::path path)
{
    if (matrix_node == nullptr) {
        return;
    }

    if (matrix_node->is_terminal())
    {
        state.save(path / "out.txt");
        std::string score_string = std::to_string(matrix_node->stats.solved_value.get_row_value().get());
        std::fstream file;
        file.open(path/score_string, std::ios::binary | std::ios::app);
        file.close();
        return;
    }

    state.get_actions();

    for (int row_idx = 0; row_idx < state.row_actions.size(); ++row_idx)
    {
        for (int col_idx = 0; col_idx < state.col_actions.size(); ++col_idx)
        {
            const typename Types::ChanceNode *chance_node = matrix_node->access(row_idx, col_idx);
            if (chance_node == nullptr) {
                continue;
            }

            std::vector<typename Types::Obs> chance_actions{};
            state.get_chance_actions(
                state.row_actions[row_idx],
                state.col_actions[col_idx],
                chance_actions
            );

            const auto &data = matrix_node->stats.data_matrix.get(row_idx, col_idx);

            auto joint_action_path = path;
            joint_action_path /=
                std::format("{}, {} - {}, {}", 
                    std::to_string(row_idx), std::to_string(col_idx),
                    std::to_string(data.alpha_explored.get()), std::to_string((float)data.unexplored.get()));
            std::filesystem::create_directory(joint_action_path);
            

            for (auto chance_action : chance_actions) {
                typename Types::State state_copy = state;
                state_copy.apply_actions(state.row_actions[row_idx], state.col_actions[col_idx], chance_action);
                auto chance_path = joint_action_path;
                chance_path /= std::format(
                    "{}, {}",
                    (int)(state_copy.prob.get() * 1000),
                    arrayToString(state_copy.get_obs().get()));
                std::filesystem::create_directory(chance_path);
                const typename Types::MatrixNode *matrix_node_next = chance_node->access(state_copy.obs);
                fill<Types>(
                    state_copy, matrix_node_next, chance_path);
            }
        }
    }
}

void mapped_state_test(
    BattleTypes::State &state)
{
    using Types = TreeBanditThreaded<Exp3<MonteCarloModel<BattleTypes>>>;
    Types::PRNG device{0};
    Types::Model model{0};
    Types::Search search{{.1}, 4};
    std::cout << "threads " << search.threads << std::endl;

    using AB = AlphaBeta<EmptyModel<MappedState<Types>>>;
    // expands state up to depth 2 and call search.run_for_iterations for get_payoff
    AB::State ab_state{
        2, //depth
        10000, //tries
        1 << 10, // playouts
        device,
        state,
        model,
        search};

    std::cout << "TOTAL: " << ab_state.node->count_matrix_nodes() << std::endl;

    AB::Search ab_solver{};
    AB::Model ab_model{};
    AB::MatrixNode ab_root{};

    // solves until terminal, which is up to the depth set above
    ab_solver.run(device, ab_state, ab_model, ab_root);
    std::cout << "SEARCHED: " << ab_root.count_matrix_nodes() << std::endl;

    AB::MatrixStats &stats = ab_root.stats;

    math::print(stats.I);
    math::print(stats.row_solution);
    math::print(stats.J);
    math::print(stats.col_solution);
    std::cout << stats.row_solution.size() << ' ' << stats.col_solution.size() << std::endl;
    stats.data_matrix.print();

    // fill<AB>(
    //     ab_state, 
    //     &ab_root, 
    //     "/home/user/Desktop/pkmn-pinyon-test/tree/");
}

int main(int argc, char **argv)
{

    BattleTypes::State state{};
    // get past trivial switch-in state
    state.apply_actions(
        state.row_actions[0], state.col_actions[0]);
    state.get_actions();
    // BattleTypes::Seed seed;
    // BattleTypes::PRNG device;

    mapped_state_test(state);

    // while (!state.is_terminal()) {
    //     const int rows = state.row_actions.size();
    //     const int cols = state.col_actions.size();
    //     state.apply_actions(
    //         state.row_actions[device.random_int(rows)],
    //         state.col_actions[device.random_int(cols)]
    //     );
    //     state.get_actions();
    // }

    // save(state);
}
