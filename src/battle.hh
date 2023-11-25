#pragma once

#include <pinyon.hh>
#include <pkmn.h>

#include "./sides.hh"

const int n_bytes_battle = 376;

using TypeList = DefaultTypes<
    float,
    pkmn_choice,
    bool,
    float,
    ConstantSum<1, 1>::Value,
    A<9>::Array>;

struct BattleTypes : TypeList
{

    class State : public PerfectInfoState<TypeList>
    {
    public:
        pkmn_gen1_battle battle;
        pkmn_gen1_battle_options options;
        pkmn_result result{}; // init so no sporadic panic, probably on first get_actions call
        pkmn_result_kind result_kind;
        pkmn_gen1_calc_options calc_options{};
        pkmn_gen1_chance_options chance_options{};
        std::array<uint8_t, 64> log{};
        pkmn_gen1_log_options log_options;

        State(const int row_idx = 0, const int col_idx = 0)
        {
            // setup bytes
            const auto row_side = sides[row_idx];
            const auto col_side = sides[col_idx];
            memcpy(battle.bytes, row_side, 184);
            memcpy(battle.bytes + 184, col_side, 184);
            memset(battle.bytes + 2 * 184, 0, n_bytes_battle - 2 * 184);
            memset(battle.bytes + n_bytes_battle, 0, 8);
            pkmn_rational_init(&chance_options.probability);
            log_options = {log.data(), 64};
            pkmn_gen1_battle_options_set(&options, &log_options, &chance_options, NULL);
            get_actions();
        }

        State(const State &other)
        {
            this->row_actions = other.row_actions;
            this->col_actions = other.col_actions;
            this->terminal = other.terminal;
            memcpy(battle.bytes, other.battle.bytes, 384);
            pkmn_rational_init(&chance_options.probability);
            log_options = {log.data(), 64};
            pkmn_gen1_battle_options_set(&options, &log_options, &chance_options, NULL);
            get_actions();
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

        void apply_actions(
            TypeList::Action row_action,
            TypeList::Action col_action)
        {
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
                pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
            }
        }

        void randomize_transition(TypeList::PRNG &device)
        {
            uint8_t *battle_prng_bytes = battle.bytes + n_bytes_battle;
            *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = device.uniform_64();
        }
    };
};
