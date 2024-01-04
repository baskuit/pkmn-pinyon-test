#include "./src/battle.hh"
#include "./src/mapped-state-alpha-beta.hh"
#include "./src/print.hh"

struct Branch
{
    BattleTypes::Prob prob;
    BattleTypes::Seed seed;
    BattleTypes::Obs obs;
};

/*
Pass hash map that takes chance actions and stores branches:
    prob, original obs, and seed that produced that chance action

*/
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

    const BattleTypes::ObsHash hasher{};

    size_t t = 0;
    while (unexplored.get() > -.5 && (t < tries))
    {
        const size_t seed = device.uniform_64();
        auto state_ = state;

        state_.randomize_transition(seed);
        state_.apply_actions(row_a, col_a);
        BattleTypes::Obs obs = state_.get_obs();

        // now we mask the duration bits
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
                std::cout << "chance brute forcing - same hash different obs!" << std::endl;
                std::cout << "incoming:" << std::endl;
                print_chance_actions(obs.get().data());
                std::cout << "existing:" << std::endl;
                print_chance_actions(obs_.get().data());
                std::cout << "hash:" << std::endl;
                std::cout << transition_hash << std::endl;
            }
        }

        ++t;
    }
}

void prob_test(
    const size_t n_games = 10000)
{
    int r, c;

    BattleTypes::PRNG device{0};
    for (int i = 0; i < n_games; ++i)
    {

        r = device.random_int(20) + 3;
        c = device.random_int(20) + 3;

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

            std::cout << "try_force completed:" << std::endl;
            std::cout << "unexplored: " << unexplored.get() << std::endl;

            state.apply_actions(
                state.row_actions[r],
                state.col_actions[c]);

            // if (unexplored.get() < -.4)
            // {
            //     std::cout << "holy" << std::endl;
            //     state.save_debug_log("out.txt");
            //     std::cout << r << ' ' << c << std::endl;

            //     int status[12];
            //     state.put_status<int>(status);
            //     std::cout << "Status:" << std::endl;
            //     for (int s = 0; s < 12; ++s)
            //     {
            //         std::cout << status[s] << ' ';
            //     }
            //     std::cout << std::endl;

            //     for (auto pair : branches)
            //     {
            //         for (int i = 0; i < 16; ++i)
            //         {
            //             std::cout << (int)pair.second.obs.get()[i] << ' ';
            //         }
            //         std::cout << std::endl;
            //         print_chance_actions(pair.second.obs.get().data());
            //         std::cout << pair.second.prob.get() << std::endl;
            //     }
            //     exit(1);
            // }
        }
    }
}

void sleep_test()
{
    BattleTypes::State state{0, 0};
    BattleTypes::Obs obs;
    BattleTypes::Prob prob;

    BattleTypes::PRNG device{};
    uint64_t seed = 677749784; // device.get_seed();
    std::cout << seed << std::endl;
    state.randomize_transition(seed);

    state.apply_actions(
        state.row_actions[0],
        state.col_actions[0]);
    state.get_actions();

    state.apply_actions(
        state.row_actions[0],
        state.col_actions[1]);

    obs = state.get_obs();
    print_chance_actions(obs.get().data());
    prob = state.prob;
    std::cout << "prob: " << prob.get() << std::endl;
}

int main(int argc, char **argv)
{
    prob_test();
    return 0;
}
