#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pkmn.h>

pkmn_choice choose(
   pkmn_gen1_battle *battle,
   pkmn_psrng *random,
   pkmn_player player,
   pkmn_choice_kind request,
   pkmn_choice choices[])
{
   uint8_t n = pkmn_gen1_battle_choices(battle, player, request, choices, PKMN_CHOICES_SIZE);
   // Technically due to Generation I's Transform + Mirror Move/Metronome PP
   // error if the battle contains Pokémon with a combination of Transform,
   // Mirror Move/Metronome, and Disable its possible that there are no
   // available choices (softlock), though this is impossible here given that
   // our example battle involves none of these moves
   assert(n > 0);
   // pkmn_gen1_battle_choices determines what the possible choices are - the
   // simplest way to choose an option here is to just use the PSRNG to pick one
   // at random
   return choices[(uint64_t)pkmn_psrng_next(random) * n / 0x100000000];
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

   uint8_t buf[PKMN_LOGS_SIZE];
   pkmn_gen1_log_options log_options = {.buf = buf, .len = PKMN_LOGS_SIZE};

   pkmn_gen1_chance_options chance_options;
   pkmn_rational_init(&chance_options.probability);

   pkmn_gen1_calc_options calc_options{};

   pkmn_gen1_battle_options options;
   pkmn_gen1_battle_options_set(&options, &log_options, &chance_options, &calc_options);

   pkmn_gen1_chance_actions* actions = pkmn_gen1_battle_options_chance_actions(&options);
   pkmn_rational* p = pkmn_gen1_battle_options_chance_probability(&options);

   printf("PROB: %lf/%lf\n", pkmn_rational_numerator(p), pkmn_rational_denominator(p));

   printf("CHANCE: ");
   for (int i = 0; i < PKMN_GEN1_CHANCE_ACTIONS_SIZE; i++) {
      printf("%d, ", actions->bytes[i]);
   }
   printf("\n");

   pkmn_result result;

   pkmn_choice c1 = 0, c2 = 0;

   result = pkmn_gen1_battle_update(
      &battle, 0, 0, &options
   );

   // set duration bytes to 7, hopefully 
   chance_options.actions.bytes[2] = (7 << 4);

   pkmn_gen1_battle_options_set(&options, NULL, &chance_options, &calc_options);

   // assert fail
   result = pkmn_gen1_battle_update(
      &battle, move(2), move(1), &options
   );

   return 0;
}
