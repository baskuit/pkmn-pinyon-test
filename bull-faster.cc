#include <gmpxx.h>

#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <utility>
#include <sstream>

const size_t MAX_HP = 353;
const size_t MAX_PP = 4;
const size_t PP_ALL = (MAX_PP + 1) * (MAX_PP + 1) * (MAX_PP + 1) * (MAX_PP + 1);

const mpq_class CRIT{55, 256};
const mpq_class NO_CRIT{201, 256};

void save_map(
    const std::string filename,
    const std::unordered_map<size_t, mpq_class> &value_table)
{
    std::ofstream file{filename, std::ios::out | std::ios::trunc};

    if (!file.is_open())
    {
        std::cout << "FILE NOT OPEN" << std::endl;
        exit(1);
    }

    for (const auto pair : value_table)
    {
        file << pair.first << ' ' << pair.second.get_str() << std::endl;
    }

    file.close();
}

void load_map(
    const std::string filename,
    std::unordered_map<size_t, mpq_class> &map)
{
    std::ifstream file{filename};

    if (!file.is_open())
    {
        std::cout << "FILE NOT OPEN" << std::endl;
        exit(1);
    }
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss{line};
        std::string hash, rational;
        iss >> hash >> rational;
        map[std::stoul(hash)] = mpq_class{rational};
    }
}

struct Roll
{
    int dmg;
    int n;
};

struct Move
{
    std::string id;
    // probabilities. I assume (1 - p) can't be optimized if we use libgmp, so I double up
    mpq_class acc;
    mpq_class one_minus_acc;
    mpq_class frz;
    mpq_class one_minus_frz;

    // recharge gets one 0 dmg roll
    std::vector<Roll> rolls;
    std::vector<Roll> crit_rolls;
    // std::vector<Roll> burned_rolls;
    bool must_recharge;
    bool may_freeze;
    // bool may_burn;
};

const Move BODY_SLAM{
    "Body Slam",
    mpq_class{255, 256},
    mpq_class{1, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{95, 2}, {96, 2}, {97, 3}, {98, 2}, {99, 2}, {100, 2}, {101, 3}, {102, 2}, {103, 2}, {104, 3}, {105, 2}, {106, 2}, {107, 2}, {108, 3}, {109, 2}, {110, 2}, {111, 2}, {112, 1}},
    {{184, 1}, {185, 1}, {186, 1}, {187, 1}, {188, 2}, {189, 1}, {190, 1}, {191, 1}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}, {199, 2}, {200, 1}, {201, 1}, {202, 1}, {203, 1}, {204, 1}, {205, 2}, {206, 1}, {207, 1}, {208, 1}, {209, 1}, {210, 1}, {211, 2}, {212, 1}, {213, 1}, {214, 1}, {215, 1}, {216, 1}, {217, 1}},
    false,
    false};

const Move HYPER_BEAM{
    "Hyper Beam",
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{166, 1}, {167, 1}, {168, 1}, {169, 2}, {170, 1}, {171, 1}, {172, 2}, {173, 1}, {174, 1}, {175, 1}, {176, 2}, {177, 1}, {178, 1}, {179, 2}, {180, 1}, {181, 1}, {182, 2}, {183, 1}, {184, 1}, {185, 1}, {186, 2}, {187, 1}, {188, 1}, {189, 2}, {190, 1}, {191, 1}, {192, 2}, {193, 1}, {194, 1}, {195, 1}, {196, 1}},
    {{324, 1}, {325, 1}, {327, 1}, {328, 1}, {330, 1}, {331, 1}, {333, 1}, {334, 1}, {336, 1}, {337, 1}, {339, 1}, {340, 1}, {342, 1}, {343, 1}, {345, 1}, {346, 1}, {348, 1}, {349, 1}, {351, 1}, {352, 1}, {354, 1}, {355, 1}, {357, 1}, {358, 1}, {360, 1}, {361, 1}, {363, 1}, {364, 1}, {366, 1}, {367, 1}, {369, 1}, {370, 1}, {372, 1}, {373, 1}, {375, 1}, {376, 1}, {378, 1}, {379, 1}, {381, 1}},
    true,
    false};

