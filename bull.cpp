#include <gmpxx.h>

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>

#include <assert.h>

#include "extern/eigen/Eigen/Dense"

void printMatrix(const Eigen::Matrix<mpq_class, 4, 4> &matrix)
{
    for (int i = 0; i < matrix.rows(); ++i)
    {
        for (int j = 0; j < matrix.cols(); ++j)
        {
            std::cout << matrix(i, j).get_str() << '\t';
        }
        std::cout << std::endl;
    }
}

const size_t MAX_HP = 353;
// after this point, we calculate instead of using analytic solution.
// may improve but currently stops when out of body slam range for both players
const size_t MIN_HP = 0;

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
    &BLIZZARD,
    &HYPER_BEAM};
const int N_MOVES = MOVES.size();

const std::vector<const Move *> MOVES_WITH_RECHARGE{
    &BODY_SLAM,
    &BLIZZARD,
    &HYPER_BEAM,
    &RECHARGE};
const int N_MOVES_WITH_RECHARGE = MOVES_WITH_RECHARGE.size();

struct State
{
    int hp_1;
    int hp_2;
    bool r_1;
    bool r_2;

    // for unordered_map support
    operator size_t() const
    {
        return 0;
    }
};

void print(const State &state)
{
    std::cout << state.hp_1 << ' ' << state.hp_2 << ' ' << state.r_1 << ' ' << state.r_2 << std::endl;
}

size_t hash(const State &state)
{
    return 4 * MAX_HP * state.hp_1 + 4 * state.hp_2 + 2 * state.r_1 + state.r_2;
}

size_t hash_a(const int r1, const int r2, const int m1, const int m2)
{
    return r1 * 2 * N_MOVES * N_MOVES + r2 * N_MOVES * N_MOVES + m1 * N_MOVES + m2;
}

std::tuple<int, int, int, int> unhash_a(size_t h)
{
    const int m2 = h % N_MOVES;
    h -= m2;
    h /= N_MOVES;
    const int m1 = h % N_MOVES;
    h -= m1;
    h /= N_MOVES;
    const int r2 = h % 2;
    h -= r2;
    h /= 2;
    const int r1 = h;
    return {r1, r2, m1, m2};
}

// State unhash(size_t hash)
// {
//     int hp_1, hp_2, r_1, r_2;
//     r_2 = hash % 2;
//     hash -= r_2;
//     r_1

// }

struct Branch
{
    State state;
    // probability of transition
    mpq_class p;
};

struct Solution
{
    std::unordered_map<size_t, mpq_class> value{};
    std::unordered_map<size_t, std::vector<Move *>> moves{};
};

