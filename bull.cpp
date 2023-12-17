#include <gmpxx.h>

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>

#include <assert.h>

#include "extern/eigen/Eigen/Dense"

template <size_t r, size_t c, size_t n>
using rat_matrix = std::array<std::array<std::array<mpq_class, n>, c>, r>;

template <size_t rows, size_t cols>
void printMatrix(const Eigen::Matrix<mpq_class, rows, cols> &matrix)
{
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            std::cout << matrix(i, j).get_str() << '\t';
        }
        std::cout << std::endl;
    }
}

const size_t MAX_HP = 353;
// after this point, we calculate instead of using analytic solution.
// may improve but currently stops when out of body slam range for both players
const size_t MIN_HP = 95;

const mpq_class CRIT{55, 256};
const mpq_class NO_CRIT{201, 256};

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

const std::vector<const Move *> MOVES{
    &BODY_SLAM,
    &HYPER_BEAM
    // &BLIZZARD
};
const size_t N_MOVES = MOVES.size();

const std::vector<const Move *> MOVES_WITH_RECHARGE{
    &BODY_SLAM,
    &HYPER_BEAM,
    // &BLIZZARD,
    &RECHARGE};
const size_t N_MOVES_WITH_RECHARGE = MOVES_WITH_RECHARGE.size();

// assert rolls are 'correct'
void move_rolls_assert()
{
    for (const Move *move : MOVES)
    {
        int a = 0;
        int b = 0;
        for (const auto roll : move->rolls)
        {
            a += roll.n;
        }
        for (const auto roll : move->crit_rolls)
        {
            b += roll.n;
        }
        assert(a == 39);
        assert(b == 39);
    }
}

struct State
{
    int hp_1;
    int hp_2;
    bool r_1;
    bool r_2;
};

void print_state(const State &state)
{
    std::cout << state.hp_1 << ' ' << state.hp_2 << ' ' << state.r_1 << ' ' << state.r_2 << std::endl;
}

State unhash_state(size_t h)
{
    const int m2 = h % 2;
    h -= m2;
    h /= 2;
    const int m1 = h % 2;
    h -= m1;
    h /= 2;
    const int r2 = h % MAX_HP;
    h -= r2;
    h /= MAX_HP;
    const int r1 = h;
    return {r1 + 1, r2 + 1, static_cast<bool>(m1), static_cast<bool>(m2)};
}

size_t hash_state(const State &state)
{
    size_t hash = 4 * MAX_HP * (state.hp_1 - 1) + 4 * (state.hp_2 - 1) + 2 * state.r_1 + state.r_2;
    const State copy = unhash_state(hash);
    // assert(state.hp_1 == copy.hp_1);
    // assert(state.hp_2 == copy.hp_2);
    // assert(state.r_1 == copy.r_1);
    // assert(state.r_2 == copy.r_2);
    return hash;
}

struct Branch
{
    State state;
    // probability of transition
    mpq_class prob;
};

struct Solution
{
    std::unordered_map<size_t, mpq_class> value_table{};
    std::unordered_map<size_t, std::vector<const Move *>[2]> move_table {};
};

void init_tables(Solution &tables)
{
    for (int hp_1 = 1; hp_1 <= MIN_HP; ++hp_1)
    {
        for (int hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            tables.value_table[hash_state({hp_1, hp_2, false, false})] = mpq_class{1, 2};
            tables.value_table[hash_state({hp_1, hp_2, false, true})] = mpq_class{511, 512};
            tables.value_table[hash_state({hp_1, hp_2, true, false})] = mpq_class{1, 512};
            tables.value_table[hash_state({hp_1, hp_2, true, true})] = mpq_class{1, 2};
            tables.move_table[hash_state({hp_1, hp_2, false, false})][0] = {&BODY_SLAM};
            tables.move_table[hash_state({hp_1, hp_2, false, false})][1] = {&BODY_SLAM};
            tables.move_table[hash_state({hp_1, hp_2, false, true})][0] = {&BODY_SLAM};
            tables.move_table[hash_state({hp_1, hp_2, false, true})][1] = {&RECHARGE};
            tables.move_table[hash_state({hp_1, hp_2, true, false})][0] = {&RECHARGE};
            tables.move_table[hash_state({hp_1, hp_2, true, false})][1] = {&BODY_SLAM};
            tables.move_table[hash_state({hp_1, hp_2, true, true})][0] = {&RECHARGE};
            tables.move_table[hash_state({hp_1, hp_2, true, true})][1] = {&RECHARGE};
        }
    }
}