const Move BLIZZARD{
    "Blizzard",
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    {{168, 1}, {169, 1}, {170, 2}, {171, 1}, {172, 1}, {173, 2}, {174, 1}, {175, 1}, {176, 1}, {177, 2}, {178, 1}, {179, 1}, {180, 2}, {181, 1}, {182, 1}, {183, 1}, {184, 2}, {185, 1}, {186, 1}, {187, 2}, {188, 1}, {189, 1}, {190, 1}, {191, 2}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}},
    false,
    true};

const Move RECHARGE{
    "Recharge",
    mpq_class{0, 1},
    mpq_class{1, 1},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{0, 39}},
    {{0, 39}},
    false,
    false};

const std::array<const Move *, 5> MOVES{
    &BODY_SLAM,
    &HYPER_BEAM,
    &BODY_SLAM,
    &BODY_SLAM,
    &RECHARGE};
const size_t N_MOVES = 4;

struct State
{
    uint16_t hp_1;
    uint16_t hp_2;
    bool recharge_1;
    bool recharge_2;
    std::array<uint8_t, 4> pp_1;
    std::array<uint8_t, 4> pp_2;

    State() {}

    State(
        const uint16_t hp_1,
        const uint16_t hp_2,
        const bool recharge_1,
        const bool recharge_2,
        const std::array<uint8_t, 4> pp_1_arr,
        const std::array<uint8_t, 4> pp_2_arr)
        : hp_1{hp_1}, hp_2{hp_2}, recharge_1{recharge_1}, recharge_2{recharge_2}, pp_1{pp_1_arr}, pp_2{pp_2_arr}
    {
    }

    bool operator==(const State &other) const
    {
        const bool hp_and_recharge_check =
            (hp_1 == other.hp_1) &&
            (hp_2 == other.hp_1) &&
            (recharge_1 == other.recharge_1) &&
            (recharge_2 == other.recharge_2);

        if (!hp_and_recharge_check)
        {
            return false;
        }

        for (int i = 0; i < 4; ++i)
        {
            if ((pp_1[i] != other.pp_1[i]) || (pp_2[i] != other.pp_2[i]))
            {
                return false;
            }
        }
        return true;
    }
};

void print_state(
    const State &state)
{
    std::cout << state.hp_1 << ' ' << state.hp_2 << ' ' << state.recharge_1 << ' ' << state.recharge_2 << ' ';
    std::cout << '{' << (int)state.pp_1[0] << ' ' << (int)state.pp_1[1] << ' ' << (int)state.pp_1[2] << ' ' << (int)state.pp_1[3] << "} ";
    std::cout << '{' << (int)state.pp_2[0] << ' ' << (int)state.pp_2[1] << ' ' << (int)state.pp_2[2] << ' ' << (int)state.pp_2[3] << "}";
}

// first entry is the value, the remaining 3 x 2 determine a prob distro over the 4 moves for both players
// (last entry can be elided)
using SolutionEntry = mpq_class[1 + 3 + 3];
struct Solution
{
    SolutionEntry data[353][353][2][2][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1][MAX_PP + 1];
};

