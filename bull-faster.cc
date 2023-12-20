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
#include <cmath>

// #include <pinyon.hh>

int f
(
    const int hp_1,
    const int hp_2
)
{
    return hp_1 * (hp_1 - 1) / 2 + hp_2 - 1;
}

constexpr size_t MAX_HP = 353;
constexpr size_t HP_ALL = MAX_HP * (MAX_HP + 1) / 2;
constexpr size_t MAX_PP[4] = {4, 2, 2, 0};
constexpr size_t PP_ALL = (MAX_PP[0] + 1) * (MAX_PP[1] + 1) * (MAX_PP[2] + 1) * (MAX_PP[3] + 1);

const mpq_class CRIT{55, 256};
const mpq_class NO_CRIT{201, 256};

std::ofstream OUTPUT_FILE{"out.txt", std::ios::out | std::ios::trunc};

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
    &BLIZZARD,
    &RECHARGE,
    &RECHARGE};
const size_t N_MOVES = 4;

using HP_T = uint16_t;
using PP_T = uint8_t;

struct State
{
    HP_T hp_1;
    HP_T hp_2;
    bool recharge_1;
    bool recharge_2;
    std::array<PP_T, 4> pp_1;
    std::array<PP_T, 4> pp_2;

    State() {}

    State(
        const HP_T hp_1,
        const HP_T hp_2,
        const bool recharge_1,
        const bool recharge_2,
        const std::array<PP_T, 4> pp_1_arr,
        const std::array<PP_T, 4> pp_2_arr)
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
struct SolutionEntry
{
    mpq_class value;
};

struct Solution
{
    SolutionEntry data[HP_ALL][3][MAX_PP[0] + 1][MAX_PP[1] + 1][MAX_PP[2] + 1][MAX_PP[3] + 1][MAX_PP[0] + 1][MAX_PP[1] + 1][MAX_PP[2] + 1][MAX_PP[3] + 1];
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

    // if (state.recharge_1 && state.recharge_2)
    // {
    //     const int index = f(state.hp_2, state.hp_1);
    //     mpq_class value = tables.data[state.hp_1 - 1][state.hp_2 - 1][0][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]].value;
    //     return value;
    // }

    if (state.hp_1 < state.hp_2)
    {
        const int index = f(state.hp_2, state.hp_1);
        mpq_class value = tables.data[index][state.recharge_2 + 2 * state.recharge_1][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]].value;
        value = mpq_class{1} - value;
        return value;
    }
    else
    {
        const int index = f(state.hp_1, state.hp_2);
        const int recharge_index = (state.recharge_1 && state.recharge_2) ? 0 : (2 * state.recharge_1 + state.recharge_2);
        mpq_class value = tables.data[index][recharge_index][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]].value;
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
        // value.canonicalize();

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
        // hit_proc_prob.canonicalize();

        if (hit_proc_prob == mpq_class{0})
        {
            // Don't need to check rolls if we are assuming freeze on move that can't freeze
            continue;
        }

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
            // crit_prob.canonicalize();

            const std::vector<Roll> &rolls_1 = hit_1 ? (crit_1 ? move_1.crit_rolls : move_1.rolls) : RECHARGE.rolls;
            const std::vector<Roll> &rolls_2 = hit_2 ? (crit_2 ? move_2.crit_rolls : move_2.rolls) : RECHARGE.rolls;