mpq_class lookup_value(
    const Solution &tables,
    const State &state)
{
    if (state.hp_1 < state.hp_2)
    {
        const size_t reverse_hash = hash_state(State{state.hp_2, state.hp_1, state.r_2, state.r_1});
        mpq_class v = tables.value_table.at(reverse_hash);
        v = mpq_class{1} - v;
        v.canonicalize();
        return v;
    }
    else
    {
        const size_t hash = hash_state(state);
        return tables.value_table.at(hash);
    }
}

// considers all possiple transitions from a state given joint actions
// any transition to a state with lesser hp will be looked up
// the value and associated probabilty will be incremented
// if the state cannot be looked up, we simply add its branch to the output vector
void transitions(
    const State &state,
    const Solution &tables,
    const Move &move_1,
    const Move &move_2,
    mpq_class &value,
    std::vector<Branch> &branches,
    const bool debug = false)
{
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

        // check if frz win
        if (p1_frz_win)
        {
            // should not be affected by the speed tie stuff?
            if (p2_frz_win)
            {
                // speed tie, only modify p
                value += hit_proc_prob * mpq_class{1, 2};

                // total_prob += hit_proc_prob;
                // children[-1] += hit_proc_prob * mpq_class{1, 2};
                // children[-2] += hit_proc_prob * mpq_class{1, 2};
                // children[-1].canonicalize();
                // children[-2].canonicalize();
            }
            else
            {
                value += hit_proc_prob;

                // total_prob += hit_proc_prob;
                // children[-1] += hit_proc_prob;
                // children[-1].canonicalize();
            }
            continue;
        }
        if (p2_frz_win)
        {
            // p1 loss, add 0...
            // total_prob += hit_proc_prob;
            // children[-2] += hit_proc_prob;
            // children[-2].canonicalize();
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

                            // children[-1] += crit_roll_prob * mpq_class{1, 2};
                            // children[-2] += crit_roll_prob * mpq_class{1, 2};
                            // children[-1].canonicalize();
                            // children[-2].canonicalize();
                        }
                        else
                        {
                            value += crit_roll_prob;

                            // children[-1] += crit_roll_prob;
                            // children[-1].canonicalize();
                        }
                        continue;
                    }
                    if (p2_ko_win)
                    {
                        // p1 loss, add 0...
                        // children[-2] += crit_roll_prob;
                        // children[-2].canonicalize();
                        continue;
                    }

                    const State child{post_hp_1, post_hp_2, move_1.must_recharge && hit_1, move_2.must_recharge && hit_2};
                    const size_t child_hash = hash_state(child);

                    // children[child_hash] += crit_roll_prob;
                    // children[child_hash].canonicalize();
                    if (!debug)
                    {
                        if ((post_hp_1 == state.hp_1) && (post_hp_2 == state.hp_2))
                        {
                            branches.push_back({child, crit_roll_prob});
                        }
                        else
                        {
                            // child state has less hp, lookup and increment
                            mpq_class weighted_solved_value = crit_roll_prob * lookup_value(tables, child);
                            weighted_solved_value.canonicalize();
                            value += weighted_solved_value;
                        }
                    }
                }
            }
        }
    }

    // if (debug)
    // {
    //     mpq_class child_prob{0};
    //     for (const auto &pair : children)
    //     {
    //         child_prob += pair.second;
    //         child_prob.canonicalize();

    //         const State child = unhash_state(pair.first);
    //         std::cout << "STATE: ";
    //         print_state(child);
    //         std::cout << pair.second.get_d() << " = " << pair.second.get_str() << std::endl;
    //         if (tables.value_table.find(pair.first) != tables.value_table.end())
    //         {
    //             std::cout << "TABLE VALUE: " << tables.value_table.at(pair.first).get_str() << " = " << tables.value_table.at(pair.first).get_d() << std::endl;
    //         }
    //         std::cout << std::endl;
    //     }
    //     assert(child_prob == mpq_class{1});
    // }

    // assert(total_prob == mpq_class{1});
}

template <size_t r, size_t c, size_t n>
void print_solved_value_matrix(const rat_matrix<r, c, n> &matrix)
{
    for (int i = 0; i < r; ++i)
    {
        for (int j = 0; j < c; ++j)
        {
            for (int k = 0; k < n; ++k)
            {
                std::cout << matrix[i][j][k].get_d() << ' ';
            }
            std::cout << '\t';
        }
        std::cout << std::endl;
    }
}

