#include <gmpxx.h>

#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>

const size_t MAX_HP = 353;
// after this point, we calculate instead of using analytic solution.
// may improve but currently stops when out of body slam range for both players
const size_t MIN_HP = 0;

const mpq_class CRIT{55 / 256};
const mpq_class NO_CRIT{201 / 256};

struct Roll
{
    int dmg;
    int n;
};

struct Move
{
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
    mpq_class{255, 256},
    mpq_class{1, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{95, 2}, {96, 2}, {97, 3}, {98, 2}, {99, 2}, {100, 2}, {101, 3}, {102, 2}, {103, 2}, {104, 3}, {105, 2}, {106, 2}, {107, 2}, {108, 3}, {109, 2}, {110, 2}, {111, 2}, {112, 1}},
    {{184, 1}, {185, 1}, {186, 1}, {187, 1}, {188, 2}, {189, 1}, {190, 1}, {191, 1}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}, {199, 2}, {200, 1}, {201, 1}, {202, 1}, {203, 1}, {204, 1}, {205, 2}, {206, 1}, {207, 1}, {208, 1}, {209, 1}, {210, 1}, {211, 2}, {212, 1}, {213, 1}, {214, 1}, {215, 1}, {216, 1}, {217, 1}},
    false,
    false};

const Move BLIZZARD{
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    {{168, 1}, {169, 1}, {170, 2}, {171, 1}, {172, 1}, {173, 2}, {174, 1}, {175, 1}, {176, 1}, {177, 2}, {178, 1}, {179, 1}, {180, 2}, {181, 1}, {182, 1}, {183, 1}, {184, 2}, {185, 1}, {186, 1}, {187, 2}, {188, 1}, {189, 1}, {190, 1}, {191, 2}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}},
    false,
    true};

const Move HYPER_BEAM{
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{166, 1}, {167, 1}, {168, 1}, {169, 2}, {170, 1}, {171, 1}, {172, 2}, {173, 1}, {174, 1}, {175, 1}, {176, 2}, {177, 1}, {178, 1}, {179, 2}, {180, 1}, {181, 1}, {182, 2}, {183, 1}, {184, 1}, {185, 1}, {186, 2}, {187, 1}, {188, 1}, {189, 2}, {190, 1}, {191, 1}, {192, 2}, {193, 1}, {194, 1}, {195, 1}, {196, 1}},
    {{324, 1}, {325, 1}, {327, 1}, {328, 1}, {330, 1}, {331, 1}, {333, 1}, {334, 1}, {336, 1}, {337, 1}, {339, 1}, {340, 1}, {342, 1}, {343, 1}, {345, 1}, {346, 1}, {348, 1}, {349, 1}, {351, 1}, {352, 1}, {354, 1}, {355, 1}, {357, 1}, {358, 1}, {360, 1}, {361, 1}, {363, 1}, {364, 1}, {366, 1}, {367, 1}, {369, 1}, {370, 1}, {372, 1}, {373, 1}, {375, 1}, {376, 1}, {378, 1}, {379, 1}, {381, 1}},
    true,
    false};

// const Move FIRE_BLAST{
//     mpq_class{229, 256},
//     mpq_class{27, 256},
//     mpq_class{0, 1},
//     mpq_class{1, 1},
//     {{166, 1}, {167, 1}, {168, 1}, {169, 2}, {170, 1}, {171, 1}, {172, 2}, {173, 1}, {174, 1}, {175, 1}, {176, 2}, {177, 1}, {178, 1}, {179, 2}, {180, 1}, {181, 1}, {182, 2}, {183, 1}, {184, 1}, {185, 1}, {186, 2}, {187, 1}, {188, 1}, {189, 2}, {190, 1}, {191, 1}, {192, 2}, {193, 1}, {194, 1}, {195, 1}, {196, 1}},
//     {{324, 1}, {325, 1}, {327, 1}, {328, 1}, {330, 1}, {331, 1}, {333, 1}, {334, 1}, {336, 1}, {337, 1}, {339, 1}, {340, 1}, {342, 1}, {343, 1}, {345, 1}, {346, 1}, {348, 1}, {349, 1}, {351, 1}, {352, 1}, {354, 1}, {355, 1}, {357, 1}, {358, 1}, {360, 1}, {361, 1}, {363, 1}, {364, 1}, {366, 1}, {367, 1}, {369, 1}, {370, 1}, {372, 1}, {373, 1}, {375, 1}, {376, 1}, {378, 1}, {379, 1}, {381, 1}},
//     true,
//     false};

// TODO stomp etc lol

const Move RECHARGE{
    mpq_class{256, 256},
    mpq_class{0, 1},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{0, 39}},
    {{0, 39}},
    false,
    false};