mpq_class lookup_value(
    const Solution &tables,
    const State &state)
{
    // pp inspection day
    bool pp_1_out = true;
    bool pp_2_out = true;
    for (int m = 0; m < 4; ++m)
    {
        pp_1_out &= (state.pp_1[m] == 0);
        pp_2_out &= (state.pp_2[m] == 0);
    }
    if (pp_1_out)
    {
        if (pp_2_out)
        {
            return {1, 2};
        }
        else
        {
            return {0};
        }
    }
    if (pp_2_out)
    {
        return {1};
    }

    if (state.recharge_1 && state.recharge_2)
    {
        mpq_class value = tables.data[state.hp_1 - 1][state.hp_2 - 1][0][0][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]][0];
        return value;
    }

    if (state.hp_1 < state.hp_2)
    {
        mpq_class value = tables.data[state.hp_2 - 1][state.hp_1 - 1][state.recharge_2][state.recharge_1][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][0];
        value = mpq_class{1} - value;
        return value;
    }
    else
    {
        mpq_class value = tables.data[state.hp_1 - 1][state.hp_2 - 1][state.recharge_1][state.recharge_2][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]][0];
        return value;
    }
}

mpq_class q_value(
    const Solution &tables,
    const State &state,
    const int move_1_idx,
    const int move_2_idx,
    const bool debug = false)
{
    mpq_class value{0};

    const Move &move_1 = *MOVES[move_1_idx];
    const Move &move_2 = *MOVES[move_2_idx];

    // Debug only
    // State hashes in, prob of encountering out
    std::unordered_map<size_t, mpq_class> children{};
    mpq_class total_prob{0};

    for (int i = 0; i < 16; ++i)
    {
        // after incrementing and 'continue' call
        // total_prob.canonicalize();
        value.canonicalize();

        // iterate over all accuracy and freeze checks
        const bool hit_1 = i & 1;
        const bool hit_2 = i & 2;
        const bool proc_1 = i & 4;
        const bool proc_2 = i & 8;

        // corresponding probs
        const mpq_class acc_1 = hit_1 ? move_1.acc : move_1.one_minus_acc;
        const mpq_class acc_2 = hit_2 ? move_2.acc : move_2.one_minus_acc;
        const mpq_class frz_1 = proc_1 ? move_1.frz : move_1.one_minus_frz;
        const mpq_class frz_2 = proc_2 ? move_2.frz : move_2.one_minus_frz;

        mpq_class hit_proc_prob = acc_1 * acc_2 * frz_1 * frz_2;
        hit_proc_prob.canonicalize();

        // if (hit_proc_prob == mpq_class{0})
        // {
        //     // Don't need to check rolls if we are assuming freeze on move that can't freeze
        //     continue;
        // }

        const bool p1_frz_win = move_1.may_freeze && hit_1 && proc_1;
        const bool p2_frz_win = move_2.may_freeze && hit_2 && proc_2;

        if (p1_frz_win)
        {
            // should not be affected by the speed tie stuff?
            if (p2_frz_win)
            {
                value += hit_proc_prob * mpq_class{1, 2};
            }
            else
            {
                value += hit_proc_prob;
            }
            continue;
        }
        if (p2_frz_win)
        {
            // value += mpq_class{};
            continue;
        }

        for (int j = 0; j < 4; ++j)
        {
            // iterate over crit checks
            const bool crit_1 = j & 1;
            const bool crit_2 = j & 2;

            const mpq_class &crit_p_1 = crit_1 ? CRIT : NO_CRIT;
            const mpq_class &crit_p_2 = crit_2 ? CRIT : NO_CRIT;
            mpq_class crit_prob = hit_proc_prob * crit_p_1 * crit_p_2;
            crit_prob.canonicalize();

            const std::vector<Roll> &rolls_1 = hit_1 ? (crit_1 ? move_1.crit_rolls : move_1.rolls) : RECHARGE.rolls;
            const std::vector<Roll> &rolls_2 = hit_2 ? (crit_2 ? move_2.crit_rolls : move_2.rolls) : RECHARGE.rolls;

            for (const Roll &roll_1 : rolls_1)
            {
                for (const Roll &roll_2 : rolls_2)
                {
                    // iterate over all damage rolls
                    mpq_class roll_probs{roll_1.n * roll_2.n, 39 * 39};
                    roll_probs.canonicalize();
                    mpq_class crit_roll_prob = crit_prob * roll_probs;
                    crit_roll_prob.canonicalize();

                    total_prob += crit_roll_prob;

                    const int post_hp_1 = std::max(state.hp_1 - roll_2.dmg * hit_2, 0);
                    const int post_hp_2 = std::max(state.hp_2 - roll_1.dmg * hit_1, 0);

                    const bool p1_ko_win = (post_hp_2 == 0);
                    const bool p2_ko_win = (post_hp_1 == 0);

                    if (p1_ko_win)
                    {
                        if (p2_ko_win)
                        {
                            value += crit_roll_prob * mpq_class{1, 2};
                        }
                        else
                        {
                            value += crit_roll_prob;
                        }
                        continue;
                    }
                    if (p2_ko_win)
                    {
                        // value += crir_roll_prob * mpq_class{};
                        continue;
                    }

                    State child{
                        post_hp_1,
                        post_hp_2,
                        move_1.must_recharge && hit_1,
                        move_2.must_recharge && hit_2,
                        state.pp_1,
                        state.pp_2};

                    if (move_1_idx < 4)
                    {
                        // child.pp_1[move_1_idx] -= (child.pp_1[move_1_idx] > 0);
                        child.pp_1[move_1_idx] -= 1;
                    }
                    if (move_2_idx < 4)
                    {
                        // child.pp_2[move_2_idx] -= (child.pp_2[move_2_idx] > 0);
                        child.pp_2[move_2_idx] -= 1;
                    }

                    if (!debug)
                    {
                        mpq_class weighted_solved_value = crit_roll_prob * lookup_value(tables, child);
                        weighted_solved_value.canonicalize();
                        value += weighted_solved_value;
                    }
                }
            }
        }
    }
    return value;
}
void solve_state(
    Solution &tables,
    const State &state)
{
    using Matrix = std::array<std::array<mpq_class, 5>, 5>;
    Matrix data;

    std::vector<int> p1_legal_moves{};
    std::vector<int> p2_legal_moves{};

    // get legal moves
    {
        if (state.recharge_1)
        {
            p1_legal_moves.push_back(4);
        }
        else
        {
            for (int m = 0; m < 4; ++m)
            {
                if (state.pp_1[m] > 0)
                {
                    p1_legal_moves.push_back(m);
                }
            }
        }

        if (state.recharge_2)
        {
            p2_legal_moves.push_back(4);
        }
        else
        {
            for (int m = 0; m < 4; ++m)
            {
                if (state.pp_2[m] > 0)
                {
                    p2_legal_moves.push_back(m);
                }
            }
        }
    };

    // solve and fill value matrix
    for (const int i : p1_legal_moves)
    {
        for (const int j : p2_legal_moves)
        {
            data[i][j] = q_value(tables, state, i, j);
        }
    }

    // get (pure) NE

    mpq_class min{1};
    mpq_class max{0};
    int best_i;
    int best_j;

    {
        for (const int i : p1_legal_moves)
        {
            mpq_class min_{4};

            for (const int j : p2_legal_moves)
            {
                mpq_class x = data[i][j];
                if (x < min_)
                {
                    min_ = x;
                }
            }

            if (min_ > max)
            {
                max = min_;
                best_i = i;
            }
        }

        for (const int j : p2_legal_moves)
        {

            mpq_class max_{0};

            for (const int i : p1_legal_moves)
            {

                mpq_class x = data[i][j];
                if (x > max_)
                {
                    max_ = x;
                }
            }

            if (max_ < min)
            {
                min = max_;
                best_j = j;
            }
        }
    }

    // assert NE is pure
    {
        bool all_good = true;

        for (const int i : p1_legal_moves)
        {
            const bool br_0 = (data[i][best_j] <= data[best_i][best_j]);
            const bool br_1 = (data[i][best_j] <= data[best_i][best_j]);
            const bool br_2 = (data[i][best_j] <= data[best_i][best_j]);
            all_good &= (br_0 && br_1 && br_2);
        }

        for (const int j : p2_legal_moves)
        {
            const bool br_0 = (data[best_i][j] >= data[best_i][best_j]);
            const bool br_1 = (data[best_i][j] >= data[best_i][best_j]);
            const bool br_2 = (data[best_i][j] >= data[best_i][best_j]);
            all_good &= (br_0 && br_1 && br_2);
        }

        assert(all_good);
    }

    // add to cache
    SolutionEntry &entry = tables.data[state.hp_1 - 1][state.hp_2 - 1][state.recharge_1][state.recharge_2][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]];
    entry[0] = data[best_i][best_j];
    for (int s = 0; s < 6; ++s)
    {
        entry[s] = mpq_class{0};
    }
    if (best_i < 4)
    {
        entry[best_i] = mpq_class{1};
    }
    if (best_j < 4)
    {
        entry[best_j + 3] = mpq_class{1};
    }
}

