#include "./src/battle.hh"
#include "./src/mapped-state-alpha-beta.hh"
#include "./src/print.hh"

using Types = BattleTypes<true>;

void sleep_test()
{
    Types::State state{0, 0};
    state.clamp = true;
    Types::Obs obs;
    Types::Prob prob;

    Types::PRNG device{};
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


struct Branch
{
    Types::Prob prob;
    Types::Seed seed;
    Types::Obs obs;
    size_t count;
};

/*
Pass hash map that takes chance actions and stores branches:
    prob, original obs, and seed that produced that chance action

*/
void try_force(
    Types::PRNG &device,
    const Types::State &state,
    Types::Action row_a, Types::Action col_a,
    std::unordered_map<size_t, Branch> &branches,
    Types::Prob &unexplored,
    const size_t tries)
{
    const static Types::Prob eps = .0000001;
    const static Types::Prob neps = -.0000001;

    const Types::ObsHash hasher{};

    size_t t = 0;
    while (unexplored.get() > -.5 && (t < tries))
    {
        const size_t seed = device.uniform_64();
        auto state_ = state;

        state_.randomize_transition(1596898293987374370);
        state_.apply_actions(row_a, col_a);
        Types::Obs obs = state_.get_obs();

        // now we mask the duration bits
        const size_t transition_hash = hasher(obs);

        if (branches.find(transition_hash) == branches.end())
        {
            unexplored -= state_.prob;
            branches[transition_hash] = {state_.prob, seed, obs, 1};
        }
        else
        {
            Branch &branch = branches.at(transition_hash);
            branch.count += 1;
            const Types::Obs obs_ = branch.obs;

            if (obs != obs_)
            {
                static const uint64_t duration_mask = 0xFFFFFFFFFF0FFFFF;
                const uint64_t *a = reinterpret_cast<const uint64_t *>(obs.value.data());
                const uint64_t *b = reinterpret_cast<const uint64_t *>(obs_.value.data());
                if (
                    ((a[0] & duration_mask) == (b[0] & duration_mask)) &&
                    ((a[1] & duration_mask) == (b[1] & duration_mask)))
                {
                    // they are in fact the same - minus the duration
                }
                else
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
        }

        ++t;
    }
}

void prob_test(
    const size_t n_games = 10000)
{
    int r, c;

    Types::PRNG device{1};
    for (int i = 0; i < n_games; ++i)
    {

        r = device.random_int(20) + 3;
        c = device.random_int(20) + 3;

        Types::State state{r, c};
        state.clamp = true;

        int turns = 0;

        while (!state.is_terminal())
        {
            state.get_actions();
            const size_t rows = state.row_actions.size();
            const size_t cols = state.col_actions.size();
            r = device.random_int(rows);
            c = device.random_int(cols);

            std::unordered_map<size_t, Branch> branches{};
            Types::Prob unexplored{1};
            try_force(
                device,
                state,
                state.row_actions[r], state.col_actions[c],
                branches, unexplored, 1 << 16);

            const float u = unexplored.get();
            if (u < -.01 or u > .05)
            {
                std::cout << "options chance bytes prior to commit:" << std::endl;
                auto *ptr = pkmn_gen1_battle_options_chance_actions(&state.options);
                print_chance_actions(ptr->bytes);

                state.apply_actions(
                    state.row_actions[r],
                    state.col_actions[c]);
            }
            else
            {
                state.apply_actions(
                    state.row_actions[r],
                    state.col_actions[c]);
            }

            if (u < -.01 or u > .05)
            {
                std::cout << "bad unexplored value: " << u << std::endl;
                state.save_debug_log();

                for (auto pair : branches)
                {
                    std::cout << "raw chance action bytes:" << std::endl;
                    for (int i = 0; i < 16; ++i)
                    {
                        std::cout << (int)pair.second.obs.get()[i] << ' ';
                    }
                    std::cout << std::endl;
                    print_chance_actions(pair.second.obs.get().data());
                    std::cout << "prob: " << pair.second.prob.get() << std::endl;
                    std::cout << "count: " << pair.second.count << std::endl;
                }

                exit(1);
            }

            ++turns;
        }
    }
}


int main(int argc, char **argv)
{
    prob_test();
    return 0;
}
