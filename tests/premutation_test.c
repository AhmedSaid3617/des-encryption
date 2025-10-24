#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

int pc1_test()
{
    uint64_t input = (uint64_t)1 << 64-57;
    input |= (uint64_t)1 << (64-49);
    input |= (uint64_t)1 << (64-41);
    input |= (uint64_t)1 << (64-33);
    uint64_t round_key = 0;

    printf("pc1: 0x%lx\n", permutation_pc1(input));
    printf("exp: 0x%lx\n", 0xF0000000000000);

}

int pc2_test()
{
    uint64_t input = (uint64_t)1 << 56-14;
    input |= (uint64_t)1 << (56-17);
    input |= (uint64_t)1 << (56-11);
    input |= (uint64_t)1 << (56-24);
    uint64_t round_key = 0;

    printf("pc2: 0x%lx\n", permutation_pc2(input));
    printf("exp: 0x%lx\n", 0xF00000000000);

}