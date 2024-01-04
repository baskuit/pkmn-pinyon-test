#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#include <pkmn.h>

#include "./src/print.hh"

void randomize_transition(uint8_t* battle, const uint64_t seed)
{
   uint8_t *battle_prng_bytes = battle + 376;
   *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = seed;
}

pkmn_choice move(int x) {
   return (x << 2) | 1;
}

pkmn_choice switch_(int x) {
   return (x << 2) | 2;
}

int main()
{
   pkmn_result result;

   pkmn_choice c1 = 0, c2 = 0;

   pkmn_gen1_battle battle = { {

      67, 1, 228, 0, 218, 0, 62, 1, 102, 1, 85, 24, 95, 32, 101, 24, 153, 8, 67, 1, 0, 94, 55, 100,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 0, 0, 0,
      67, 1, 228, 0, 218, 0, 62, 1, 102, 1, 85, 24, 95, 32, 101, 24, 153, 8, 67, 1, 0, 94, 55, 100,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   } };
   // 85 tbolt, 95 hypno, 101 nightshade, 15x explosion

   pkmn_gen1_chance_options chance_options{};
   pkmn_rational_init(&chance_options.probability);
   float64_t num, den, prob;

   pkmn_gen1_calc_options calc_options{};

   pkmn_gen1_battle_options options;
   pkmn_gen1_battle_options_set(&options, NULL, &chance_options, NULL);
   pkmn_gen1_chance_actions* actions = pkmn_gen1_battle_options_chance_actions(&options);
   pkmn_rational* p = pkmn_gen1_battle_options_chance_probability(&options);

   std::cout << "Pass turn -" << std::endl;
   result = pkmn_gen1_battle_update(
      &battle, 0, 0, &options
   );
   print_chance_actions(actions->bytes);
   num = pkmn_rational_numerator(p); den = pkmn_rational_denominator(p); prob = num / den;
   printf("PROB: %lf/%lf = %lf\n\n", num, den, prob);


   // seed = 4 works for sleep hit + don't insta wake
   std::cout << "P1 hit hypno and P2 stay asleep -" << std::endl;
   randomize_transition(battle.bytes, 4);
   pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);
   result = pkmn_gen1_battle_update(
      &battle, move(2), move(3), &options
   );
   // actions = pkmn_gen1_battle_options_chance_actions(&options);
   // p = pkmn_gen1_battle_options_chance_probability(&options);
   print_chance_actions(actions->bytes);
   num = pkmn_rational_numerator(p); den = pkmn_rational_denominator(p); prob = num / den;
   printf("PROB: %lf/%lf = %lf\n\n", num, den, prob);

   for (int over = 0; over < 8; ++over) {
      std::cout << "override: " << over << std::endl;
      calc_options = {};
      calc_options.overrides.bytes[18] = uint8_t{over};

      pkmn_gen1_battle battle_{};
      memcpy(battle_.bytes, battle.bytes, PKMN_GEN1_BATTLE_SIZE);
      pkmn_gen1_battle_options options_{};
      memcpy(options_.bytes, options.bytes, PKMN_GEN1_BATTLE_OPTIONS_SIZE);

      std::cout << "Both Seismic Toss" << std::endl;
      pkmn_gen1_battle_options_set(&options, NULL, NULL, &calc_options);
      result = pkmn_gen1_battle_update(
         &battle, move(3), move(3), &options
      );

      print_chance_actions(actions->bytes);
      num = pkmn_rational_numerator(p); den = pkmn_rational_denominator(p); prob = num / den;
      printf("PROB: %lf/%lf = %lf\n\n", num, den, prob);

      
   }
   return 0;
}
