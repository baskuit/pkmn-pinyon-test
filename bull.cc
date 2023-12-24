#include <pinyon.hh>

#include <string>
#include <array>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <fstream>

std::ofstream OUTPUT_FILE{"out.txt", std::ios::out | std::ios::trunc};

const size_t MAX_HP = 353;

using HP_T = int;

const HP_T BURN_DMG = MAX_HP / 16;

const mpq_class CRIT{55, 256};
const mpq_class NO_CRIT{201, 256};

struct Roll
{
    HP_T dmg;
    int n;
};

struct Move
{
    std::string id;
    // probabilities. I assume (1 - p) can't be optimized if we use libgmp, so I double up
    mpq_class acc;
    mpq_class one_minus_acc;
    mpq_class eff;
    mpq_class one_minus_eff;

    // recharge gets one 0 dmg roll
    std::vector<Roll> rolls;
    std::vector<Roll> crit_rolls;
    std::vector<Roll> burned_rolls;
    bool must_recharge;
    bool may_freeze;
    bool may_burn;
};

const Move BODY_SLAM{
    "Body Slam",
    mpq_class{255, 256},
    mpq_class{1, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{95, 2}, {96, 2}, {97, 3}, {98, 2}, {99, 2}, {100, 2}, {101, 3}, {102, 2}, {103, 2}, {104, 3}, {105, 2}, {106, 2}, {107, 2}, {108, 3}, {109, 2}, {110, 2}, {111, 2}, {112, 1}},
    {{184, 1}, {185, 1}, {186, 1}, {187, 1}, {188, 2}, {189, 1}, {190, 1}, {191, 1}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}, {199, 2}, {200, 1}, {201, 1}, {202, 1}, {203, 1}, {204, 1}, {205, 2}, {206, 1}, {207, 1}, {208, 1}, {209, 1}, {210, 1}, {211, 2}, {212, 1}, {213, 1}, {214, 1}, {215, 1}, {216, 1}, {217, 1}},
    {{48, 3}, {49, 4}, {50, 5}, {51, 4}, {52, 5}, {53, 4}, {54, 5}, {55, 4}, {56, 4}, {57, 1}},
    false,
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
    {{84, 2}, {85, 3}, {86, 3}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 3}, {94, 2}, {95, 3}, {96, 2}, {97, 3}, {98, 2}, {99, 1}},
    true,
    false,
    false};

