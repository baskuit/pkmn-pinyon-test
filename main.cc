#include "./src/battle.hh"
#include "./src/mapped-state-alpha-beta.hh"
#include "./src/print.hh"

struct Branch
{
    BattleTypes::Prob prob;
    BattleTypes::Seed seed;
    BattleTypes::Obs obs;
};

void try_force(
    BattleTypes::PRNG &device,
    const BattleTypes::State &state,
    BattleTypes::Action row_a, BattleTypes::Action col_a,
    std::unordered_map<size_t, Branch> &branches,
    BattleTypes::Prob &unexplored,
    const size_t tries)
{
    const static BattleTypes::Prob eps = .0000001;
    const static BattleTypes::Prob neps = -.0000001;

    BattleTypes::ObsHash hasher{};
    size_t t = 0;
    while (unexplored.get() > -1000 && (t < tries))
    {
        size_t seed = device.uniform_64();
        auto state_ = state;
        state_.randomize_transition(seed);
        state_.apply_actions(row_a, col_a);
        BattleTypes::Obs obs = state_.get_obs();
        // print_chance_actions(obs.get().data());
        const size_t transition_hash = hasher(obs);
        if (branches.find(transition_hash) == branches.end())
        {
            unexplored -= state_.prob;
            branches[transition_hash] = {state_.prob, seed, obs};
        }
        else
        {
            const Branch &branch = branches.at(transition_hash);
            const BattleTypes::Obs obs_ = branch.obs;

            if (obs != obs_)
            {
                for (int i = 0; i < 16; ++i)
                {
                    std::cout << (int)obs.get()[i] << ' ';
                }
                std::cout << std::endl;
                for (int i = 0; i < 16; ++i)
                {
                    std::cout << (int)obs_.get()[i] << ' ';
                }
                std::cout << std::endl;
                std::cout << transition_hash << std::endl;
                assert(false);
            }

            // if (unexplored < -.001)
            // {
            //     assert(false);
            // }

            ++t;
        }
    }
}

void test_prob()
{
    BattleTypes::PRNG device{0};
    for (int i = 0; i < 10000; ++i)
    {

        int r, c;
        r = device.random_int(20) + 2;
        c = device.random_int(20) + 2;

        std::cout << "Teams: " << r << ' ' << c << std::endl;

        BattleTypes::State state{r, c};
        state.clamp = false;

        while (!state.is_terminal())
        {
            state.get_actions();
            const size_t rows = state.row_actions.size();
            const size_t cols = state.col_actions.size();
            r = device.random_int(rows);
            c = device.random_int(cols);

            std::unordered_map<size_t, Branch> branches{};
            BattleTypes::Prob unexplored{1};
            try_force(
                device,
                state,
                state.row_actions[r], state.col_actions[c],
                branches, unexplored, 1 << 10);

            std::cout << "unexplored: " << unexplored.get() << std::endl;

            state.apply_actions(
                state.row_actions[r],
                state.col_actions[c]);
            if (unexplored.get() < -.4)
            {
                std::cout << "holy" << std::endl;
                state.save_debug_log("out.txt");
                std::cout << r << ' ' << c << std::endl;

                int status[12];
                state.put_status<int>(status);
                std::cout << "Status:" << std::endl;
                for (int s = 0; s < 12; ++s)
                {
                    std::cout << status[s] << ' ';
                }
                std::cout << std::endl;

                for (auto pair : branches)
                {
                    for (int i = 0; i < 16; ++i)
                    {
                        std::cout << (int)pair.second.obs.get()[i] << ' ';
                    }
                    std::cout << std::endl;
                    print_chance_actions(pair.second.obs.get().data());
                    std::cout << pair.second.prob.get() << std::endl;
                }
                exit(1);
            }
        }
    }
}

int main(int argc, char **argv)
{
    test_prob();
    // BattleTypes::State state{0, 0};
    // BattleTypes::PRNG device{};
    // state.randomize_transition(device.uniform_64());
    // // use full rolls instead of default
    // state.clamp = false;
    // // get past trivial switch-in state
    // state.apply_actions(
    //     state.row_actions[0], state.col_actions[0]);
    // state.get_actions();

    // state.apply_actions(
    //     state.row_actions[0], state.col_actions[0]);
    // state.get_actions();

    // std::array<uint8_t, 16> obs = state.get_obs().get();

    // for (const uint8_t x : obs)
    // {
    //     std::cout << (int)x << ", ";
    // }
    // std::cout << std::endl;

    // print_chance_actions(obs.data());
    // print_chance_action(obs.data());
    // print_chance_action(obs.data() + 8);

    // using Exp3Model = SearchModel<
    //     TreeBandit<Exp3<MonteCarloModel<BattleTypes>>>>;

    // Exp3Model::PRNG device{0};

    // Exp3Model::Model exp3_model{
    //     1 << 12, device, {0}, {}};

    // using AlphaBetaModel = MappedAlphaBetaModel<Exp3Model>;

    // AlphaBetaModel::Model mapped_alpha_beta_model{
    //     1, 1 << 20, device, exp3_model};

    // using Types = FullTraversal<EmptyModel<MappedState<AlphaBetaModel>>>;

    // Types::State root_state{1, 1 << 20, device, state, mapped_alpha_beta_model};
    // Types::Model model{};
    // Types::Search search{};
    // Types::MatrixNode node{};
    // search.run(1, device, root_state, model, node, 6);

    // std::endl;
    // std::cout << node.stats.payoff.get_row_value() << std::endl;
    // std::endl;
    // math::print(node.stats.row_solution);
    // math::print(node.stats.col_solution);
    // std::endl;
    // node.stats.nash_payoff_matrix.print();
}
