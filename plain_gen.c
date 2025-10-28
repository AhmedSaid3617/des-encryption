#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

int main() {
    FILE *f = fopen("plaintext.bin", "wb");
    if (!f) {
        perror("Error creating plain.bin");
        return 1;
    }

    size_t num_blocks;
    printf("Enter number of 64-bit plaintext blocks to generate: ");
    scanf("%zu", &num_blocks);

    srand((unsigned)time(NULL)); // seed random generator

    for (size_t i = 0; i < num_blocks; i++) {
        uint64_t block = 0;
        // generate random 64-bit value using two 32-bit rand() calls
        block = ((uint64_t)rand() << 32) | rand();
        fwrite(&block, sizeof(uint64_t), 1, f);
    }

    // uint64_t plains[3] = {
    //     0x0123456789ABCDEF,
    //     0x1111111111111111,
    //     0x0000000000000000
    // };
    // fwrite(plains, sizeof(uint64_t), 3, f);
    

    fclose(f);
    printf("plain.bin created with %zu random 64-bit blocks.\n", num_blocks);
    return 0;
}
