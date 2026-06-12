#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "cryptography.h"
#include "maths.h"
#include "sqrt.h"
#include <limits.h>

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;
//colours:
uint16_t black = 0;
uint16_t blue = 1;
uint16_t green = 2;
uint16_t cyan = 3;
uint16_t red = 4;
uint16_t magenta = 5;
uint16_t brown = 6;
uint16_t light_grey = 7;
uint16_t dark_grey = 8;
uint16_t light_blue = 9;
uint16_t light_green = 10;
uint16_t light_cyan = 11;
uint16_t light_red = 12;
uint16_t pink = 13;
uint16_t yellow = 14;
uint16_t white = 15;
//colour settings variables
uint16_t text_colour = 13;
uint16_t bgcolour = 0;
uint16_t foreground = 0;
 

uint16_t terminal_make_char(char c, char colour)
{
	return (colour << 8) | c;
}

int colour_contrast(int incolour)
{
	switch (incolour)
    {
        case 0:  return 15; // black -> white
        case 1:  return 14; // blue -> yellow
        case 2:  return 13; // green -> pink
        case 3:  return 4;  // cyan -> red
        case 4:  return 11; // red -> light_cyan
        case 5:  return 10; // magenta -> light_green
        case 6:  return 9;  // brown -> light_blue
        case 7:  return 1;  // light_grey -> blue
        case 8:  return 14; // dark_grey -> yellow
        case 9:  return 6;  // light_blue -> brown
        case 10: return 5;  // light_green -> magenta
        case 11: return 4;  // light_cyan -> red
        case 12: return 3;  // light_red -> cyan
        case 13: return 2;  // pink -> green
        case 14: return 1;  // yellow -> blue
	 	case 15: return 0;  // white -> black
		default: return 15;
    }
}

void int_to_string(int num, char* str);
void float_to_string(float value, char *str, int precision)
{
    if (precision < 0) precision = 0;

    // handle negative values
    if (value < 0.0f) {
        *str++ = '-';
        value = -value;
    }

    // rounding
    float rounding = 0.5f;
    for (int i = 0; i < precision; ++i) rounding *= 0.1f;
    value += rounding;

    int int_part = (int)value;
    float frac = value - (float)int_part;

    char int_buf[32];
    int_to_string(int_part, int_buf);
    char *p = int_buf;
    while (*p) *str++ = *p++;

    if (precision > 0) {
        *str++ = '.';
        for (int i = 0; i < precision; ++i) {
            frac *= 10.0f;
            int digit = (int)frac;
            *str++ = '0' + digit;
            frac -= (float)digit;
        }
    }

    *str = '\0';
}

void int_to_string(int num, char* str)
{
    unsigned int n;
    int i = 0;
    int start = 0;

    if (str == 0) return;

    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (num < 0) {
        str[i++] = '-';
        /*
         * Convert magnitude using unsigned to handle INT_MIN.
         * For negative num: -(unsigned)num yields correct magnitude.
         */
        n = (unsigned int)(-(num + 1)) + 1u;
        start = 1; // digits start after '-'
    } else {
        n = (unsigned int)num;
    }

    /* write digits in reverse starting at str[i] */
    int j = i;
    while (n > 0) {
        str[j++] = (char)('0' + (n % 10));
        n /= 10;
    }

    str[j] = '\0';

    /* reverse only the digit portion (preserve '-' if present) */
    int left = start;
    int right = j - 1;
    while (left < right) {
        char tmp = str[left];
        str[left] = str[right];
        str[right] = tmp;
        left++;
        right--;
    }
}

void terminal_putchar(int x, int y, char c, char colour)
{
	video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}


void print_hex(uint8_t* data, int length, int spaces)
{
    const char* hex = "0123456789ABCDEF";

    for (int i = 0; i < length; i++)
    {
        char str[3];

        str[0] = hex[(data[i] >> 4) & 0xF];
        str[1] = hex[data[i] & 0xF];
        str[2] = 0;

        if (spaces == 1)
        {	
            print(" ", 15);
        }

        print(str, text_colour);
    }
	print("\n", 0);
}

void terminal_writechar(char c, char colour)
{	
	if (c == '\n')
	{
		terminal_row += 1;
		terminal_col = 0;
		return;
	}
	terminal_putchar(terminal_col, terminal_row, c, colour);
	terminal_col += 1;
	if (terminal_col >= VGA_WIDTH)
	{
		terminal_col = 0;
		terminal_row += 1;
	}
}

void terminal_initialize()
{
	// video_mem[0] = 0x0341;  color first then ascii value because of endianness prints A in blue
	video_mem = (uint16_t*)(0xB8000);
	terminal_row = 0;
	terminal_col = 0;
	for (int y =0; y < VGA_HEIGHT; y++)
	{
		for (int x = 0; x < VGA_WIDTH; x++)
		{
			terminal_putchar(x, y, ' ', bgcolour);
		}
	}
}

