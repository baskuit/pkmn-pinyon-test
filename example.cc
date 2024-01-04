#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

int main()
{
   pkmn_choice choices[PKMN_CHOICES_SIZE];

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

   pkmn_gen1_calc_options calc_options{};

   pkmn_gen1_battle_options options;
   pkmn_gen1_battle_options_set(&options, NULL, &chance_options, NULL);

   pkmn_gen1_chance_actions* actions = pkmn_gen1_battle_options_chance_actions(&options);
   pkmn_rational* p = pkmn_gen1_battle_options_chance_probability(&options);

   print_chance_actions(actions->bytes);
   printf("PROB: %lf/%lf\n", pkmn_rational_numerator(p), pkmn_rational_denominator(p));

   pkmn_result result;

   pkmn_choice c1 = 0, c2 = 0;

   result = pkmn_gen1_battle_update(
      &battle, 0, 0, &options
   );

   pkmn_gen1_battle_options_set(&options, NULL, NULL, NULL);

   // assert fail

   randomize_transition(battle.bytes, 0);
   result = pkmn_gen1_battle_update(
      &battle, move(2), move(1), &options
   );
   
   print_chance_actions(actions->bytes);
   printf("PROB: %lf/%lf\n", pkmn_rational_numerator(p), pkmn_rational_denominator(p));

   return 0;
}
