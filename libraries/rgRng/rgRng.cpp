/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
 * 
 * This class is derived from original code found at https://en.wikipedia.org/wiki/Xorshift#xorwow
*/

#include "rgRng.h"

// The Xorwow_State array must be initialized to not be all zero in the first four words
rgRng::rgRng(void) {
    Seed(0);
}

void rgRng::Seed(uint32_t this_seed) {
    if (this_seed==0)
        this_seed=2147483629; // default seed
    for (int idx=0; idx<5; idx++)
        Xorwow_State.x[idx]=this_seed+idx;
    Xorwow_State.counter=0;
}

// return a pseudo-random value in the range 0 / max_value-1
uint32_t rgRng::Next(uint32_t max_value) {
    uint32_t retval=xorwow(&Xorwow_State);
    if (max_value)
        retval=retval%max_value;
    return retval;
}

uint32_t rgRng::xorwow(struct xorwow_state *state) {
    /* Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs" */
    uint32_t t4  = state->x[4];
    uint32_t s0  = state->x[0];  
    // Perform a contrived 32-bit shift
    state->x[4] = state->x[3];
    state->x[3] = state->x[2];
    state->x[2] = state->x[1];
    state->x[1] = s0;
 
    t4 ^= t4 >> 2;
    t4 ^= t4 << 1;
    t4 ^= s0 ^ (s0 << 4);
    state->x[0] = t4;
    state->counter += 362437;
    return t4 + state->counter;
}