#include <gmpxx.h>

#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>

size_t MAX_HP = 353;

struct Roll
{
    int dmg;
    int n;
};

struct Move
{
    mpq_class acc;
    mpq_class one_minus_acc;
    mpq_class frz;
    mpq_class one_minus_frz;

    std::vector<Roll> rolls;
    std::vector<Roll> crit_rolls;
    bool must_recharge;
    bool may_freeze;
};

Move BODY_SLAM{
    mpq_class{255, 256},
    mpq_class{1, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{95, 2}, {96, 2}, {97, 3}, {98, 2}, {99, 2}, {100, 2}, {101, 3}, {102, 2}, {103, 2}, {104, 3}, {105, 2}, {106, 2}, {107, 2}, {108, 3}, {109, 2}, {110, 2}, {111, 2}, {112, 1}},
    {{184, 1}, {185, 1}, {186, 1}, {187, 1}, {188, 2}, {189, 1}, {190, 1}, {191, 1}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}, {199, 2}, {200, 1}, {201, 1}, {202, 1}, {203, 1}, {204, 1}, {205, 2}, {206, 1}, {207, 1}, {208, 1}, {209, 1}, {210, 1}, {211, 2}, {212, 1}, {213, 1}, {214, 1}, {215, 1}, {216, 1}, {217, 1}},
    false,
    false};

Move BLIZZARD{
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{86, 1}, {87, 2}, {88, 3}, {89, 2}, {90, 3}, {91, 2}, {92, 3}, {93, 2}, {94, 3}, {95, 2}, {96, 3}, {97, 2}, {98, 3}, {99, 2}, {100, 3}, {101, 2}, {102, 1}},
    {{168, 1}, {169, 1}, {170, 2}, {171, 1}, {172, 1}, {173, 2}, {174, 1}, {175, 1}, {176, 1}, {177, 2}, {178, 1}, {179, 1}, {180, 2}, {181, 1}, {182, 1}, {183, 1}, {184, 2}, {185, 1}, {186, 1}, {187, 2}, {188, 1}, {189, 1}, {190, 1}, {191, 2}, {192, 1}, {193, 1}, {194, 2}, {195, 1}, {196, 1}, {197, 1}, {198, 1}},
    false,
    true};

Move HYPER_BEAM{
    mpq_class{229, 256},
    mpq_class{27, 256},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{166, 1}, {167, 1}, {168, 1}, {169, 2}, {170, 1}, {171, 1}, {172, 2}, {173, 1}, {174, 1}, {175, 1}, {176, 2}, {177, 1}, {178, 1}, {179, 2}, {180, 1}, {181, 1}, {182, 2}, {183, 1}, {184, 1}, {185, 1}, {186, 2}, {187, 1}, {188, 1}, {189, 2}, {190, 1}, {191, 1}, {192, 2}, {193, 1}, {194, 1}, {195, 1}, {196, 1}},
    {{324, 1}, {325, 1}, {327, 1}, {328, 1}, {330, 1}, {331, 1}, {333, 1}, {334, 1}, {336, 1}, {337, 1}, {339, 1}, {340, 1}, {342, 1}, {343, 1}, {345, 1}, {346, 1}, {348, 1}, {349, 1}, {351, 1}, {352, 1}, {354, 1}, {355, 1}, {357, 1}, {358, 1}, {360, 1}, {361, 1}, {363, 1}, {364, 1}, {366, 1}, {367, 1}, {369, 1}, {370, 1}, {372, 1}, {373, 1}, {375, 1}, {376, 1}, {378, 1}, {379, 1}, {381, 1}},
    true,
    false};

Move RECHARGE{
    mpq_class{256, 256},
    mpq_class{0, 1},
    mpq_class{0, 1},
    mpq_class{1, 1},
    {{0, 39}},
    {{0, 39}},
    false,
    false};

std::unordered_map<int, mpq_class> value_table{};

struct State
{
    int hp_1;
    int hp_2;
    bool r_1;
    bool r_2;

    void transitions(
        const Move &move_1,
        const Move &move_2,
        std::vector<State> &states) const
    {
        for (int i = 0; i < 16; ++i)
        {

            int hit_1 = i & 1;
            int hit_2 = i & 2;
            int proc_1 = i & 4;
            int proc_2 = i & 8;

            mpq_class acc_1 = hit_1 ? move_1.acc : move_1.one_minus_acc;
            mpq_class acc_2 = hit_2 ? move_2.acc : move_2.one_minus_acc;
            mpq_class frz_1 = proc_1 ? move_1.frz : move_1.one_minus_frz;
            mpq_class frz_2 = proc_2 ? move_2.frz : move_2.one_minus_frz;

            mpq_class p = acc_1 * acc_2 * frz_1 * frz_2;
            p.canonicalize();

            bool p1_frz_win = hit_1 && proc_1 && move_1.may_freeze;
            bool p2_frz_win = hit_2 && proc_2 && move_2.may_freeze;

            std::cout << p.get_str() << std::endl;

            // check if frz win

            if (p1_frz_win)
            {
                if (p2_frz_win)
                {
                }
                else
                {
                }
                continue;
            }
            if (p2_frz_win)
            {
                continue;
            }

            for (int j = 0; j < 4; ++j)
            {
                int crit_1 = j & 1;
                int crit_2 = j & 2;

                mpq_class crit_p_1 = crit_1 ? mpq_class{110 / 512} : mpq_class{402, 512};
                mpq_class crit_p_2 = crit_2 ? mpq_class{110 / 512} : mpq_class{402, 512};

                const std::vector<Roll> &rolls_1 = crit_1 ? move_1.rolls : move_1.crit_rolls;
                const std::vector<Roll> &rolls_2 = crit_2 ? move_2.rolls : move_2.crit_rolls;

                for (const Roll &roll_1 : rolls_1)
                {
                    for (const Roll &roll_2 : rolls_2)
                    {
                        mpq_class roll_probs{roll_1.n * roll_2.n, 39 * 39};

                        int post_hp_1 = std::max(hp_1 - roll_2.dmg * hit_2, 0);
                        int post_hp_2 = std::max(hp_2 - roll_1.dmg * hit_1, 0);

                        bool p1_ko_win = post_hp_2 == 0;
                        bool p2_ko_win = post_hp_1 == 0;

                        if (p1_ko_win)
                        {
                            if (p2_ko_win)
                            {
                            }
                            else
                            {
                            }
                            continue;
                        }
                        if (p2_ko_win)
                        {
                            continue;
                        }

                        // From this point on, there is no win

                        State next{post_hp_1, post_hp_2, move_1.must_recharge, move_2.must_recharge};
                        states.push_back(next);
                    }
                }
            }
        }
    }

    void print() const
    {
        std::cout << hp_1 << ' ' << hp_2 << ' ' << r_1 << ' ' << r_2 << std::endl;
    }

    operator size_t() const
    {
        return 4 * MAX_HP * hp_1 + 4 * hp_2 + 2 * r_1 + r_1;
    }
};

void bootstrap(

)
{
    for (int hp_1 = 1; hp_1 <= MAX_HP; ++hp_1)
    {
        for (int hp_2 = 1; hp_2 <= hp_1; ++hp_2)
        {
            solve_hp(hp_1, hp_2);
        }
    }
}


void solve_hp (
    int hp_1,
    int hp_2
) {}

int main()
{

    State init{353, 353, false, false};

    std::vector<State> next{};

    init.transitions(
        BODY_SLAM, BODY_SLAM, next);

    std::cout << "next states:" << std::endl;

    for (const auto x : next)
    {
        x.print();
    }

    return 0;
}