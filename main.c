#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define BUFFER_SIZE 1024 * 8

// Convert between host byte order and big-endian (network byte order)
static inline uint64_t htobe64_custom(uint64_t host_64bits)
{
    // Check if system is little-endian
    uint16_t test = 0x0001;
    uint8_t is_little_endian = *(uint8_t *)&test;

    if (is_little_endian)
    {
        // Swap bytes for little-endian systems
        return ((host_64bits & 0x00000000000000FFULL) << 56) |
               ((host_64bits & 0x000000000000FF00ULL) << 40) |
               ((host_64bits & 0x0000000000FF0000ULL) << 24) |
               ((host_64bits & 0x00000000FF000000ULL) << 8) |
               ((host_64bits & 0x000000FF00000000ULL) >> 8) |
               ((host_64bits & 0x0000FF0000000000ULL) >> 24) |
               ((host_64bits & 0x00FF000000000000ULL) >> 40) |
               ((host_64bits & 0xFF00000000000000ULL) >> 56);
    }
    return host_64bits; // Already big-endian
}

static inline uint64_t be64toh_custom(uint64_t big_endian_64bits)
{
    // Conversion is symmetric
    return htobe64_custom(big_endian_64bits);
}

static inline uint64_t expansion(uint32_t round_input)
{
    uint64_t expansion_result = 0;

    static const uint8_t E_TABLE[48] = {
        32, 1, 2, 3, 4, 5,
        4, 5, 6, 7, 8, 9,
        8, 9, 10, 11, 12, 13,
        12, 13, 14, 15, 16, 17,
        16, 17, 18, 19, 20, 21,
        20, 21, 22, 23, 24, 25,
        24, 25, 26, 27, 28, 29,
        28, 29, 30, 31, 32, 1};

    for (int i = 0; i < 48; i++)
    {
        expansion_result <<= 1;
        expansion_result |= (round_input >> (32 - E_TABLE[i])) & 0x01;
    }
    return expansion_result;
}

static inline uint8_t get_row_index(uint8_t s_box_group)
{ // input: b5b4b3b2b1b0
    uint8_t row_index = 0;
    row_index = (s_box_group & 0x20) >> 4; // row index = 0000_00b50
    row_index |= (s_box_group & 0x01);     // row index = 0000_00b5b0
    return row_index;
}

static inline uint8_t get_col_index(uint8_t s_box_group)
{ // input: b5b4b3b2b1b0
    uint8_t col_index = 0;
    col_index = (s_box_group & 0x1E) >> 1; // col_index = 0000_b4b3b2b1

    return col_index;
}