void solve_hp(
    Solution &tables,
    const int hp_1,
    const int hp_2)
{
    // value, prob, state
    std::tuple<mpq_class, mpq_class, State>
        memo_matrix[N_MOVES_WITH_RECHARGE][N_MOVES_WITH_RECHARGE][2][2];

    // given (hp,) recharge states and joint actions,
    // what is the data from the transitions function?
    // That is, what is the weighted solved values, total branch prob, and unique state?
    for (int m1 = 0; m1 < N_MOVES_WITH_RECHARGE; ++m1)
    {
        const Move *move_1 = MOVES_WITH_RECHARGE[m1];
        for (int m2 = 0; m2 < N_MOVES_WITH_RECHARGE; ++m2)
        {
            const Move *move_2 = MOVES_WITH_RECHARGE[m2];

            for (int i = 0; i < 4; ++i)
            {
                const bool recharge_1 = i & 1;
                const bool recharge_2 = i & 2;

                if (
                    ((move_1 == &RECHARGE) != recharge_1) ||
                    ((move_2 == &RECHARGE) != recharge_2))
                {
                    continue;
                }

                // move has to miss
                mpq_class miss = move_1->one_minus_acc * move_2->one_minus_acc;
                miss.canonicalize();

                std::vector<Branch> branches{};
                mpq_class value{0};
                const State state{hp_1, hp_2, recharge_1, recharge_2};

                transitions(state, tables, *move_1, *move_2, value, branches);

                mpq_class total_branch_prob{0};
                const State &unique_child_state = branches[0].state;
                for (const Branch &branch : branches)
                {
                    total_branch_prob += branch.prob;
                    total_branch_prob.canonicalize();
                    assert(unique_child_state.r_1 == branch.state.r_1 && unique_child_state.r_2 == branch.state.r_2);
                }
                assert(miss == total_branch_prob);

                memo_matrix[m1][m2][(int)recharge_1][(int)recharge_2] =
                    {value, total_branch_prob, unique_child_state};
            }
        }
    }

    // now iterate over every possible pure strategy over the equal hp states
    // and derive a system of simple equations for the value of states given those pure strategies
    // then solve and store in NE matrix
    rat_matrix<4, 4, 1> solved_value_sum_matrix{};
    rat_matrix<4, 4, 3> solved_value_matrix{};

    // every possible joint strategy
    for (int row_strat = 0; row_strat < N_MOVES * N_MOVES; ++row_strat)
    {

        for (int col_strat = 0; col_strat < N_MOVES * N_MOVES; ++col_strat)
        {
            const int a = row_strat % N_MOVES;
            const int b = row_strat / N_MOVES;
            const int c = col_strat % N_MOVES;
            const int d = col_strat / N_MOVES;

            // Now construct matrix
            Eigen::Matrix<mpq_class, 4, 4>
                coefficients = Eigen::Matrix<mpq_class, 4, 4>::Identity();

            // {value, same hp prob, child state}

            mpq_class value00;
            mpq_class value01;
            mpq_class value10;
            mpq_class value11;

            // Construct transition matrix and solve for the 4 same-HP states
            {
                const auto w = memo_matrix[a][c][0][0];
                const auto x = memo_matrix[b][N_MOVES][0][1];
                const auto y = memo_matrix[N_MOVES][d][1][0];
                const auto z = memo_matrix[N_MOVES][N_MOVES][1][1];

                const mpq_class y00 = std::get<0>(w);
                const mpq_class y01 = std::get<0>(x);
                const mpq_class y10 = std::get<0>(y);
                const mpq_class y11 = std::get<0>(z);
                assert(y11 == mpq_class{0});

                const mpq_class p00 = std::get<1>(w);
                const mpq_class p01 = std::get<1>(x);
                const mpq_class p10 = std::get<1>(y);
                const mpq_class p11 = std::get<1>(z);
                assert(p11 == mpq_class{1});

                const State &next_state00 = std::get<2>(w);
                const State &next_state01 = std::get<2>(x);
                const State &next_state10 = std::get<2>(y);
                const State &next_state11 = std::get<2>(z);

                coefficients(0, 2 * next_state00.r_1 + next_state00.r_2) -= p00;
                coefficients(1, 2 * next_state01.r_1 + next_state01.r_2) -= p01;
                coefficients(2, 2 * next_state10.r_1 + next_state10.r_2) -= p10;
                coefficients(3, 2 * next_state11.r_1 + next_state11.r_2) -= p11;

                Eigen::Matrix<mpq_class, 4, 1> constants;
                constants << y00, y01, y10, y11;

                // Perform LU decomposition
                Eigen::FullPivLU<Eigen::Matrix<mpq_class, 4, 4>> lu(coefficients);

                // Solution is unique
                assert(lu.determinant() != mpq_class{0});

                // Solve the system of linear equations using LU decomposition
                // v00 is value of the state {hp1, hp2, false, false}, etc
                Eigen::Matrix<mpq_class, 4, 1> solution = lu.solve(constants);

                value00 = solution(0, 0);
                value01 = solution(1, 0);
                value10 = solution(2, 0);
                value11 = solution(3, 0);

                value00.canonicalize();
                value01.canonicalize();
                value10.canonicalize();
                value11.canonicalize();

                // // Print linear system for dubugging
                // if (hp_1 == 167 && hp_2 == 167 && true)
                // {
                //     std::cout << "LETTER MOVES: " << a << ' ' << b << ' ' << c << ' ' << d << std::endl;
                //     std::cout << "STRATS: " << row_strat << ' ' << col_strat << std::endl;
                //     std::cout << "MATRIX:" << std::endl;
                //     printMatrix<4, 4>(coefficients);
                //     std::cout << "CONSTANTS:" << std::endl;
                //     printMatrix<4, 1>(constants);
                //     std::cout << "SOLUTION:" << std::endl;
                //     printMatrix<4, 1>(solution);
                //     std::cout << std::endl;

                //     exit(1);
                // }
            };

            // non critical assert
            assert(value00 == value11);

            // hack
            mpq_class total_solved_value = value00 + value01 + value10 + value11;
            total_solved_value.canonicalize();
            solved_value_sum_matrix[row_strat][col_strat][0] = total_solved_value;

            solved_value_matrix[row_strat][col_strat][0] = value00;
            solved_value_matrix[row_strat][col_strat][1] = value01;
            solved_value_matrix[row_strat][col_strat][2] = value10;
        }
    }

    // min max checks
    int best_r, best_c;
    mpq_class max{0};
    mpq_class min{4};
    {
        for (int row_strat = 0; row_strat < N_MOVES * N_MOVES; ++row_strat)
        {
            mpq_class min_{4};

            for (int c = 0; c < N_MOVES * N_MOVES; ++c)
            {
                mpq_class x = solved_value_sum_matrix[row_strat][c][0];
                if (x < min_)
                {
                    min_ = x;
                }
            }

            if (min_ > max)
            {
                max = min_;
                best_r = row_strat;
            }
        }

        for (int col_strat = 0; col_strat < N_MOVES * N_MOVES; ++col_strat)
        {
            mpq_class max_{0};

            for (int r = 0; r < N_MOVES * N_MOVES; ++r)
            {
                mpq_class x = solved_value_sum_matrix[r][col_strat][0];
                if (x > max_)
                {
                    max_ = x;
                }
            }

            if (max_ < min)
            {
                min = max_;
                best_c = col_strat;
            }
        }
    };

    // print_solved_value_matrix<4, 4, 3>(solved_value_matrix);
    // print_solved_value_matrix<4, 4, 1>(solved_value_sum_matrix);
    // std::cout << "debug best strats: " << best_r << ' ' << best_c << std::endl;

    // assert that we actually found a NE - FINALLY
    {
        for (int col_strat = 0; col_strat < N_MOVES * N_MOVES; ++col_strat)
        {
            // p2 can't improve on best_c
            assert(solved_value_matrix[best_r][col_strat][0] >= solved_value_matrix[best_r][best_c][0]);
            assert(solved_value_matrix[best_r][col_strat][1] >= solved_value_matrix[best_r][best_c][1]);
            assert(solved_value_matrix[best_r][col_strat][2] >= solved_value_matrix[best_r][best_c][2]);
        }
        for (int row_strat = 0; row_strat < N_MOVES * N_MOVES; ++row_strat)
        {
            // p1 can't improve on best_r
            assert(solved_value_matrix[row_strat][best_c][0] <= solved_value_matrix[best_r][best_c][0]);
            assert(solved_value_matrix[row_strat][best_c][1] <= solved_value_matrix[best_r][best_c][1]);
            assert(solved_value_matrix[row_strat][best_c][2] <= solved_value_matrix[best_r][best_c][2]);
        }
    };

    // Same HP asserts
    if (hp_1 == hp_2)
    {
        // Switching stategies should flip expected score
        for (int i = 0; i < N_MOVES * N_MOVES; ++i)
        {
            for (int j = 0; j < N_MOVES * N_MOVES; ++j)
            {
                const mpq_class a = solved_value_matrix[i][j][0];
                const mpq_class b = solved_value_matrix[j][i][0];
                mpq_class c = a + b;
                c.canonicalize();
                assert(c == mpq_class{1});
            }
        }

        // So should switchin recharge when they have the same strats
        for (int i = 0; i < N_MOVES * N_MOVES; ++i)
        {
            const mpq_class a = solved_value_matrix[i][i][1];
            const mpq_class b = solved_value_matrix[i][i][2];
            mpq_class c = a + b;
            c.canonicalize();
            assert(c == mpq_class{1});
        }
    }

    // Answer found, adding to table
    const int a = best_r % N_MOVES;
    const int b = best_r / N_MOVES;
    const int c = best_c % N_MOVES;
    const int d = best_c / N_MOVES;

    tables.value_table[hash_state({hp_1, hp_2, false, false})] = solved_value_matrix[best_r][best_c][0];
    tables.value_table[hash_state({hp_1, hp_2, false, true})] = solved_value_matrix[best_r][best_c][1];
    tables.value_table[hash_state({hp_1, hp_2, true, false})] = solved_value_matrix[best_r][best_c][2];
    tables.value_table[hash_state({hp_1, hp_2, true, true})] = solved_value_matrix[best_r][best_c][0];

    // analytic asserts
    // if (hp_1 <= BODY_SLAM.rolls[0].dmg)
    // {
    //     assert((solved_value_matrix[best_r][best_c][0] == mpq_class{1, 2}));
    //     assert((solved_value_matrix[best_r][best_c][1] == mpq_class{511, 512}));
    //     assert((solved_value_matrix[best_r][best_c][2] == mpq_class{1, 512}));
    // }

    tables.move_table[hash_state({hp_1, hp_2, false, false})][0] = {MOVES[a]};
    tables.move_table[hash_state({hp_1, hp_2, false, false})][1] = {MOVES[c]};
    tables.move_table[hash_state({hp_1, hp_2, false, true})][0] = {MOVES[b]};
    tables.move_table[hash_state({hp_1, hp_2, false, true})][1] = {&RECHARGE};
    tables.move_table[hash_state({hp_1, hp_2, true, false})][0] = {&RECHARGE};
    tables.move_table[hash_state({hp_1, hp_2, true, false})][1] = {MOVES[d]};
    tables.move_table[hash_state({hp_1, hp_2, true, true})][0] = {&RECHARGE};
    tables.move_table[hash_state({hp_1, hp_2, true, true})][1] = {&RECHARGE};

    std::cout << "FINAL VALUES" << std::endl;
    std::cout << solved_value_matrix[best_r][best_c][0].get_str() << ' ';
    std::cout << solved_value_matrix[best_r][best_c][1].get_str() << ' ';
    std::cout << solved_value_matrix[best_r][best_c][2].get_str() << std::endl;
    std::cout << "STRATEGIES: " << std::endl;
    std::cout << MOVES[a]->id << ", " << MOVES[b]->id << std::endl;
    std::cout << MOVES[c]->id << ", " << MOVES[d]->id << std::endl;
    std::cout << std::endl;
}

void total_solve(
    Solution &tables,
    const int starting_hp = MIN_HP + 1)
{
    for (int hp_1 = starting_hp; hp_1 <= MAX_HP; ++hp_1)
    {
        for (int hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            std::cout << "HP1: " << hp_1 << " HP2: " << hp_2 << std::endl;
            solve_hp(tables, hp_1, hp_2);
        }
    }
}

int main()
{
    move_rolls_assert();

    Solution tables{};
    std::vector<Branch> branches{};
    mpq_class value{0};
    init_tables(tables);

    total_solve(tables);
    // transitions(
    //     {167, 167, false, false},
    //     tables,
    //     HYPER_BEAM,
    //     HYPER_BEAM,
    //     value,
    //     branches,
    //     false);
    // std::cout << value.get_str() << std::endl;
    return 0;
}