size_t strlen(const char* str)
{
	size_t len = 0;
	while(str[len])
	{
		len++;
	}

	return len;
}

void print(const char* str, char colour)
{	
	size_t len = strlen(str);
	for (int i = 0; i < len; i++)
	{
		terminal_writechar(str[i], colour);
	}
}

extern int problem();

void kernel_main()
{
	random_init();
	bgcolour = 0;
	terminal_initialize();

	print("Initialised entropy [", 15);
	print(" OK ", 2);
	print("]\n", 15);

	print("Initialised terminal [", 15);
	print(" OK ", 2);
	print("]\n", 15);

	// video_mem[79] = terminal_make_char('A', 2); print A in green on the last column
	// initialise interrupt descriptor table

	print("Initialising interrupt descriptor table ", 15);
	idt_init();
	print("[", 15);
	print(" OK ", 2);
	print("]\n", 15);
	print("Testing IDT div by zero: ", 15);
	problem();
	print("[", 15);
	print(" OK ", 2);
	print("]\n", 15);
	
	print("Testing randomness [", 15);
	//generate 6 random numbers
	char num = random_range(0, 50);
	char str[12];
	int_to_string(num, str);
	char num1 = random_range(0, 50);
	char str1[12];
	int_to_string(num1, str1);
	char num2 = random_range(0, 50);
	char str2[12];
	int_to_string(num2, str2);
	char num3 = random_range(0, 50);
	char str3[12];
	int_to_string(num3, str3);
	char num4 = random_range(0, 50);
	char str4[12];
	int_to_string(num4, str4);
	char num5 = random_range(0, 50);
	char str5[12];
	int_to_string(num5, str5);

	//compare the random numbers
	if(num == num1 && num1 == num2 && num2 == num3 && num3 == num4 && num4 == num5)
	{
		print("[ ", 15);
		print("FAILED", red);
		print(" ]", 15);
		print(": not random, fatal security flaw not continuing", red);
		__asm__ volatile ("hlt");
	}
	else
	{
		print(" OK ", 2);
		print("] \n", 15);
	}
	print("Random number log: ", 15);
	print(str, 15);
	print(", ", 15);
	print(str1, 15);
	print(", ", 15);
	print(str2, 15);
	print(", ", 15);
	print(str3, 15);
	print(", ", 15);
	print(str4, 15);
	print(", ", 15);
	print(str5, 15);
	
	print("\nTesting power maths function ", 15);
	int done = 0;
	while (!done)
	{
		int x = 0;
		int y = 0;
		int test_count = 0;
		int power_test1 = 0;
		int power_test2 = 0;
		int power_test3 = 0;
		int power_test4 = 0;
		int power_test5 = 0;
		while (test_count < 5)
		{
			int power_test = power(x, y);
			switch(test_count)
			{
				case 0: power_test1 = power_test; break;
				case 1: power_test2 = power_test; break;
				case 2: power_test3 = power_test; break;
				case 3: power_test4 = power_test; break;
				case 4: power_test5 = power_test; break;
			}
			y++;
			x++;
			test_count++;
		}
		char power_teststr[12];
		char power_teststr1[12];
		char power_teststr2[12];
		char power_teststr3[12];
		char power_teststr4[12];
		if (power_test1 == 1 && power_test2 == 1 && power_test3 == 4 && power_test4 == 27 && power_test5 == 256)
		{
			print("[", 15);
			print(" OK ", 2);
			print("] ", 15);
		}
		print("\nOutputs of 0^0, 1^1, 2^2, 3^3 and 4^4: ", 15);
		int_to_string(power_test1, power_teststr);
		int_to_string(power_test2, power_teststr1);
		int_to_string(power_test3, power_teststr2);
		int_to_string(power_test4, power_teststr3);
		int_to_string(power_test5, power_teststr4);


		print(power_teststr, 15);
		print(", ", 15);
		print(power_teststr1, 15);
		print(", ", 15);
		print(power_teststr2, 15);
		print(", ", 15);
		print(power_teststr3, 15);
		print(", ", 15);
		print(power_teststr4, 15);
		done = 1;
	}
	float sqrt_test0 = ks_sqrt(5);
	char sqrt_test1[32];
	print("\nTesting SQRT function: ", 15);
	float error = ks_sqrt(5) - 2.2360679775f;

	if (error < 0)
    	error = -error;
	if (error < 0.0001f)
	{
    	print("[", 15);
    	print(" OK ", 2);
    	print("] ", 15);
	}
	float_to_string(sqrt_test0, sqrt_test1, 12);
	print(sqrt_test1, 15);
	
	uint8_t aes_key[32];
	generate_aes256_key(aes_key);

	print("\nAES-256 Key: ", text_colour);

	print_hex(aes_key, 32, 0); // print aes key thats 32 bytes with no spaces
}