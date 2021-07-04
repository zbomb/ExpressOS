/*==============================================================
    Axon Kernel Libk - big_math.h
    2021, Zachary Berry
    libk/public/big_math.c
==============================================================*/


#include "big_math.h"


uint64_t const _base = 1ULL << 32;
uint64_t const _maxdiv = (_base - 1 ) * _base + ( _base - 1 );

uint64_t _muldiv64( uint64_t a, uint64_t b, uint64_t c )
{
    /*
        This code is from:
        https://newbedev.com/most-accurate-way-to-do-a-combined-multiply-and-divide-operation-in-64-bit
        TODO: Write our own version for this, preferably in ASM
    */
   // First get the easy thing
    uint64_t res = (a/c) * b + (a%c) * (b/c);
    a %= c;
    b %= c;
    // Are we done?
    if (a == 0 || b == 0)
        return res;
    // Is it easy to compute what remain to be added?
    if (c < _base)
        return res + (a*b/c);
    // Now 0 < a < c, 0 < b < c, c >= 1ULL
    // Normalize
    uint64_t norm = _maxdiv/c;
    c *= norm;
    a *= norm;
    // split into 2 digits
    uint64_t ah = a / _base, al = a % _base;
    uint64_t bh = b / _base, bl = b % _base;
    uint64_t ch = c / _base, cl = c % _base;
    // compute the product
    uint64_t p0 = al*bl;
    uint64_t p1 = p0 / _base + al*bh;
    p0 %= _base;
    uint64_t p2 = p1 / _base + ah*bh;
    p1 = (p1 % _base) + ah * bl;
    p2 += p1 / _base;
    p1 %= _base;
    // p2 holds 2 digits, p1 and p0 one

    // first digit is easy, not null only in case of overflow
    uint64_t q2 = p2 / c;
    p2 = p2 % c;

    // second digit, estimate
    uint64_t q1 = p2 / ch;
    // and now adjust
    uint64_t rhat = p2 % ch;
    // the loop can be unrolled, it will be executed at most twice for
    // even bases -- three times for odd one -- due to the normalisation above
    while (q1 >= _base || (rhat < _base && q1*cl > rhat*_base+p1)) {
        q1--;
        rhat += ch;
    }
    // subtract 
    p1 = ((p2 % _base) * _base + p1) - q1 * cl;
    p2 = (p2 / _base * _base + p1 / _base) - q1 * ch;
    p1 = p1 % _base + (p2 % _base) * _base;

    // now p1 hold 2 digits, p0 one and p2 is to be ignored
    uint64_t q0 = p1 / ch;
    rhat = p1 % ch;
    while (q0 >= _base || (rhat < _base && q0*cl > rhat*_base+p0)) {
        q0--;
        rhat += ch;
    }
    // we don't need to do the subtraction (needed only to get the remainder,
    // in which case we have to divide it by norm)
    return res + q0 + q1 * _base; // + q2 *base*base
}