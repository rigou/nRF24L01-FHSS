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

#pragma once

#include <stdint.h>

#define RNGLIB_NAME	"Rng" // spaces not permitted
#define RNGLIB_VERSION	"v1.1.2"

class rgRng {
    private:
        struct xorwow_state {
            uint32_t x[5];
            uint32_t counter;
        } Xorwow_State;
        
        uint32_t xorwow(struct xorwow_state *state);
    
    public:
        rgRng(void);
        void Seed(uint32_t this_seed);
        uint32_t Next(uint32_t max_value=0);
};
