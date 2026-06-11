#include "cryptography.h"

volatile uint32_t timer_ticks = 0;
volatile uint32_t keyboard_events = 0;

static uint32_t random_state = 0;

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;

    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));

    return ((uint64_t)hi << 32) | lo;
}

void random_init(void)
{
    uint64_t tsc = rdtsc();

    random_state =
        (uint32_t)tsc ^
        (uint32_t)(tsc >> 32) ^
        timer_ticks ^
        keyboard_events ^
        0xA5A5A5A5;

    if (random_state == 0)
    {
        random_state = 1;
    }
}

uint32_t random_u32(void)
{
    random_state ^= (uint32_t)rdtsc();
    random_state ^= timer_ticks;
    random_state ^= keyboard_events;

    uint32_t x = random_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    random_state = x;

    return x;
}

void random_bytes(uint8_t* buffer, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        buffer[i] = (uint8_t)(random_u32() & 0xFF);
    }
}
int random_range(int min, int max)
{
    return min + (random_u32() % (max - min + 1));
}

void generate_aes256_key(uint8_t key[32])
{
    random_bytes(key, 32);
}