void total_solve(
    Solution &tables)
{

    // only give first 2 moves pp
    const size_t max_pp_local = 25;

    const int last_save = 0;
    const int new_save = 20;

    for (uint16_t hp_1 = last_save + 1; hp_1 <= new_save; ++hp_1)
    {
        for (uint16_t hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {

            // iterate over pp values in dictionary order (skpping no pp)
            for (size_t pp_2_iter = 1; pp_2_iter < max_pp_local; ++pp_2_iter)
            {
                std::array<uint8_t, 4> pp_2_arr;
                size_t pp_2_temp = pp_2_iter;
                for (int pp_2_idx = 0; pp_2_idx < 4; ++pp_2_idx)
                {
                    pp_2_arr[pp_2_idx] = pp_2_temp % (MAX_PP + 1);
                    pp_2_temp -= pp_2_arr[pp_2_idx];
                    pp_2_temp /= (MAX_PP + 1);
                }

                for (size_t pp_1_iter = 1; pp_1_iter < max_pp_local; ++pp_1_iter)
                {
                    std::array<uint8_t, 4> pp_1_arr;
                    size_t pp_1_temp = pp_1_iter;
                    for (int pp_1_idx = 0; pp_1_idx < 4; ++pp_1_idx)
                    {
                        pp_1_arr[pp_1_idx] = pp_1_temp % (MAX_PP + 1);
                        pp_1_temp -= pp_1_arr[pp_1_idx];
                        pp_1_temp /= (MAX_PP + 1);
                    }

                    // Solve
                    {

                        const State state_00{hp_1, hp_2, false, false, pp_1_arr, pp_2_arr};
                        const State state_01{hp_1, hp_2, false, true, pp_1_arr, pp_2_arr};
                        const State state_10{hp_1, hp_2, true, false, pp_1_arr, pp_2_arr};

                        solve_state(tables, state_00);
                        solve_state(tables, state_01);
                        solve_state(tables, state_10);
                    };
                }
            }

            // progress report
            const SolutionEntry &entry = tables.data[hp_1 - 1][hp_2 - 1][0][0][4][4][0][0][4][4][0][0];
            std::cout << "HP: " << hp_1 << ' ' << hp_2 << " : " << entry[0].get_str() << std::endl;
            std::cout << "STRATEGY 1: ";
            for (int s = 0; s < 3; ++s) {
                std::cout << entry[s + 1].get_str() << '\t';
            }
            std::cout << std::endl;
            std::cout << "STRATEGY 2: ";
            for (int s = 0; s < 3; ++s) {
                std::cout << entry[s + 4].get_str() << '\t';
            }
            std::cout << std::endl;
        }
    }
}

int main()
{
    Solution *tables_ptr = new Solution;
    Solution &tables = *tables_ptr;
    // load_map("cache.txt", tables.value_table);
    total_solve(tables);
    // save_map("cache.txt", tables.value_table);

    return 0;
}