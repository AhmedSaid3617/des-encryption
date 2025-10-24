#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

uint64_t permutation_pc2(uint64_t input)
{
    static uint8_t pc2_array[48] = {
        14, 17, 11, 24, 1, 5,
        3, 28, 15, 6, 21, 10,
        23, 19, 12, 4, 26, 8,
        16, 7, 27, 20, 13, 2,
        41, 52, 31, 37, 47, 55,
        30, 40, 51, 45, 33, 48,
        44, 49, 39, 56, 34, 53,
        46, 42, 50, 36, 29, 32};
    // out[1] = in[14]
    // out[max] = in[max-14]
    // out[48 - 1] = in[56-14]
    // out[48 - i] = in[56-arr[i]]
    // out |= (in & 1 << (56-arr[i]))?1:0 << (48-i)
    uint64_t permutation_result = 0;
    for (int i = 0; i < 48; i++)
    {
        uint64_t y = ((uint64_t)1 << (56 - pc2_array[i]));
        uint64_t x = (input & y) ? 1 : 0;
        permutation_result |= x << (48 - 1 - i);
    }
    return permutation_result;
}

int main()
{
    uint64_t input = (uint64_t)1 << 56-14;
    input |= (uint64_t)1 << (56-17);
    input |= (uint64_t)1 << (56-11);
    input |= (uint64_t)1 << (56-24);
    uint64_t round_key = 0;

    printf("pc2: 0x%lx\n", permutation_pc2(input));
    printf("exp: 0x%lx\n", 0xF00000000000);

}