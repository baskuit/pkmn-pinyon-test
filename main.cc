#include "./src/battle.hh"
#include "./src/mapped-state-alpha-beta.hh"

void save_debug_bytes(
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

void mapped_state_test(
    BattleTypes::State &state)
{
    // performing efficient alpha beta on each lvl 1 matrix node

    using Types = TreeBanditThreaded<Exp3<MonteCarloModel<BattleTypes>>>;
    using T = SearchModel<Types, false>;

    const size_t iterations = 1 << 20;
    Types::PRNG device{0};
    T::Model model{iterations, device, {0}, {{.1}, 4}};

    using AB = AlphaBeta<EmptyModel<MappedState<T>>>;
    // expands state up to depth 2 and call search.run_for_iterations for get_payoff
    AB::State ab_state{
        2,       // depth
        1 << 23, // tries
        device,
        state,
        model};

    // const size_t n = ab_state.explored_tree->child->stats.count;
    // for (const auto branch : ab_state.explored_tree->child->stats.branches) {
    //     const float p = (float)branch.second.count / n;
    //     const float ratio = p / static_cast<float>(branch.second.prob);
    //     const int x = std::log(ratio) / std::log(19.5) + .01;
    //     std::cout << (float)branch.second.count / n * 1000 << " ~ " <<  branch.second.prob * 1000 * std::pow(19.5, x) << "; ";
    // }

    // return;

    std::cout << "TOTAL: " << ab_state.node->count_matrix_nodes() << std::endl;

    AB::Search ab_solver{};
    AB::Model ab_model{};
    AB::MatrixNode ab_root{};

    // solves until terminal, which is up to the depth set above
    ab_solver.run(2, device, ab_state, ab_model, ab_root);
    std::cout << "SEARCHED: " << ab_root.count_matrix_nodes() << std::endl;

    AB::MatrixStats &stats = ab_root.stats;

    math::print(stats.I);
    math::print(stats.row_solution);
    math::print(stats.J);
    math::print(stats.col_solution);
    std::cout << stats.row_solution.size() << ' ' << stats.col_solution.size() << std::endl;
    stats.data_matrix.print();
}

int main(int argc, char **argv)
{

    BattleTypes::State state{1, 1};
    // use full rolls instead of default
    state.clamp = false;
    // get past trivial switch-in state
    state.apply_actions(
        state.row_actions[0], state.col_actions[0]);
    state.get_actions();

    using Exp3Model = SearchModel<
        TreeBandit<Exp3<MonteCarloModel<BattleTypes>>>>;

    Exp3Model::PRNG device{0};

    Exp3Model::Model exp3_model{
        1 << 12, device, {0}, {}};

    using AlphaBetaModel = MappedAlphaBetaModel<Exp3Model>;

    AlphaBetaModel::Model mapped_alpha_beta_model{
        1, 1 << 20, device, exp3_model};

    using Types = FullTraversal<EmptyModel<MappedState<AlphaBetaModel>>>;

    Types::State root_state{1, 1 << 20, device, state, mapped_alpha_beta_model};
    Types::Model model{};
    Types::Search search{};
    Types::MatrixNode node{};
    search.run(1, device, root_state, model, node);

    std::cout << "VALUE:" << std::endl;
    std::cout << node.stats.payoff.get_row_value() << std::endl;
    std::cout << "STRATEGIES:" << std::endl;
    math::print(node.stats.row_solution);
    math::print(node.stats.col_solution);
    std::cout << "PAYOFF MATRIX:" << std::endl;
    node.stats.nash_payoff_matrix.print();
}