const Move BLIZZARD{
    "Blizzard",
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{19, 64},
    mpq_class{45, 64},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    {{168, 1}, {169, 1}, {170, 2}, {171, 1}, {172, 1}, {173, 2}, {174, 1}, {175, 1}, {176, 1}, {177, 2}, {178, 1}, {179, 1}, {180, 2}, {181, 1}, {182, 1}, {183, 1}, {184, 2}, {185, 1}, {186, 1}, {187, 2}, {188, 1}, {189, 1}, {190, 1}, {191, 2}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    false,
    true,
    false};

const Move EARTHQUAKE{
    "Earthquake",
    mpq_class{255, 256},
    mpq_class{1, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{74, 1}, {75, 3}, {76, 3}, {77, 3}, {78, 2}, {79, 3}, {80, 3}, {81, 3}, {82, 3}, {83, 3}, {84, 3}, {85, 3}, {86, 3}, {87, 2}, {88, 1}},
    {{144, 1}, {145, 1}, {146, 2}, {147, 1}, {148, 2}, {149, 1}, {150, 2}, {151, 1}, {152, 2}, {153, 1}, {154, 2}, {155, 1}, {156, 2}, {157, 1}, {158, 2}, {159, 1}, {160, 2}, {161, 1}, {162, 2}, {163, 1}, {164, 2}, {165, 1}, {166, 2}, {167, 1}, {168, 2}, {169, 1}, {170, 1}},
    {{38, 4}, {39, 6}, {40, 6}, {41, 5}, {42, 6}, {43, 6}, {44, 4}, {45, 1}},
    false,
    false,
    false};

const Move FIRE_BLAST{
    "Fire Blast",
    mpq_class{27, 32},
    mpq_class{5, 32},
    mpq_class{19, 64},
    mpq_class{45, 64},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    {{168, 1}, {169, 1}, {170, 2}, {171, 1}, {172, 1}, {173, 2}, {174, 1}, {175, 1}, {176, 1}, {177, 2}, {178, 1}, {179, 1}, {180, 2}, {181, 1}, {182, 1}, {183, 1}, {184, 2}, {185, 1}, {186, 1}, {187, 2}, {188, 1}, {189, 1}, {190, 1}, {191, 2}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    false,
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
    {{0, 39}},
    false,
    false,
    false};

const std::array<const Move *, 5> MOVES{
    &BODY_SLAM,
    &HYPER_BEAM,
    &BLIZZARD,
    &FIRE_BLAST,
    &RECHARGE};

struct State
{
    HP_T hp_1;
    HP_T hp_2;
    int burned_1;
    int burned_2;
    int recharge_1;
    int recharge_2;

    State() {}

    State(
        const HP_T hp_1,
        const HP_T hp_2,
        const int burned_1,
        const int burned_2,
        const int recharge_1,
        const int recharge_2)
        : hp_1{hp_1}, hp_2{hp_2},
          burned_1{burned_1}, burned_2{burned_2},
          recharge_1{recharge_1}, recharge_2{recharge_2}
    {
    }

    bool operator==(const State &other) const
    {
        return (hp_1 == other.hp_1) &&
               (hp_2 == other.hp_2) &&
               (burned_1 == other.burned_1) &&
               (burned_2 == other.burned_2) &&
               (recharge_1 == other.recharge_1) &&
               (recharge_2 == other.recharge_2);
    }
};

void print_state(
    const State &state)
{
    std::cout << state.hp_1 << ' ' << state.hp_2 << ' ' << state.burned_1 << ' ' << state.burned_2 << ' ' << state.recharge_1 << ' ' << state.recharge_2 << ' ' << std::endl;
}

struct SolutionEntry
{
    mpq_class value;
    float p1_strategy[4];
    float p2_strategy[4];
};

// using Solution = SolutionEntry[MAX_HP][MAX_HP][3];

struct Solution
{
    SolutionEntry data[MAX_HP][MAX_HP][2][2][3];
};

void init_tables(
    Solution &tables)
{
    for (int hp_1 = 1; hp_1 <= BODY_SLAM.rolls[0].dmg; ++hp_1)
    {
        for (int hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            SolutionEntry *entries = tables.data[hp_1 - 1][hp_2 - 1][0][0];
            entries[0].value = mpq_class{1, 2};
            entries[1].value = mpq_class{1, 512};
            entries[2].value = mpq_class{511, 512};
        }
    }
}

SolutionEntry &get_entry(
    Solution &tables,
    const State &state)
{
    int x;
    if (state.hp_1 < state.hp_2)
    {
        x = (state.recharge_1 << 1) + (state.recharge_2 << 0);
        return tables.data[state.hp_2 - 1][state.hp_1 - 1][state.burned_2][state.burned_1][x % 3];
    }
    else
    {
        int x = (state.recharge_1 << 0) + (state.recharge_2 << 1);
        return tables.data[state.hp_1 - 1][state.hp_2 - 1][state.burned_1][state.burned_2][x % 3];
    }
}

const SolutionEntry &get_entry(
    const Solution &tables,
    const State &state)
{
    int x;
    if (state.hp_1 < state.hp_2)
    {
        x = (state.recharge_1 << 1) + (state.recharge_2 << 0);
        return tables.data[state.hp_2 - 1][state.hp_1 - 1][state.burned_2][state.burned_1][x % 3];
    }
    else
    {
        int x = (state.recharge_1 << 0) + (state.recharge_2 << 1);
        return tables.data[state.hp_1 - 1][state.hp_2 - 1][state.burned_1][state.burned_2][x % 3];
    }
}

mpq_class lookup_value(
    const Solution &tables,
    const State &state)
{
    const SolutionEntry &entry = get_entry(tables, state);

    if (state.hp_1 < state.hp_2)
    {
        mpq_class answer = mpq_class{1} - entry.value;
        answer.canonicalize();
        return answer;
    }
    else
    {
        return entry.value;
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
    mpq_class reflexive_prob{0};
    mpq_class total_prob{0};
    mpq_class total_prob_no_rolls{0};

    for (int i = 0; i < 128; ++i)
    {
        // These booleans are all ordered w.r.t. turn. hit_1 is whether the first attack of the turn goes off, hit_2 the second
        // finally flipped is weather the first attack is from Player 1 vs Player 2. P1/P2 is rougly speaking, the order of args/params
        const bool hit_1 = i & 1;
        const bool hit_2 = i & 2;
        const bool crit_1 = i & 4;
        const bool crit_2 = i & 8;
        const bool proc_1 = i & 16;
        const bool proc_2 = i & 32;
        const bool flipped = i & 64;

        // also in turn order
        HP_T t1_hp = flipped ? state.hp_2 : state.hp_1;
        HP_T t2_hp = flipped ? state.hp_1 : state.hp_2;
        const int t1_already_burned = flipped ? state.burned_2 : state.burned_1;
        const int t2_already_burned = flipped ? state.burned_1 : state.burned_2;

        const Move &m1 = *MOVES[flipped ? move_2_idx : move_1_idx];
        const Move &m2 = *MOVES[flipped ? move_1_idx : move_2_idx];

        const bool frz_t1 = hit_1 && proc_1 && m1.may_freeze;
        const bool frz_t2 = hit_2 && proc_2 && m2.may_freeze;
        const bool brn_t1 = hit_1 && proc_1 && m1.may_burn;
        const bool brn_t2 = hit_2 && proc_2 && m2.may_burn;

        // const bool flinched_t2 = hit_1 && proc_1 && m1.may_flinch;

        mpq_class t1_prob_no_roll =
            mpq_class{1, 2} *
            (hit_1 ? m1.acc : m1.one_minus_acc) * (crit_1 ? CRIT : NO_CRIT) * (proc_1 ? m1.eff : m1.one_minus_eff) *
            (hit_2 ? m2.acc : m2.one_minus_acc) * (crit_2 ? CRIT : NO_CRIT) * (proc_2 ? m2.eff : m2.one_minus_eff);
        t1_prob_no_roll.canonicalize();

        total_prob_no_rolls += t1_prob_no_roll;
        total_prob_no_rolls.canonicalize();

        if (frz_t1)
        {
            if (!flipped)
            {
                value += t1_prob_no_roll;
                value.canonicalize();
            }
            else
            {
            }
            total_prob += t1_prob_no_roll;
            total_prob.canonicalize();
            continue;
        }

        const std::vector<Roll> &rolls_t1 = hit_1 ? (crit_1 ? m1.crit_rolls : (t1_already_burned ? m1.burned_rolls : m1.rolls)) : RECHARGE.rolls;
        const std::vector<Roll> &rolls_t2 = hit_2 ? (crit_2 ? m2.crit_rolls : ((t2_already_burned || brn_t1) ? m2.burned_rolls : m2.rolls)) : RECHARGE.rolls;

        for (const Roll roll_1 : rolls_t1)
        {
            mpq_class t1_roll_prob = t1_prob_no_roll * mpq_class{roll_1.n, 39};
            t1_roll_prob.canonicalize();

            t2_hp -= (hit_1 ? roll_1.dmg : 0);
            if (t2_hp <= 0)
            {
                if (!flipped)
                {
                    value += t1_roll_prob;
                    value.canonicalize();
                }
                else
                {
                }
                total_prob += t1_roll_prob;
                total_prob.canonicalize();
                continue;
            }

            // brn damage
            t1_hp -= (t1_already_burned ? BURN_DMG : 0);
            if (t1_hp <= 0)
            {
                if (!flipped)
                {
                }
                else
                {
                    value += t1_roll_prob;
                    value.canonicalize();
                }
                total_prob += t1_roll_prob;
                total_prob.canonicalize();
                continue;
            }

            if (frz_t2)
            {
                if (!flipped)
                {
                }
                else
                {
                    value += t1_roll_prob;
                    value.canonicalize();
                }
                total_prob += t1_roll_prob;
                total_prob.canonicalize();
                continue;
            }

            for (const Roll roll_2 : rolls_t2)
            {
                mpq_class t2_roll_prob = t1_roll_prob * mpq_class{roll_2.n, 39};
                t2_roll_prob.canonicalize();

                total_prob += t2_roll_prob;
                total_prob.canonicalize();

                t1_hp -= (hit_2 ? roll_2.dmg : 0);
                if (t1_hp <= 0)
                {
                    if (!flipped)
                    {
                    }
                    else
                    {
                        value += t2_roll_prob;
                        value.canonicalize();
                    }
                    continue;
                }

                // brn damage
                t2_hp -= ((t2_already_burned || brn_t1) ? BURN_DMG : 0);
                if (t2_hp <= 0)
                {
                    if (!flipped)
                    {
                        value += t2_roll_prob;
                        value.canonicalize();
                    }
                    else
                    {
                    }
                    value.canonicalize();
                    continue;
                }

                // No KOs or freeze

                State child;
                if (!flipped)
                {
                    child = State{
                        t1_hp,
                        t2_hp,
                        t1_already_burned || brn_t2,
                        t2_already_burned || brn_t1,
                        hit_1 && m1.must_recharge,
                        hit_2 && m2.must_recharge};
                }
                else
                {
                    child = State{
                        t2_hp,
                        t1_hp,
                        t2_already_burned || brn_t1,
                        t1_already_burned || brn_t2,
                        hit_2 && m2.must_recharge,
                        hit_1 && m1.must_recharge};
                }

                if (child != state)
                {
                    mpq_class lv = lookup_value(tables, child);
                    if (lv == mpq_class{0})
                    {
                        std::cout << '!' << std::endl;
                        print_state(state);
                        print_state(child);
                        // std::cout << move_1.id << ' ' << move_2.id << std::endl;
                        assert(false);
                        exit(1);
                    }
                    const mpq_class weighted_solved_value = t2_roll_prob * lv;
                    value += weighted_solved_value;
                    value.canonicalize();
                }
                else
                {
                    reflexive_prob += t2_roll_prob;
                    reflexive_prob.canonicalize();
                }
            }
        }
    }

    if (total_prob != mpq_class(1))
    {
        std::cout << total_prob.get_str() << std::endl;
        // std::cout << move_1.id << ' ' << move_2.id << std::endl;
        assert(false);
        exit(1);
    }

    if (reflexive_prob > mpq_class{0})
    {
        if (state.recharge_1 || state.recharge_2 || state.burned_1 || state.burned_2)
        {
            std::cout << "reflexive assert fail" << std::endl;
            std::cout << reflexive_prob.get_str() << std::endl;
            // std::cout << move_1.id << ' ' << move_2.id << std::endl;
            assert(false);
            exit(1);
        }
        // only S00 should have this
    }
    mpq_class real_value = value / (mpq_class{1} - reflexive_prob);
    real_value.canonicalize();

    // if (state.hp_1 == state.hp_2)
    // {
    //     if ( == m2.id)
    //     {
    //         if (x != mpq_class{1, 2})
    //         {
    //             std::cout << "mirror q fail" << std::endl;
    //             std::cout << move_1.id << ' ' << move_2.id << std::endl;
    //             std::cout << x.get_str() << std::endl;
    //             assert(false);
    //             exit(1);
    //         }
    //     }
    // }

    return real_value;
}
void solve_state(
    Solution &tables,
    const State &state)
{
    // pinyon ftw!!!
    using Types = DefaultTypes<mpq_class, int, int, mpq_class, ConstantSum<1, 1>::Value>;

    const std::vector<int> legal0 = {0, 1, 2, 3};
    const std::vector<int> legal1 = {4};

    // get legal moves
    std::vector<int> p1_legal_moves = (state.recharge_1 > 0) ? legal1 : legal0;
    std::vector<int> p2_legal_moves = (state.recharge_2 > 0) ? legal1 : legal0;
    const size_t rows = p1_legal_moves.size();
    const size_t cols = p2_legal_moves.size();

    // fill payoff matrix
    Types::MatrixValue payoff_matrix{rows, cols};

    for (int row_idx = 0; row_idx < rows; ++row_idx)
    {
        for (int col_idx = 0; col_idx < cols; ++col_idx)
        {
            payoff_matrix.get(row_idx, col_idx) =
                Types::Value{q_value(tables, state, p1_legal_moves[row_idx], p2_legal_moves[col_idx])};
        }
    }

    // solve

    std::cout << "SOLVING, PRINTING STATE AND PAYOFF MATRIX" << std::endl;
    print_state(state);
    payoff_matrix.print();
    std::cout << std::endl;

    Types::VectorReal row_strategy{rows};
    Types::VectorReal col_strategy{cols};

    auto value = LRSNash::solve(payoff_matrix, row_strategy, col_strategy);
    SolutionEntry &entry = get_entry(tables, state);
    entry.value = value.get_row_value().get();

    for (int row_idx = 0; row_idx < rows; ++row_idx)
    {
        entry.p1_strategy[row_idx] = row_strategy[row_idx].get().get_d();
    }

    for (int col_idx = 0; col_idx < cols; ++col_idx)
    {
        entry.p2_strategy[col_idx] = col_strategy[col_idx].get().get_d();
    }
}

void total_solve(
    Solution &tables)
{
    // const int last_save = BODY_SLAM.rolls[0].dmg;
    const int last_save = 0;
    const int new_save = MAX_HP;

    for (uint16_t hp_1 = last_save + 1; hp_1 <= new_save; ++hp_1)
    {
        for (uint16_t hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            for (int b = 0; b < 4; ++b)
            {
                const int burned_1 = (b & 1) >> 0;
                const int burned_2 = (b & 2) >> 1;

                // Solve

                const State state_00{hp_1, hp_2, burned_1, burned_2, false, false};
                const State state_01{hp_1, hp_2, burned_1, burned_2, true, false};
                const State state_10{hp_1, hp_2, burned_1, burned_2, false, true};

                solve_state(tables, state_00);
                solve_state(tables, state_01);
                solve_state(tables, state_10);

                SolutionEntry *entries = tables.data[hp_1 - 1][hp_2 - 1][burned_1][burned_2];
                if ((hp_1 == hp_2) && (burned_1 == 0) && (burned_2 == 0))
                {
                    if ((entries[0].value != mpq_class{1, 2}))
                    {
                        std::cout << "s00 not 1/2 for same hp" << std::endl;
                        assert(false);
                        exit(1);
                    }
                    if (entries[1].value + entries[2].value != mpq_class{1})
                    {
                        std::cout << "s01 doesnt mirror s10" << std::endl;
                        assert(false);
                        exit(1);
                    }
                }

                // progress report
                {
                    for (int r = 0; r < 3; ++r)
                    {
                        const State state{hp_1, hp_2, burned_1, burned_2, r % 2, r / 2};
                        print_state(state);
                        std::cout << "VALUE: " << entries[r].value.get_d() << " = " << entries[r].value.get_str() << std::endl;
                        std::cout << "STRATEGIES:" << std::endl;
                        for (int i = 0; i < 4; ++i)
                        {
                            std::cout << entries[r].p1_strategy[i] << ' ';
                        }
                        std::cout << std::endl;
                        for (int i = 0; i < 4; ++i)
                        {
                            std::cout << entries[r].p2_strategy[i] << ' ';
                        }
                        std::cout << std::endl;
                    }
                };

                // file output
                {
                    for (int r = 0; r < 3; ++r)
                    {
                        const State state{hp_1, hp_2, burned_1, burned_2, r % 2, r / 2};
                        // print_state(state);
                        OUTPUT_FILE << "STATE: " << state.hp_1 << ' ' << state.hp_2 << ' ' << state.recharge_1 << ' ' << state.recharge_2 << std::endl;
                        OUTPUT_FILE << "VALUE: " << entries[r].value.get_d() << " = " << entries[r].value.get_str() << std::endl;
                        OUTPUT_FILE << "STRATEGIES:" << std::endl;
                        for (int i = 0; i < 4; ++i)
                        {
                            OUTPUT_FILE << entries[r].p1_strategy[i] << ' ';
                        }
                        OUTPUT_FILE << std::endl;
                        for (int i = 0; i < 4; ++i)
                        {
                            OUTPUT_FILE << entries[r].p2_strategy[i] << ' ';
                        }
                        OUTPUT_FILE << std::endl;
                    }
                };
            }
        }
    }
}

void move_rolls_assert()
{
    for (const Move *move : MOVES)
    {
        int a = 0;
        int b = 0;
        int c = 0;
        for (const auto roll : move->rolls)
        {
            a += roll.n;
        }
        for (const auto roll : move->crit_rolls)
        {
            b += roll.n;
        }
        for (const auto roll : move->burned_rolls)
        {
            c += roll.n;
        }
        if ((a != 39) || (b != 39) || (c != 39))
        {
            assert(false);
            exit(1);
        }
    }
}

int main()
{
    move_rolls_assert();

    Solution *tables_ptr = new Solution();
    Solution &tables = *tables_ptr;
    // init_tables(tables);

    const size_t table_size_bytes = sizeof(tables);
    std::cout << "SOLUTION TABLE SIZE (MB): " << (table_size_bytes >> 20) << std::endl
              << std::endl;

    total_solve(tables);

    return 0;
}