std::vector<const Move *> moves{
    &BODY_SLAM,
    &BLIZZARD,
    &HYPER_BEAM,
    &RECHARGE};

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
    const State &state
)
{
    if (state.hp_1 == 0) {
        return {0};
    }
    if (state.hp_2 == 0) {
        return {1};
    }
    print(state);
    if (state.hp_1 < state.hp_2) {
        size_t h = hash(State{state.hp_2, state.hp_1, state.r_2, state.r_2});
        mpq_class v = solution.value.at(h);
        v = mpq_class{1} - v;
        v.canonicalize();
        return v;
    } else {
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
    for (int i = 0; i < 4; ++i)
    {
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
            if (p2_frz_win)
            {
                // speed tie
                p *= mpq_class{1, 2};
                p.canonicalize();
                value += p;
                value.canonicalize();
            }
            else
            {
                // p1 loss, add 0...
            }
            continue;
        }
        if (p2_frz_win)
        {
            value += p;
            value.canonicalize();
            continue;
        }

        for (int j = 0; j < 4; ++j)
        {
            // iterate over a crit checks
            int crit_1 = j & 1;
            int crit_2 = j & 2;

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
                    mpq_class q = p * roll_probs;
                    q.canonicalize();

                    int post_hp_1 = std::max(state.hp_1 - roll_2.dmg * hit_2, 0);
                    int post_hp_2 = std::max(state.hp_2 - roll_1.dmg * hit_1, 0);

                    bool p1_ko_win = post_hp_2 == 0;
                    bool p2_ko_win = post_hp_1 == 0;

                    if (p1_ko_win)
                    {
                        if (p2_ko_win)
                        {
                            q *= mpq_class{1, 2};
                            q.canonicalize();
                            value += q;
                            value.canonicalize();
                        }
                        else
                        {
                            // p1 loss, add 0...
                        }
                    }
                    if (p2_ko_win)
                    {
                        value += q;
                        value.canonicalize();
                        continue;
                    }

                    const State child{post_hp_1, post_hp_2, move_1.must_recharge, move_2.must_recharge};
                    const size_t child_hash = hash(child);
                    if ((post_hp_1 == state.hp_1) && (post_hp_2 == state.hp_2))
                    {
                        branches.push_back({child, p});
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
}

void solve_hp(
    Solution &tables,
    const int hp_1,
    const int hp_2)
{
    // what simple equation does a joint action applied to a state induce?
    for (const Move *move_1 : moves)
    {
        for (const Move *move_2 : moves)
        {
            for (int i = 0; i < 4; ++i)
            {
                int recharge_1 = i & 1;
                int recharge_2 = i & 2;

                if ((move_1 != &RECHARGE && recharge_1) || (move_2 != &RECHARGE && recharge_2))
                {
                    continue;
                }

                // move has to miss
                mpq_class miss = move_1->one_minus_acc * move_2->one_minus_acc;
                miss.canonicalize();

                std::vector<Branch> branches{};
                mpq_class value{};
                State state{hp_1, hp_2, recharge_1 > 0, recharge_2 > 0};
                transitions(state, tables, *move_1, *move_2, value, branches);


                mpq_class total_branch_prob{0};
                for (const Branch &branch : branches)
                {
                    total_branch_prob += branch.p;
                    total_branch_prob.canonicalize();
                }


                print(state);
                std::cout << "Move 1: " << move_1 << " Move 2: " << move_2 << std::endl;
                std::cout << "Solved values: " << value.get_d() << std::endl;



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

    using Strategy = Move *[4];

    for (int s = 0; s < 256; ++s)
    {

        int a = s & (3 << 0);
        int b = s & (3 << 2);
        int c = s & (3 << 4);
        int d = s & (3 << 6);

        // every possible joint strategy
    }

    // min max checks
    for (int r = 0; r < 16; ++r)
    {
        for (int c = 0; c < 16; ++c)
        {
        }
    }

    for (int c = 0; c < 16; ++c)
    {
        for (int r = 0; r < 16; ++r)
        {
        }
    }

    // update tables
}

int main()
{
    Solution tables{};

    solve_hp(tables, 1, 1);

    return 0;
}

void old_test () {
    State init{353, 353, false, false};

    std::vector<Branch> branches{};
    std::unordered_map<size_t, mpq_class> branches_{};

    mpq_class value{};

    Solution tables{};

    transitions(
        init,
        tables,
        BODY_SLAM,
        BODY_SLAM,
        value,
        branches);

    std::cout << "next states:" << std::endl;

    for (const auto x : branches)
    {
        print(x.state);
        std::cout << x.p.get_str() << std::endl;
    }
}