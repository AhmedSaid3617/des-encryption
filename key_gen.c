#include <stdio.h>
#include <stdint.h>

int main() {
    FILE *f = fopen("key.bin", "wb");
    if (!f) {
        perror("Error creating key file");
        return 1;
    }

    uint64_t key = 0x133457799BBCDFF1ULL;  // example DES key
    fwrite(&key, sizeof(uint64_t), 1, f);

    fclose(f);
    printf("key.bin created successfully.\n");
    return 0;
}
