#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <stdint.h>

void random_init(void);
uint32_t random_u32(void);
void random_bytes(uint8_t* buffer, uint32_t length);
int random_range(int min, int max);
void generate_aes256_key(uint8_t key[32]);
#endif