int key_gen_test()
{
    uint64_t round_keys[16];
    uint64_t key = (uint64_t)1 << 64 -57; // A sample key
    generate_keys(key, round_keys);

    printf("Generated Round Keys:\n");
    for (int i = 0; i < 16; i++)
    {
        printf("Round %2d Key: 0x%lx\n", i + 1, round_keys[i]);
    }
    
}