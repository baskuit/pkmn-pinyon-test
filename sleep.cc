#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <pkmn.h>

#include "./src/print.hh"

void randomize_transition(uint8_t *battle, const uint64_t seed)
{
   uint8_t *battle_prng_bytes = battle + 376;
   *(reinterpret_cast<uint64_t *>(battle_prng_bytes)) = seed;
}

pkmn_choice move(int x)
{
   return (x << 2) | 1;
}

pkmn_choice switch_(int x)
{
   return (x << 2) | 2;
}

int foo(
   const int seed = 0
)
{
   pkmn_result result;

   pkmn_gen1_battle battle = {{77, 1, 198, 0, 168, 0, 32, 1, 32, 1, 59, 7, 94, 15, 142, 16, 156, 16, 77, 1, 0, 124, 205, 100, 57, 1, 198, 0, 188, 0, 82, 1, 112, 1, 69, 32, 86, 31, 94, 16, 105, 32, 57, 1, 64, 65, 204, 100, 47, 1, 32, 1, 202, 1, 238, 0, 12, 1, 59, 8, 128, 16, 153, 8, 156, 16, 47, 1, 0, 91, 217, 100, 191, 2, 108, 0, 108, 0, 198, 0, 52, 1, 58, 16, 85, 23, 86, 32, 135, 16, 191, 2, 0, 113, 0, 100, 97, 1, 42, 1, 32, 1, 62, 1, 238, 0, 34, 24, 59, 8, 63, 8, 89, 16, 97, 1, 0, 128, 0, 100, 11, 2, 62, 1, 228, 0, 158, 0, 228, 0, 34, 24, 115, 31, 120, 8, 156, 16, 11, 2, 0, 143, 0, 100, 191, 2, 108, 0, 108, 0, 198, 0, 52, 1, 113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 16, 85, 23, 86, 32, 135, 16, 4, 2, 3, 1, 5, 6, 59, 0, 191, 2, 108, 0, 108, 0, 198, 0, 52, 1, 47, 24, 69, 32, 86, 32, 135, 16, 191, 2, 0, 113, 0, 100, 157, 1, 102, 1, 82, 1, 178, 0, 188, 0, 34, 24, 89, 16, 157, 16, 164, 15, 162, 0, 0, 112, 84, 100, 67, 1, 248, 0, 12, 1, 72, 1, 42, 1, 57, 24, 85, 24, 86, 32, 105, 32, 67, 1, 0, 121, 201, 100, 97, 1, 42, 1, 32, 1, 62, 1, 238, 0, 34, 24, 59, 8, 63, 8, 89, 16, 97, 1, 0, 128, 0, 100, 11, 2, 62, 1, 228, 0, 158, 0, 228, 0, 34, 24, 89, 16, 115, 32, 156, 15, 11, 2, 129, 143, 0, 100, 127, 1, 22, 1, 12, 1, 42, 1, 92, 1, 65, 32, 85, 24, 86, 31, 97, 47, 127, 1, 64, 145, 43, 100, 191, 2, 108, 0, 108, 0, 198, 0, 52, 1, 113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 47, 24, 69, 32, 86, 32, 135, 16, 1, 5, 3, 4, 2, 6, 115, 0, 13, 0, 164, 0, 1, 0, 1, 0, 69, 6, 145, 169, 199, 235, 170, 192}};
   // 85 tbolt, 95 hypno, 101 nightshade, 15x explosion

   battle.bytes[383] = uint8_t{seed};

   uint8_t log[64];

   pkmn_gen1_log_options log_options{log, 64};

   float64_t num, den, prob;

   pkmn_gen1_battle_options options{};
   pkmn_gen1_battle_options_set(&options, &log_options, NULL, NULL);

   pkmn_gen1_chance_actions *actions = pkmn_gen1_battle_options_chance_actions(&options);
   pkmn_rational *p = pkmn_gen1_battle_options_chance_probability(&options);

   result = pkmn_gen1_battle_update(
       &battle, switch_(2), move(1), &options);

   num = pkmn_rational_numerator(p);
   den = pkmn_rational_denominator(p);
   prob = num / den;
   printf("PROB: %lf/%lf = %lf\n", num, den, prob);

   return 0;
}

int main()
{

   const int tries = 1 << 10;

   for (int t = 0; t < tries; ++t)
   {
      foo(t);
   }
}