#pragma once

#include <pinyon.hh>
#include <pkmn.h>

#include "./sides.hh"

#include <fstream>
#include <filesystem>

using TypeList = DefaultTypes<
    float,
    pkmn_choice,
    std::array<uint8_t, 16>,
    float,
    ConstantSum<1, 1>::Value,
    A<9>::Array>;

template <bool use_debug_log = false>
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
        pkmn_gen1_chance_options chance_options{};
        pkmn_rational *p{};
        pkmn_gen1_calc_options calc_options{};

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
            // options stuff
            log_options = {log.data(), 64};
            pkmn_rational_init(&chance_options.probability);
            pkmn_gen1_battle_options_set(&options, &log_options, &chance_options, &calc_options);
            p = pkmn_gen1_battle_options_chance_probability(&options);
            get_actions();

            // setup debug log. this part probably uses the wrong seed since its called before randomize_transition
            if constexpr (use_debug_log)
            {
                debug_log.push_back(uint8_t{1});
                debug_log.push_back(uint8_t{1});
                debug_log.push_back(uint8_t{64});
                debug_log.push_back(uint8_t{0});
                for (int i = 0; i < 384; ++i)
                {
                    debug_log.push_back(battle.bytes[i]);
                }
            }
        }

        State(const State &other)
        {
            this->row_actions = other.row_actions;
            this->col_actions = other.col_actions;
            this->terminal = other.terminal;
            clamp = other.clamp;
            result = other.result;
            memcpy(battle.bytes, other.battle.bytes, 384);
            options = other.options;
            // memcpy(options.bytes, other.options.bytes, PKMN_GEN1_BATTLE_OPTIONS_SIZE);
            log_options = {log.data(), 64};
            pkmn_gen1_battle_options_set(&options, &log_options, NULL, NULL);
            p = pkmn_gen1_battle_options_chance_probability(&options);
            if constexpr (use_debug_log)
            {
                debug_log = other.debug_log;
            }
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
            TypeList::VectorAction &col_actions) const
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
            if (clamp)
            {
                calc_options.overrides.bytes[0] = 217 + 19 * (battle.bytes[383] % 3);
                calc_options.overrides.bytes[8] = 217 + 19 * (battle.bytes[382] % 3);
                pkmn_gen1_battle_options_set(&options, NULL, NULL, &calc_options);
            }
            else
            {
                pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
            }

            result = pkmn_gen1_battle_update(&battle, row_action.get(), col_action.get(), &options);

            this->prob = typename TypeList::Prob{static_cast<float>(
                pkmn_rational_numerator(p) / pkmn_rational_denominator(p))};

            pkmn_gen1_chance_actions *chance_ptr = pkmn_gen1_battle_options_chance_actions(&options);
            memcpy(this->obs.get().data(), chance_ptr->bytes, 16);

            if (clamp)
            {
                const Obs &obs_ref = this->obs;
                if (obs_ref.get()[1] & 2 && obs_ref.get()[0])
                {
                    this->prob *= static_cast<typename TypeList::Prob>(
                        typename TypeList::Q{13, 1});
                }
                if (obs_ref.get()[9] & 2 && obs_ref.get()[8])
                {
                    this->prob *= static_cast<typename TypeList::Prob>(
                        typename TypeList::Q{13, 1});
                }
            }

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

            if constexpr (use_debug_log)
            {
                for (int i = 0; i < 64; ++i)
                {
                    debug_log.push_back(log[i]);
                }
                for (int i = 0; i < 384; ++i)
                {
                    debug_log.push_back(battle.bytes[i]);
                }
                debug_log.push_back(result);
                debug_log.push_back(static_cast<uint8_t>(row_action));
                debug_log.push_back(static_cast<uint8_t>(col_action));
            }
        }

        // const Obs &get_obs() const
        // {
        //     auto *ptr = pkmn_gen1_battle_options_chance_actions(&options)->bytes;
        //     const Obs &obs_ref = *reinterpret_cast<std::array<uint8_t, 16> *>(ptr);
        //     BattleTypes::Obs obs = obs_ref;
        //     return obs;
        // }

        void randomize_transition(TypeList::PRNG &device)
        {
            uint8_t *battle_prng_bytes = battle.bytes + n_bytes_battle;
            *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = device.uniform_64();
        }

        void randomize_transition(const TypeList::Seed seed)
        {
            uint8_t *battle_prng_bytes = battle.bytes + n_bytes_battle;
            *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = seed;
        }

        void save_debug_log(const std::filesystem::path path) const
        {
            std::fstream file;
            file.open(path, std::ios::binary | std::ios::app);
            const size_t n = debug_log.size();
            file.write(reinterpret_cast<const char *>(debug_log.data()), n);
            file.close();
        }

        void save_debug_log() const
        {
            const uint8_t *battle_prng_bytes = battle.bytes + n_bytes_battle;
            const uint64_t *seed = reinterpret_cast<const uint64_t *>(battle_prng_bytes);
            const std::string path =
                "/home/user/Desktop/pkmn-pinyon-test/" + std::to_string(*seed) + ".txt";
            std::fstream file;
            file.open(path, std::ios::binary | std::ios::app);
            const size_t n = debug_log.size();
            file.write(reinterpret_cast<const char *>(debug_log.data()), n);
            file.close();
        }
    };
};