mpq_class lookup_value(
    const Solution &solution,
    const State &state)
{
    if (state.hp_1 == 0)
    {
        return {0};
    }
    if (state.hp_2 == 0)
    {
        return {1};
    }

        

    // print(state);
    if (state.hp_1 < state.hp_2)
    {
        size_t h = hash(State{state.hp_2, state.hp_1, state.r_2, state.r_2});
        mpq_class v = solution.value.at(h);
        v = mpq_class{1} - v;
        v.canonicalize();
        return v;
    }
    else
    {
        size_t h = hash(state);
        return solution.value.at(h);
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
    std::vector<Branch> &branches)
{
    mpq_class total_prob{0};

    for (int i = 0; i < 16; ++i)
    {
        total_prob.canonicalize();

        // iterate over all accuracy and freeze checks
        int hit_1 = i & 1;
        int hit_2 = i & 2;
        int proc_1 = i & 4;
        int proc_2 = i & 8;

        // corresponding probs
        mpq_class acc_1 = hit_1 ? move_1.acc : move_1.one_minus_acc;
        mpq_class acc_2 = hit_2 ? move_2.acc : move_2.one_minus_acc;
        mpq_class frz_1 = proc_1 ? move_1.frz : move_1.one_minus_frz;
        mpq_class frz_2 = proc_2 ? move_2.frz : move_2.one_minus_frz;

        mpq_class p = acc_1 * acc_2 * frz_1 * frz_2;
        p.canonicalize();

        bool p1_frz_win = hit_1 && proc_1 && move_1.may_freeze;
        bool p2_frz_win = hit_2 && proc_2 && move_2.may_freeze;

        // check if frz win
        if (p1_frz_win)
        {
            // should not be affected by the speed tie stuff?
            total_prob += p;

            if (p2_frz_win)
            {
                // speed tie, only modify p so that value is correct
                p *= mpq_class{1, 2};
                p.canonicalize();
                value += p;
                value.canonicalize();
            }
            else
            {
                total_prob += p;

                value += p;
                value.canonicalize();
            }
            continue;
        }
        if (p2_frz_win)
        {
            // p1 loss, add 0...
            continue;
        }

        for (int j = 0; j < 4; ++j)
        {
            // iterate over a crit checks
            const int crit_1 = j & 1;
            const int crit_2 = j & 2;

            const mpq_class &crit_p_1 = crit_1 ? CRIT : NO_CRIT;
            const mpq_class &crit_p_2 = crit_2 ? CRIT : NO_CRIT;
            mpq_class crit_p = p * crit_p_1 * crit_p_2;
            crit_p.canonicalize();

            const std::vector<Roll> &rolls_1 = hit_1 ? (crit_1 ? move_1.rolls : move_1.crit_rolls) : RECHARGE.rolls;
            const std::vector<Roll> &rolls_2 = hit_2 ? (crit_2 ? move_2.rolls : move_2.crit_rolls) : RECHARGE.rolls;

            for (const Roll &roll_1 : rolls_1)
            {
                for (const Roll &roll_2 : rolls_2)
                {
                    // iterate over all damage rolls
                    mpq_class roll_probs{roll_1.n * roll_2.n, 39 * 39};
                    roll_probs.canonicalize();
                    mpq_class q = crit_p * roll_probs;
                    q.canonicalize();

                    total_prob += q;

                    int post_hp_1 = std::max(state.hp_1 - roll_2.dmg * hit_2, 0);
                    int post_hp_2 = std::max(state.hp_2 - roll_1.dmg * hit_1, 0);

                    bool p1_ko_win = (post_hp_2 == 0);
                    bool p2_ko_win = (post_hp_1 == 0);

                    if (p1_ko_win)
                    {
                        // don't need to increm
                        if (p2_ko_win)
                        {
                            q *= mpq_class{1, 2};
                            q.canonicalize();
                            value += q;
                            value.canonicalize();
                        }
                        else
                        {
                            value += q;
                            value.canonicalize();
                        }
                        continue;
                    }
                    if (p2_ko_win)
                    {
                        // p1 loss, add 0...
                        continue;
                    }

                    const State child{post_hp_1, post_hp_2, move_1.must_recharge, move_2.must_recharge};
                    const size_t child_hash = hash(child);
                    if ((post_hp_1 == state.hp_1) && (post_hp_2 == state.hp_2))
                    {
                        branches.push_back({child, q});
                    }
                    else
                    {
                        // child state has less hp, lookup and increment
                        mpq_class temp = q * lookup_value(tables, child);
                        temp.canonicalize();
                        value += temp;
                        value.canonicalize();
                    }
                }
            }
        }
    }
    // std::cout << "Transition total probs: " << total_prob.get_str() << std::endl;
}

void solve_hp(
    Solution &tables,
    const int hp_1,
    const int hp_2)
{
    std::unordered_map<
        size_t,
        std::tuple<mpq_class, mpq_class, State>>
        mem{};
    // what simple equation does a joint action applied to a state induce?

    int first_count = 0;
    int second_count = 0;

    for (int m1 = 0; m1 < N_MOVES_WITH_RECHARGE; ++m1)
    {
        const Move *move_1 = MOVES_WITH_RECHARGE[m1];
        for (int m2 = 0; m2 < N_MOVES_WITH_RECHARGE; ++m2)
        {
            const Move *move_2 = MOVES_WITH_RECHARGE[m2];

            // std::cout << std::endl;
            // std::cout << "Move 1: " << move_1->id << " Move 2: " << move_2->id << std::endl;

            for (int i = 0; i < 4; ++i)
            {
                int recharge_1 = i & 1;
                int recharge_2 = (i & 2) / 2;

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
                const State state{hp_1, hp_2, recharge_1 > 0, recharge_2 > 0};
                const size_t hash_ = hash_a(recharge_1, recharge_2, m1, m2);

                transitions(state, tables, *move_1, *move_2, value, branches);

                mpq_class total_branch_prob{0};
                const State child_state = branches[0].state;
                for (const Branch &branch : branches)
                {
                    total_branch_prob += branch.p;
                    total_branch_prob.canonicalize();
                    assert(child_state.r_1 == branch.state.r_1 && child_state.r_2 == branch.state.r_2);
                }

                // This assert fails due to recharge
                // miss = 0 recharge
                // std::cout << "MISS: " << miss.get_str() << ' ' << total_branch_prob.get_str() << std::endl;
                assert(miss == total_branch_prob);

                mem[hash_] = {value, total_branch_prob, child_state};

                ++first_count;

                // assert(total_branch_prob == miss);

                // x = value + (miss) * z
                // x =
                // where y is already known, but z is a peer state
            }
        }
    }

    // now iterate over every possible pure strategy over the equal hp states
    // and derive a system of simple equations for the value of states given those pure strategies
    // then solve and store in NE matrix

    // Commented out for now

    // using Strategy = Move *[4];

    // every possible joint strategy

    mpq_class p1_value_matrix[N_MOVES * N_MOVES][N_MOVES * N_MOVES];

    std::unordered_map<size_t,
                       std::tuple<mpq_class, mpq_class, mpq_class, mpq_class>>
        values4{};

    for (int i = 0; i < N_MOVES * N_MOVES; ++i)
    {

        for (int j = 0; j < N_MOVES * N_MOVES; ++j)
        {
            const int a = i % N_MOVES;
            const int b = i / N_MOVES;
            const int c = j % N_MOVES;
            const int d = j / N_MOVES;

            const Move *const p1_s00 = MOVES[a];
            const Move *const p1_s01 = MOVES[b];
            const Move *const p2_s00 = MOVES[c];
            const Move *const p2_s10 = MOVES[d];

            // Now construct matrix

            Eigen::Matrix<mpq_class, 4, 4> coefficients = Eigen::Matrix<mpq_class, 4, 4>::Identity();

            const size_t h00 = hash_a(0, 0, a, c);
            const size_t h01 = hash_a(0, 1, b, N_MOVES);
            const size_t h10 = hash_a(1, 0, N_MOVES, d);
            const size_t h11 = hash_a(1, 1, N_MOVES, N_MOVES);

            const auto w = mem.at(h00);
            const auto x = mem.at(h01);
            const auto y = mem.at(h10);
            const auto z = mem.at(h11);

            mpq_class y00 = std::get<0>(w);
            mpq_class y01 = std::get<0>(x);
            mpq_class y10 = std::get<0>(y);
            mpq_class y11 = {0};
            assert(std::get<0>(z) == mpq_class{0});

            mpq_class p00 = std::get<1>(w);
            mpq_class p01 = std::get<1>(x);
            mpq_class p10 = std::get<1>(y);
            mpq_class p11 = {1};
            assert(std::get<1>(z) == mpq_class{1});

            const State &next_state00 = std::get<2>(w);
            const State &next_state01 = std::get<2>(x);
            const State &next_state10 = std::get<2>(y);
            const State &next_state11 = std::get<2>(z);

            coefficients(0, 2 * next_state00.r_1 + next_state00.r_2) -= p00;
            coefficients(1, 2 * next_state01.r_1 + next_state01.r_2) -= p01;
            coefficients(2, 2 * next_state10.r_1 + next_state10.r_2) -= p10;
            coefficients(3, 2 * next_state11.r_1 + next_state11.r_2) -= p11;

            // printMatrix(coefficients);
            // return;

            Eigen::Matrix<mpq_class, 4, 1> constants;
            constants << y00, y01, y10, y11;

            // Perform LU decomposition
            Eigen::FullPivLU<Eigen::Matrix<mpq_class, 4, 4>> lu(coefficients);

            // Solve the system of linear equations using LU decomposition
            Eigen::Matrix<mpq_class, 4, 1> solution = lu.solve(constants);

            mpq_class v00 = solution(0, 0);
            mpq_class v01 = solution(1, 0);
            mpq_class v10 = solution(2, 0);
            mpq_class v11 = solution(3, 0);

            std::tuple<mpq_class, mpq_class, mpq_class, mpq_class> value_tuple = {v00, v01, v10, v11};

            values4[i * N_MOVES * N_MOVES + j] = value_tuple;

            v00.canonicalize();
            v01.canonicalize();
            v10.canonicalize();
            v11.canonicalize();

            // std::cout << "P1 STRATEGY: " << p1_s00->id << ' ' << p1_s01->id << std::endl;
            // std::cout << "P2 STRATEGY: " << p2_s00->id << ' ' << p2_s10->id << std::endl;

            // std::cout << v00.get_str() << " = " << v00.get_d() << std::endl;
            // std::cout << v01.get_str() << " = " << v01.get_d() << std::endl;
            // std::cout << v10.get_str() << " = " << v10.get_d() << std::endl;
            // std::cout << v11.get_str() << " = " << v11.get_d() << std::endl;

            mpq_class total_p1_value = v00 + v01 + v10 + v11;
            total_p1_value.canonicalize();
            p1_value_matrix[i][j] = total_p1_value;
        }
    }

    // for (int i = 0; i < N_MOVES * N_MOVES; ++i)
    // {
    //     for (int j = 0; j < N_MOVES * N_MOVES; ++j)
    //     {
    //         std::cout << p1_value_matrix[i][j].get_d() << ' ';
    //     }
    //     std::cout << std::endl;
    // }

    // min max checks
    int best_r, best_c;

    mpq_class max{0};
    for (int r = 0; r < N_MOVES; ++r)
    {
        mpq_class min_{N_MOVES};

        for (int c = 0; c < N_MOVES; ++c)
        {
            mpq_class x = p1_value_matrix[r][c];
            if (x < min_)
            {
                min_ = x;
            }
        }

        if (min_ > max)
        {
            max = min_;
            best_r = r;
        }
    }

    mpq_class min{N_MOVES};
    for (int c = 0; c < N_MOVES; ++c)
    {
        mpq_class max_{0};

        for (int r = 0; r < N_MOVES; ++r)
        {
            mpq_class x = p1_value_matrix[r][c];
            if (x > max_)
            {
                max_ = x;
            }
        }

        if (max_ < min)
        {
            min = max_;
            best_c = c;
        }
    }

    const int a = best_r % N_MOVES;
    const int b = best_r / N_MOVES;
    const int c = best_c % N_MOVES;
    const int d = best_c / N_MOVES;

    std::cout << "FINAL VALUES" << std::endl;
    auto value_tuple = values4.at(best_r * N_MOVES * N_MOVES + best_c);
    std::cout << std::get<0>(value_tuple).get_str() << ' ';
    std::cout << std::get<1>(value_tuple).get_str() << ' ';
    std::cout << std::get<2>(value_tuple).get_str() << ' ';
    std::cout << std::get<3>(value_tuple).get_str() << std::endl;

    tables.value[hash({hp_1, hp_2, false, false})] = std::get<0>(value_tuple);
    tables.value[hash({hp_1, hp_2, false, true})] = std::get<1>(value_tuple);
    tables.value[hash({hp_1, hp_2, true, false})] = std::get<2>(value_tuple);
    tables.value[hash({hp_1, hp_2, true, true})] = std::get<3>(value_tuple);
}

void old_test()
{
    State init{1, 1, true, false};

    std::vector<Branch> branches{};

    mpq_class value{};

    Solution tables{};

    transitions(
        init,
        tables,
        RECHARGE,
        BODY_SLAM,
        value,
        branches);

    std::cout << "VALUE: " << value.get_str() << " = " << value.get_d() << std::endl;

    std::cout << "NEXT STATES: ";
    print(branches[0].state);

    mpq_class total_prob{0};

    for (const auto x : branches)
    {
        // print(x.state);
        total_prob += x.p;
        // total_prob.canonicalize();
    }
    std::cout << "NEXT STATE PROB: " << total_prob.get_str() << " = " << total_prob.get_d() << std::endl;
}

int main()
{
    Solution tables{};

    for (int hp_1 = 1; hp_1 <= MAX_HP; ++hp_1)
    {
        for (int hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            std::cout << "HP1: " << hp_1 << " HP2: " << hp_2 << std::endl;
            solve_hp(tables, hp_1, hp_2);
        }
    }

    // old_test();

    return 0;
}