static const uint32_t S1[4][16] = { // MSB
    {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
    {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
    {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
    {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}};

static const uint32_t S2[4][16] = {
    {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
    {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
    {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
    {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}};

static const uint32_t S3[4][16] = {
    {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
    {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
    {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
    {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}};

static const uint32_t S4[4][16] = {
    {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
    {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
    {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
    {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}};

static const uint32_t S5[4][16] = {
    {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
    {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
    {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
    {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}};

static const uint32_t S6[4][16] = {
    {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
    {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
    {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
    {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}};

static const uint32_t S7[4][16] = {
    {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
    {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
    {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
    {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}};

static const uint32_t S8[4][16] = { // LSB
    {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
    {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
    {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
    {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}};

static inline uint32_t s_box(uint64_t s_box_input)
{ // 48-bit E_table output XORed with key
    uint8_t row, col;
    uint32_t res = 0;

    uint8_t input_group[8] = {0};

    for (int i = 0; i < 8; i++)
    { // group [0] stores the least 6 bits
        input_group[i] = (s_box_input >> (6 * i)) & 0x3F;
        //    printf("Group #%d: %X\n",i,input_group[i]);
    }

    for (int i = 7; i >= 0; i--)
    {

        row = get_row_index(input_group[i]);
        col = get_col_index(input_group[i]);

        switch (i)
        {
        case 7:
            res |= (S1[row][col]) << 28;
            break;
        case 6:
            res |= (S2[row][col]) << 24;
            break;
        case 5:
            res |= (S3[row][col]) << 20;
            break;
        case 4:
            res |= (S4[row][col]) << 16;
            break;
        case 3:
            res |= (S5[row][col]) << 12;
            break;
        case 2:
            res |= (S6[row][col]) << 8;
            break;
        case 1:
            res |= (S7[row][col]) << 4;
            break;
        case 0:
            res |= S8[row][col];
            break;
        }
    }
    return res;
}

static const uint8_t P_TABLE[32] = {
    16, 7, 20, 21, 29, 12, 28, 17,
    1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9,
    19, 13, 30, 6, 22, 11, 4, 25};

static inline uint32_t permutation_p(uint32_t P_input)
{
    uint32_t permutation_result = 0;

    for (int i = 0; i < 32; i++)
    {
        permutation_result <<= 1;
        permutation_result |= (P_input >> (32 - P_TABLE[i])) & 0x01;
    }
    return permutation_result;
}

static const uint8_t IP_TABLE[64] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7};

static inline uint64_t permutation_i(uint64_t input_block)
{

    uint64_t permutation_result = 0;
    for (int i = 0; i < 64; i++)
    {
        uint64_t y = ((uint64_t)1 << (64 - IP_TABLE[i]));
        uint64_t x = (input_block & y) ? 1 : 0;
        permutation_result |= x << (64 - 1 - i);
    }
    return permutation_result;
}

static const uint8_t IP_INV_TABLE[64] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41, 9, 49, 17, 57, 25};

static inline uint64_t permutation_i_inv(uint64_t input_block)
{

    uint64_t permutation_result = 0;
    for (int i = 0; i < 64; i++)
    {
        uint64_t y = ((uint64_t)1 << (64 - IP_INV_TABLE[i]));
        uint64_t x = (input_block & y) ? 1 : 0;
        permutation_result |= x << (64 - 1 - i);
    }
    return permutation_result;
}

// Perform a single Feistel round: expand right, XOR with round key, S-boxes, permutation
static inline uint32_t feistel_round(uint32_t right, uint64_t round_key)
{
    uint64_t expanded = expansion(right);
    uint64_t xored = round_key ^ expanded;
    uint32_t subbed = s_box(xored);
    uint32_t permuted = permutation_p(subbed);
    return permuted;
}

static const uint8_t PC1_TABLE[56] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4};

static inline uint64_t permutation_pc1(uint64_t input)
{

    uint64_t permutation_result = 0;
    for (int i = 0; i < 56; i++)
    {
        uint64_t y = ((uint64_t)1 << (64 - PC1_TABLE[i]));
        uint64_t x = (input & y) ? 1 : 0;
        permutation_result |= x << (56 - 1 - i);
    }
    return permutation_result;
}

static const uint8_t PC2_TABLE[48] = {
    14, 17, 11, 24, 1, 5,
    3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8,
    16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32};

static inline uint64_t permutation_pc2(uint64_t input)
{

    uint64_t permutation_result = 0;
    for (int i = 0; i < 48; i++)
    {
        uint64_t y = ((uint64_t)1 << (56 - PC2_TABLE[i]));
        uint64_t x = (input & y) ? 1 : 0;
        permutation_result |= x << (48 - 1 - i);
    }
    return permutation_result;
}

// Circular left shift for 28-bit value
static inline uint32_t rotate_left_28(uint32_t val, int n)
{
    return ((val << n) | (val >> (28 - n))) & 0x0FFFFFFF; // Mask to keep 28 bits
}

// Rotate the 56-bit key (split into two 28-bit halves)
static inline uint64_t des_key_rotate(uint64_t key56, int rotations)
{
    // Extract left and right 28-bit halves
    uint32_t C = (key56 >> 28) & 0x0FFFFFFF;
    uint32_t D = key56 & 0x0FFFFFFF;

    // Perform circular left shifts
    C = rotate_left_28(C, rotations);
    D = rotate_left_28(D, rotations);

    // Combine back into a 56-bit key
    uint64_t rotated_key = ((uint64_t)C << 28) | (uint64_t)D;
    return rotated_key;
}

static inline void generate_keys(uint64_t key, uint64_t *key_buffer)
{
    static const uint8_t key_shift[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};
    uint64_t effective_key = permutation_pc1(key);
    uint64_t tmp_key = effective_key;
    for (int i = 0; i < 16; i++)
    {
        tmp_key = des_key_rotate(tmp_key, key_shift[i]);
        key_buffer[i] = permutation_pc2(tmp_key);
    }
}

static inline uint64_t Encrypt(uint64_t text, uint64_t key)
{
    uint64_t keybuffer[16];

    generate_keys(key, keybuffer);

    uint64_t text_before_round = permutation_i(text);
    uint64_t text_after_round;
    uint32_t left_part;
    uint64_t right_part;
    uint32_t fiestel_out;

    for (int i = 0; i < 16; i++)
    {
        text_after_round = 0;
        right_part = text_before_round & (0x00000000ffffffff);
        left_part = (text_before_round & (0xffffffff00000000)) >> 32;
        text_after_round |= right_part << (32);
        fiestel_out = feistel_round(right_part, keybuffer[i]);
        text_after_round |= fiestel_out ^ left_part;
        text_before_round = text_after_round;
    }

    text_after_round = 0;
    right_part = text_before_round & (0x00000000ffffffff);
    left_part = (text_before_round & (0xffffffff00000000)) >> 32;
    text_after_round |= left_part;
    text_after_round |= right_part << (32);

    return permutation_i_inv(text_after_round);
}

static inline uint64_t Decrypt(uint64_t text, uint64_t key)
{
    uint64_t keybuffer[16];

    generate_keys(key, keybuffer);

    uint64_t text_before_round = permutation_i(text);
    uint64_t text_after_round;
    uint32_t left_part;
    uint64_t right_part;
    uint32_t fiestel_out;

    for (int i = 15; i >= 0; i--)
    {
        text_after_round = 0;
        right_part = text_before_round & (0x00000000ffffffff);
        left_part = (text_before_round & (0xffffffff00000000)) >> 32;
        text_after_round |= right_part << (32);
        fiestel_out = feistel_round(right_part, keybuffer[i]);
        text_after_round |= fiestel_out ^ left_part;
        text_before_round = text_after_round;
    }

    text_after_round = 0;
    right_part = text_before_round & (0x00000000ffffffff);
    left_part = (text_before_round & (0xffffffff00000000)) >> 32;
    text_after_round |= left_part;
    text_after_round |= right_part << (32);

    return permutation_i_inv(text_after_round);
}

uint64_t plaintext_buff[BUFFER_SIZE];
uint64_t ciphertext_buff[BUFFER_SIZE];
uint32_t ciphertext_count = 0;

int main(int argc, char **argv)
{

    char mode = argv[1][0];
    const char *keyFile = argv[2];
    const char *inputFile = argv[3];
    const char *outputFile = argv[4];

    FILE *fkey = fopen(keyFile, "rb");
    FILE *fin = fopen(inputFile, "rb");
    FILE *fout = fopen(outputFile, "wb");

    if (!fin || !fkey || !fout)
    {
        perror("Error opening file");
        return 1;
    }

    if (mode == 'e')
    {
        uint64_t key;
        if (fread(&key, sizeof(uint64_t), 1, fkey) != 1)
        {
            perror("Error reading key");
            return 1;
        }
        // Convert key from big-endian (file format) to host byte order
        key = be64toh_custom(key);

        uint64_t plaintext, ciphertext;
        int chunk_size;

        while (chunk_size = fread(plaintext_buff, sizeof(uint64_t), BUFFER_SIZE, fin))
        {
            // Convert plaintext from big-endian to host byte order
            for (int i = 0; i < chunk_size; i++)
            {
                plaintext = be64toh_custom(plaintext_buff[i]);
                //plaintext = plaintext_buff[i];
                ciphertext = Encrypt(plaintext, key);
                // Convert ciphertext to big-endian before writing
                ciphertext_buff[i] = htobe64_custom(ciphertext);
                //ciphertext_buff[i] = ciphertext;
            }

            fwrite(&ciphertext_buff, sizeof(uint64_t), chunk_size, fout);
        }
        // printf("Encryption completed successfully.\n");
    }
    else if (mode == 'd')
    {
        uint64_t key;
        if (fread(&key, sizeof(uint64_t), 1, fkey) != 1)
        {
            perror("Error reading key");
            return 1;
        }
        // Convert key from big-endian (file format) to host byte order
        key = be64toh_custom(key);

        uint64_t plaintext, ciphertext;
        int chunk_size;

        while (chunk_size = fread(&ciphertext_buff, sizeof(uint64_t), BUFFER_SIZE, fin))
        {

            for (int i = 0; i < chunk_size; i++)
            {
                // Convert ciphertext from big-endian to host byte order
                ciphertext = be64toh_custom(ciphertext_buff[i]);
                plaintext = Decrypt(ciphertext, key);
                // Convert plaintext to big-endian before writing
                plaintext_buff[i] = htobe64_custom(plaintext);
            }

            fwrite(&plaintext_buff, sizeof(uint64_t), chunk_size, fout);
        }
        // printf("Encryption completed successfully.\n");
    }
    else
    {
        fprintf(stderr, "Invalid mode: %c (use 'e' or 'd')\n", mode);
    }

    fclose(fin);
    fclose(fkey);
    fclose(fout);

    return 0;
}
