//
//  main.c
//
//  Created by Laurence Lundblade on 8/11/23.
//  Copyright © 2023 Security Theory. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>



/*
 * Copy float bits to/from a uint32_t without numeric conversion.
 * These avoid type punning, compiler warnings and such. The optimizer
 * reduces them to a simple assignment.  This is a crusty corner of C.
 */
static uint32_t copy_float_to_uint(float f)
{
    uint32_t u32;
    memcpy(&u32, &f, sizeof(uint32_t));
    return u32;
}

static float copy_uint_to_float(uint32_t u32)
{
    float  f;
    memcpy(&f, &u32, sizeof(uint32_t));
    return f;
}


/* Constants that come from IEEE 754 */
#define SINGLE_NUM_SIGNIFICAND_BITS (23)
#define SINGLE_NUM_EXPONENT_BITS    (8)
#define SINGLE_NUM_SIGN_BITS        (1)

#define SINGLE_SIGNIFICAND_SHIFT    (0)
#define SINGLE_EXPONENT_SHIFT       (SINGLE_NUM_SIGNIFICAND_BITS)
#define SINGLE_SIGN_SHIFT           (SINGLE_NUM_SIGNIFICAND_BITS + SINGLE_NUM_EXPONENT_BITS)

#define SINGLE_SIGNIFICAND_MASK     (0x7fffffU) // The lower 23 bits
#define SINGLE_EXPONENT_MASK        (0xffU << SINGLE_EXPONENT_SHIFT) // 8 bits of exponent
#define SINGLE_SIGN_MASK            (0x01U << SINGLE_SIGN_SHIFT) // 1 bit of sign
#define SINGLE_QUIET_NAN_BIT        (0x01U << (SINGLE_NUM_SIGNIFICAND_BITS-1))

#define SINGLE_EXPONENT_BIAS        (127)
#define SINGLE_EXPONENT_MAX         (SINGLE_EXPONENT_BIAS)    //  127 unbiased
#define SINGLE_EXPONENT_MIN         (-SINGLE_EXPONENT_BIAS+1) // -126 unbiased
#define SINGLE_EXPONENT_ZERO        (-SINGLE_EXPONENT_BIAS)   // -127 unbiased
#define SINGLE_EXPONENT_INF_OR_NAN  (SINGLE_EXPONENT_BIAS+1)  //  128 unbiased



/* The following functions all do the same thing in different ways. They
 * return true if float is a whole integer.
 */

bool check_ceil(float f)
{
    return ceilf(f) == f;
}

bool check_floor(float f)
{
    return floorf(f) == f;
}

bool check_cast(float f)
{
    int64_t n = (int64_t)f;
    float f2 = (float)n;
    return f2 == f;
}

bool check_nearby(float f)
{
    return nearbyint(f) == f;
}

bool check_rintf(float f)
{
    return rintf(f) == f;
}

bool check_bits(float f)
{
    int nz_bits;
    uint32_t mask;

    /* Convert float to uint32 bits and pull out exponent and significand */
    const uint32_t f_bits       = copy_float_to_uint(f);
    const int32_t  unbiased_exp = (int32_t)((f_bits & SINGLE_EXPONENT_MASK) >> SINGLE_EXPONENT_SHIFT) - SINGLE_EXPONENT_BIAS;
    const uint32_t significand  = f_bits & SINGLE_SIGNIFICAND_MASK;


    /* Count down from 23 to the number of bits that are not zero in
     * the significand. This counts from the least significant bit
     * until a non-zero bit is found.
     */
    for(nz_bits = SINGLE_NUM_SIGNIFICAND_BITS; nz_bits > 0; nz_bits--) {
        mask = (0x01 << SINGLE_NUM_SIGNIFICAND_BITS) >> nz_bits;
        if(mask & significand) {
            break;
        }
    }

    /* The exponent effectively shifts the bits in the significand to
     * the left. If there are fewer bits set in the significand than
     * are shifted, it is a whole integer.
     */
    // https://softwareengineering.stackexchange.com/questions/215065/can-anyone-explain-representation-of-float-in-memory
    if(nz_bits <= unbiased_exp) {
        return true;
    }

    return false;
}



int main(int argc, const char * argv[])
{
    float f;
    bool b_is_whole_int;
    uint32_t n;

    /* Loops over all possible single-precision floats by looping over
     * all uint32_t bit patterns and copying them into a float
     * variable.
     */
    for(n = 0; n < UINT32_MAX; n+=1) {
        f = copy_uint_to_float(n);

        if(isnan(f) || f == 0) {
            continue;
        }

        b_is_whole_int = check_bits(f) ;

        if(b_is_whole_int != check_ceil(f)) {
            printf("check_ceil failed %f\n", f);
        }
        if(b_is_whole_int != check_floor(f)) {
            printf("check_floor failed %f\n", f);
        }
        if(f > -(float)UINT32_MAX && f < (float)UINT32_MAX) {
            /* The cast method only works on the range of values that
             * can fit into a uint32_t. */
            if(b_is_whole_int != check_cast(f)) {
                printf("check_cast failed %f\n", f);
            }
        }
        if(b_is_whole_int != check_nearby(f)) {
            printf("check_nearby failed %f\n", f);
        }
        if(b_is_whole_int != check_rintf(f)) {
            printf("check_rintf failed %f\n", f);
        }

        /* Show progress indicator, because this takes a while */
        if(n % 0x0fffffff == 0) {
            printf("%u%% done...\n", n / (0xffffffff / 100));
        }
    }

    printf("DONE\n");
}