            for (const Roll &roll_1 : rolls_1)
            {
                for (const Roll &roll_2 : rolls_2)
                {
                    // iterate over all damage rolls
                    mpq_class roll_probs{roll_1.n * roll_2.n, 39 * 39};
                    // roll_probs.canonicalize();
                    mpq_class crit_roll_prob = crit_prob * roll_probs;
                    // crit_roll_prob.canonicalize();

                    total_prob += crit_roll_prob;

                    const HP_T post_hp_1 = std::max(state.hp_1 - roll_2.dmg * hit_2, 0);
                    const HP_T post_hp_2 = std::max(state.hp_2 - roll_1.dmg * hit_1, 0);

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
                        // weighted_solved_value.canonicalize();
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
    const State &state,
    std::vector<mpq_class> &row_strategy,
    std::vector<mpq_class> &col_strategy)
{
    using M = std::array<std::array<mpq_class, 5>, 5>;
    M data;

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
            mpq_class min_{1};

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

    // assert NE is pure. If not, solve using pinyon
    bool ne_is_pure = true;
    {
        for (const int i : p1_legal_moves)
        {
            ne_is_pure &= (data[i][best_j] <= data[best_i][best_j]);
        }

        for (const int j : p2_legal_moves)
        {
            ne_is_pure &= (data[best_i][j] >= data[best_i][best_j]);
        }
    }

    // add to cache
    const int index = f(state.hp_1, state.hp_2);
    SolutionEntry &entry = tables.data[index][state.recharge_2 + 2 * state.recharge_1][state.pp_1[0]][state.pp_1[1]][state.pp_1[2]][state.pp_1[3]][state.pp_2[0]][state.pp_2[1]][state.pp_2[2]][state.pp_2[3]];
    // ne_is_pure = false;
    if (!ne_is_pure)
    {
        // // std::cout << "PURE NE NOT FOUND!!! SOLVING" << std::endl;
        // const size_t rows = p1_legal_moves.size();
        // const size_t cols = p2_legal_moves.size();

        // using Matrix_ = Matrix<ConstantSum<1, 1>::Value<RealType<mpq_class>>>;
        // using Vector_ = std::vector<RealType<mpq_class>>;
        // Matrix_ payoff_matrix{rows, cols};

        // for (size_t row_idx = 0; row_idx < rows; ++row_idx)
        // {
        //     for (size_t col_idx = 0; col_idx < cols; ++col_idx)
        //     {
        //         payoff_matrix.get(row_idx, col_idx) = RealType<mpq_class>{data[p1_legal_moves[row_idx]][p2_legal_moves[col_idx]]};
        //     }
        // }

        // Vector_ r{rows};
        // Vector_ c{cols};
        // ConstantSum<1, 1>::Value<RealType<mpq_class>> value =
        //     LRSNash::solve(payoff_matrix, r, c);

        // entry.value = value.get_row_value().get();
        // for (int s = 0; s < rows; ++s)
        // {
        //     row_strategy[s] = r[s].get();
        // }
        // for (int s = 0; s < cols; ++s)
        // {
        //     col_strategy[s] = c[s].get();
        // }
        std::cout << "ERROR: NE IS NOT PURE" << std::endl;
        exit(1);
    }
    else
    {
        entry.value = data[best_i][best_j];
        row_strategy[best_i] = mpq_class{1};
        col_strategy[best_j] = mpq_class{1};
    }
}

void total_solve(
    Solution &tables)
{
    const int last_save = 0;
    const int new_save = 353;

    for (HP_T hp_1 = last_save + 1; hp_1 <= new_save; ++hp_1)
    {
        for (HP_T hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {

            // iterate over pp values in dictionary order (skpping no pp)
            for (size_t pp_2_iter = 1; pp_2_iter < PP_ALL; ++pp_2_iter)
            {
                std::array<PP_T, 4> pp_2_arr;
                size_t pp_2_temp = pp_2_iter;
                for (int pp_2_idx = 0; pp_2_idx < 4; ++pp_2_idx)
                {
                    pp_2_arr[pp_2_idx] = pp_2_temp % (MAX_PP[pp_2_idx] + 1);
                    pp_2_temp -= pp_2_arr[pp_2_idx];
                    pp_2_temp /= (MAX_PP[pp_2_idx] + 1);
                }

                for (size_t pp_1_iter = 1; pp_1_iter < PP_ALL; ++pp_1_iter)
                {
                    std::array<PP_T, 4> pp_1_arr;
                    size_t pp_1_temp = pp_1_iter;
                    for (int pp_1_idx = 0; pp_1_idx < 4; ++pp_1_idx)
                    {
                        pp_1_arr[pp_1_idx] = pp_1_temp % (MAX_PP[pp_1_idx] + 1);
                        pp_1_temp -= pp_1_arr[pp_1_idx];
                        pp_1_temp /= (MAX_PP[pp_1_idx] + 1);
                    }

                    // Solve
                    {
                        const State state_00{hp_1, hp_2, false, false, pp_1_arr, pp_2_arr};
                        const State state_01{hp_1, hp_2, false, true, pp_1_arr, pp_2_arr};
                        const State state_10{hp_1, hp_2, true, false, pp_1_arr, pp_2_arr};
                        std::vector<mpq_class> row_strategy{};
                        std::vector<mpq_class> col_strategy{};

                        for (int x = 0; x < 4; ++x) {
                            row_strategy.emplace_back();
                            col_strategy.emplace_back();
                        }

                        solve_state(tables, state_00, row_strategy, col_strategy);

                        if ((pp_1_iter == PP_ALL - 1) && (pp_2_iter == PP_ALL - 1))
                        {
                            // progress report
                            const int index = f(hp_1, hp_2);
                            const SolutionEntry &entry = tables.data[index][0][MAX_PP[0]][MAX_PP[1]][MAX_PP[2]][MAX_PP[3]][MAX_PP[0]][MAX_PP[1]][MAX_PP[2]][MAX_PP[3]];
                            // write to terminal
                            {
                                std::cout << "HP: " << hp_1 << ' ' << hp_2 << " : " << entry.value.get_str() << std::endl;
                                std::cout << "STRATEGY 1: ";
                                for (int s = 0; s < 4; ++s)
                                {
                                    std::cout << row_strategy[s].get_str() << '\t';
                                }
                                std::cout << std::endl;
                                std::cout << "STRATEGY 2: ";
                                for (int s = 0; s < 4; ++s)
                                {
                                    std::cout << col_strategy[s].get_str() << '\t';
                                }
                                std::cout << std::endl;
                            };
                            // write to file
                            {
                                OUTPUT_FILE << "HP: " << hp_1 << ' ' << hp_2 << " : " << entry.value.get_str() << std::endl;
                                OUTPUT_FILE << "STRATEGY 1: ";
                                for (int s = 0; s < 3; ++s)
                                {
                                    OUTPUT_FILE << row_strategy[s].get_str() << '\t';
                                }
                                OUTPUT_FILE << std::endl;
                                OUTPUT_FILE << "STRATEGY 2: ";
                                for (int s = 0; s < 3; ++s)
                                {
                                    OUTPUT_FILE << col_strategy[s].get_str() << '\t';
                                }
                                OUTPUT_FILE << std::endl;
                            };
                        }

                        solve_state(tables, state_01, row_strategy, col_strategy);
                        solve_state(tables, state_10, row_strategy, col_strategy);
                    };
                }
            }
            

        }
    }
}

int main()
{
    Solution *tables_ptr = new Solution;
    Solution &tables = *tables_ptr;
    std::cout << "SIZE OF SOLUTION TABLE: " << sizeof(tables) << std::endl;
    total_solve(tables);

    return 0;
}