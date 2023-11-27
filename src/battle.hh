#pragma once

#include <pinyon.hh>
#include <pkmn.h>

#include "./sides.hh"

#include <fstream>

using TypeList = DefaultTypes<
    float,
    pkmn_choice,
    std::array<uint8_t, 376>,
    float,
    ConstantSum<1, 1>::Value,
    A<9>::Array>;

struct BattleTypes : TypeList
{

    static const int n_bytes_battle = 376;

    class State : public PerfectInfoState<TypeList>
    {
    public:
        pkmn_gen1_battle battle;
        pkmn_gen1_battle_options options;
        pkmn_result result{}; // previous bugs caused by not initializing libpkmn stuff
        pkmn_result_kind result_kind;
        pkmn_gen1_calc_options calc_options{};
        pkmn_gen1_chance_options chance_options{};
        std::array<uint8_t, 64> log{};
        pkmn_gen1_log_options log_options;
        bool clamp = true;
        bool print_log = false;

        std::vector<uint8_t> debug_log{};

        State(const int row_idx = 0, const int col_idx = 0)
        {
            const auto row_side = sides[row_idx];
            const auto col_side = sides[col_idx];
            memcpy(battle.bytes, row_side, 184);
            memcpy(battle.bytes + 184, col_side, 184);
            memset(battle.bytes + 2 * 184, 0, n_bytes_battle - 2 * 184);
            memset(battle.bytes + n_bytes_battle, 0, 8);
            // pkmn_rational_init(&chance_options.probability);
            // log_options = {log.data(), 64};
            // if (clamp)
            // {
            //     calc_options.overrides.bytes[0] = 217 + 38 * (battle.bytes[383] && 1);
            //     calc_options.overrides.bytes[8] = 217 + 38 * (battle.bytes[382] && 1);
            // }
            pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
            get_actions();

            // setup debug log. this part probably uses the wrong seed since its called before randomize_transition
            // debug_log.push_back(uint8_t{1});
            // debug_log.push_back(uint8_t{1});
            // debug_log.push_back(uint8_t{64});
            // debug_log.push_back(uint8_t{0});
            // for (int i = 0; i < 384; ++i)
            // {
            //     debug_log.push_back(battle.bytes[i]);
            // }
        }

        State(const State &other)
        {
            this->row_actions = other.row_actions;
            this->col_actions = other.col_actions;
            this->terminal = other.terminal;
            memcpy(battle.bytes, other.battle.bytes, 384);
            // pkmn_rational_init(&chance_options.probability);
            // log_options = {log.data(), 64};
            // clamp = other.clamp;
            // debug_log = other.debug_log;
            // if (clamp)
            // {
            //     calc_options.overrides.bytes[0] = 217 + 38 * (battle.bytes[383] && 1);
            //     calc_options.overrides.bytes[8] = 217 + 38 * (battle.bytes[382] && 1);
            // }
            pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
            // get_actions();
        }

        void get_actions()
        {
            this->row_actions.resize(
                pkmn_gen1_battle_choices(
                    &battle,
                    PKMN_PLAYER_P1,
                    pkmn_result_p1(result),
                    reinterpret_cast<pkmn_choice *>(this->row_actions.data()),
                    PKMN_MAX_CHOICES));
            this->col_actions.resize(
                pkmn_gen1_battle_choices(
                    &battle,
                    PKMN_PLAYER_P2,
                    pkmn_result_p2(result),
                    reinterpret_cast<pkmn_choice *>(this->col_actions.data()),
                    PKMN_MAX_CHOICES));
        }

        void get_actions(
            TypeList::VectorAction &row_actions,
            TypeList::VectorAction &col_actions)
        {
            row_actions.resize(
                pkmn_gen1_battle_choices(
                    &battle,
                    PKMN_PLAYER_P1,
                    pkmn_result_p1(result),
                    reinterpret_cast<pkmn_choice *>(row_actions.data()),
                    PKMN_MAX_CHOICES));
            col_actions.resize(
                pkmn_gen1_battle_choices(
                    &battle,
                    PKMN_PLAYER_P2,
                    pkmn_result_p2(result),
                    reinterpret_cast<pkmn_choice *>(col_actions.data()),
                    PKMN_MAX_CHOICES));
        }

        void apply_actions(
            TypeList::Action row_action,
            TypeList::Action col_action)
        {
            // if (print_log)
            // {
            //     std::cout << "last actions " << (int)row_action.get() << ' ' << (int)col_action.get() << std::endl;
            // }

            result = pkmn_gen1_battle_update(&battle, row_action.get(), col_action.get(), &options);
            result_kind = pkmn_result_type(result);
            if (result_kind) [[unlikely]]
            {
                this->terminal = true;
                switch (pkmn_result_type(result))
                {
                case PKMN_RESULT_WIN:
                {
                    this->payoff = TypeList::Value{1.0f};
                    break;
                }
                case PKMN_RESULT_LOSE:
                {
                    this->payoff = TypeList::Value{0.0f};
                    break;
                }
                case PKMN_RESULT_TIE:
                {
                    this->payoff = TypeList::Value{0.5f};
                    break;
                }
                case PKMN_RESULT_ERROR:
                {
                    exit(1);
                }
                }
            }
            else [[likely]]
            {
                // if (clamp)
                // {
                //     calc_options.overrides.bytes[0] = 217 + 38 * (battle.bytes[383] && 1);
                //     calc_options.overrides.bytes[8] = 217 + 38 * (battle.bytes[382] && 1);
                // }
                // memcpy(
                //     this->obs.get().data(),
                //     log.data(),
                //     64);
                // memcpy(
                //     this->obs.get().data(),
                //     pkmn_gen1_battle_options_chance_actions(&options)->bytes,
                //     16);
                memcpy(
                    this->obs.get().data(),
                    battle.bytes,
                    376);
                pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
            }

            // if (print_log)
            // {
            // for (int i = 0; i < 64; ++i)
            // {
            //     debug_log.push_back(log[i]);
            // }
            // for (int i = 0; i < 384; ++i)
            // {
            //     debug_log.push_back(battle.bytes[i]);
            // }
            // debug_log.push_back(result);
            // debug_log.push_back(static_cast<uint8_t>(row_action));
            // debug_log.push_back(static_cast<uint8_t>(col_action));

            // std::string path = "/home/user/Desktop/pkmn-pinyon-test/stream.txt";
            // remove(path.data());
            // std::fstream file;
            // file.open(path, std::ios::binary | std::ios::app);
            // const size_t n = debug_log.size();
            // file.write(reinterpret_cast<char *>(debug_log.data()), n);
            // file.close();
            // }
        }

        const Obs &get_obs() const
        {
            return this->obs;
        }

        void randomize_transition(TypeList::PRNG &device)
        {
            uint8_t *battle_prng_bytes = battle.bytes + n_bytes_battle;
            *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = device.uniform_64();
        }
    };
};
