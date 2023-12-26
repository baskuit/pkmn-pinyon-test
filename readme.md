This repo is for testing the linkage between `pinyon` and `pkmn/engine`.

`export NODE_PATH=$NODE_PATH:/usr/local/lib/node_modules`

`sudo pkmn-debug stream.txt > index.html`

`zig build -Dlog -Dchance -Dcalc -Doptimize=Debug -Dpic -Dshowdown`

`sudo npm install @pkmn/engine@dev -g`

As a reminder to myself:

* This branch uses the pkmn-debug branch of Pinyon
* Pinyon itself uses the 'surskit' branch of lrsnash
    I will eventually have to clean up all the various branches and decide how I want to build the library. I would like to have access to both gmp and 64/128 bit versions of the solver, built at once? Or do you decide? See its not that easy
* Casting the `chance_actions` bytes as a reference to `std::array<uint8_t, 16?>` does work, so there should be no extra copying of the bytes to a `obs` member
