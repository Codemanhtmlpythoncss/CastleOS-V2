#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "cryptography.h"
#include "maths.h"
#include "sqrt.h"
#include <limits.h>
#include "io/io.h"

// VGA text-mode state and the current cursor position.
uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;
// VGA text-mode colour attribute values.
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
// Default text and background colours used by terminal output.
uint16_t text_colour = 13;
uint16_t bgcolour = 15;
uint16_t foreground = 0;
 

// Pack a VGA colour attribute and character into one text-mode cell.
uint16_t terminal_make_char(char c, char colour)
{
	return (colour << 8) | c;
}

const char* TerryQuotes[] =
{
	"\"An idiot admires complexity, a genius admires simplicity, a physicist tries to make it simple, for an idiot anything the more complicated it is the more he will admire it, if you make something so clusterfucked he can't understand it he's gonna think you're a god cause you made it so complicated nobody can understand it. That's how they write journals in Academics, they try to make it so complicated people think you're a genius\" \n- Terry Davis, Creator of Temple OS, RIP",
	"\"Real programmers can write assembly code in any language.\"\n- Terry Davis, Creator of Temple OS, RIP",
	"\"What's reality? I don't know.\" \n- Terry Davis, Creator of TempleOS, RIP",
	"\"The bird is okay even though he doesn't understand the world. You're that bird looking at the monitor, and you're thinking to yourself, 'I can figure this out'.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"The difference between an amateur and a professional is you write your own fucking compiler.\" \n- Terry Davis, Creator of TempleOS, RIP",
	"\"So, someone wanted to know what I use Ubuntu for. I use it to download VMware to run TempleOS.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"I like elephants and God likes elephants.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"God likes music that makes you feel.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"You banned me from Twitter, God bans you from Heaven.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"The first time you meet an angel you get a horrible beating.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"I wrote my own compiler I'm not a idiot like Linus\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"God said TempleOS was destined to be the third temple.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"Computers are made for making things.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"I don't use Linux. I use Linux to download things for TempleOS.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"If you don't understand it, then simplify it.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"I made an operating system from scratch.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"The best code is code you can understand.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"Everybody thinks they're going to change the world.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"I like making things.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"The simpler it is, the easier it is to know it's correct.\"\n- Terry Davis, Creator of TempleOS, RIP",
	"\"TempleOS is my masterpiece.\"\n- Terry Davis, Creator of TempleOS, RIP"
};

// Print one quote selected from the table above.
void terry_quote()
{
	print(TerryQuotes[random_range(0, 20)], 15);
}

// Format a double as fixed-point decimal text, preserving its sign.
void double_to_string(double value, char *buffer, int precision)
{
    char *ptr = buffer;

    /* Handle negative numbers */
    if (value < 0.0)
    {
        *ptr++ = '-';
        value = -value;
    }

    /* Round to requested precision */
    double rounding = 0.5;
    for (int i = 0; i < precision; i++)
        rounding /= 10.0;

    value += rounding;

    /* Integer part (32-bit only) */
    uint32_t int_part = (uint32_t)value;

    /* Fractional part */
    double frac_part = value - (double)int_part;

    /* Convert integer part */
    char temp[16];
    int pos = 0;

    if (int_part == 0)
    {
        temp[pos++] = '0';
    }
    else
    {
        while (int_part > 0)
        {
            temp[pos++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }

    /* Reverse integer digits */
    while (pos > 0)
        *ptr++ = temp[--pos];

    /* Decimal point */
    if (precision > 0)
    {
        *ptr++ = '.';

        for (int i = 0; i < precision; i++)
        {
            frac_part *= 10.0;

            int digit = (int)frac_part;

            *ptr++ = '0' + digit;

            frac_part -= digit;
        }
    }

    *ptr = '\0';
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
// Format a float with the requested number of digits after the decimal point.
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

// Convert a signed integer to decimal, including INT_MIN without signed overflow.
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


// Write bytes as hexadecimal pairs, optionally separating them with spaces.
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
	// Newlines reset the column; ordinary characters advance and wrap at screen width.
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
	// VGA text memory starts at this physical address; clear every visible cell.
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
	// Route each character through the cursor-aware writer so wrapping is consistent.
	size_t len = strlen(str);
	for (int i = 0; i < len; i++)
	{
		terminal_writechar(str[i], colour);
	}
}


void kernel_main()
{
	// Initialize the terminal and interrupt handling before running kernel checks.
	random_init();
	bgcolour = black;
	terminal_initialize();
	print("Psst... hey, you! Welcome to the kernel!\n", 15);
	print("Initialised entropy [", 15);
	print(" OK ", 2);
	print("]\n", 15);

	print("Initialised terminal [", 15);
	print(" OK ", 2);
	print("]\n", 15);

	// Example: write a green 'A' into the final column of the first VGA row.
	// video_mem[79] = terminal_make_char('A', 2);
	// Install the interrupt descriptor table before enabling interrupts.
	print("Initialising interrupt descriptor table ", 15);
	idt_init();
	// Enable CPU interrupt delivery only after installing the IDT.
	enable_interrupts();
	print("[", 15);
	print(" OK ", 2);
	print("]\n", 15);
	print("Testing randomness [", 15);
	// Generate and display sample values as a basic RNG startup check.
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

	// Identical samples are treated as a startup failure.
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
	// Exercise power() with matching base/exponent pairs from zero through four.
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
	double sqrt_test0 = ks_sqrt(10.0);
	char sqrt_teststr0[32];
	double sqrt_test1 = ks_sqrt(9.0);
	char sqrt_teststr1[32];
	double sqrt_test2 = ks_sqrt(8.0);
	char sqrt_teststr2[32];
	double sqrt_test3 = ks_sqrt(7.0);
	char sqrt_teststr3[32];
	double sqrt_test4 = ks_sqrt(6.0);
	char sqrt_teststr4[32];
	double sqrt_test5 = ks_sqrt(5.0);
	char sqrt_teststr5[32];
	double sqrt_test6 = ks_sqrt(4.0);
	char sqrt_teststr6[32];
	double sqrt_test7 = ks_sqrt(3.0);
	char sqrt_teststr7[32];
	double sqrt_test8 = ks_sqrt(2.0);
	char sqrt_teststr8[32];
	double sqrt_test9 = ks_sqrt(1.0);
	char sqrt_teststr9[32];
	print("\nTesting SQRT function: ", 15);
	// Compare computed roots to reference values and report their decimal forms.
	double error0 = sqrt_test0 - 3.162277660168379332;
	double error1 = sqrt_test0 - 3.000000000000000000;
	double error2 = sqrt_test0 - 2.828427124746190098;
	double error3 = sqrt_test0 - 2.645751311064590591;
	double error4 = sqrt_test0 - 2.449489742783178098;
	double error5 = sqrt_test0 - 2.236067977499789696;
	double error6 = sqrt_test0 - 2.000000000000000000;
	double error7 = sqrt_test0 - 1.732050807568877294;
	double error8 = sqrt_test0 - 1.414213562373095049;
	double error9 = sqrt_test0 - 1.000000000000000000;

	if (error0 || error1 || error2 || error3 || error4 || error5 || error6 || error7 || error8 || error9 < 0)
    	error0 = -error0;
		error1 = -error1;
		error2 = -error2;
		error3 = -error3;
		error4 = -error4;
		error5 = -error5;
		error6 = -error6;
		error7 = -error7;
		error8 = -error8;
		error9 = -error9;
	if (error0 && error1 && error2 && error3 && error4 && error5 && error6 && error7 && error8 && error9 < 0.0001f)
	{
    	print("[", 15);
    	print(" OK ", 2);
    	print("] \n", 15);
	}
	double_to_string(sqrt_test0, sqrt_teststr0, 15);
	double_to_string(sqrt_test1, sqrt_teststr1, 15);
	double_to_string(sqrt_test2, sqrt_teststr2, 15);
	double_to_string(sqrt_test3, sqrt_teststr3, 15);
	double_to_string(sqrt_test4, sqrt_teststr4, 15);
	double_to_string(sqrt_test5, sqrt_teststr5, 15);
	double_to_string(sqrt_test6, sqrt_teststr6, 15);
	double_to_string(sqrt_test7, sqrt_teststr7, 15);
	double_to_string(sqrt_test8, sqrt_teststr8, 15);
	double_to_string(sqrt_test9, sqrt_teststr9, 15);
	print("Result log: \n", 15);
	print("sqrt(10): ", 15);
	print(sqrt_teststr0, 15);
	print(",\n", 15);
	print("sqrt(9): ", 15);
	print(sqrt_teststr1, 15);
	print(",\n", 15);
	print("sqrt(8): ", 15);
	print(sqrt_teststr2, 15);
	print(",\n", 15);
	print("sqrt(7): ", 15);
	print(sqrt_teststr3, 15);
	print(",\n", 15);
	print("sqrt(6): ", 15);
	print(sqrt_teststr4, 15);
	print(",\n", 15);
	print("sqrt(5): ", 15);
	print(sqrt_teststr5, 15);
	print(",\n", 15);
	print("sqrt(4): ", 15);
	print(sqrt_teststr6, 15);
	print(",\n", 15);
	print("sqrt(3): ", 15);
	print(sqrt_teststr7, 15);
	print(",\n", 15);
	print("sqrt(2): ", 15);
	print(sqrt_teststr8, 15);
	print(",\n", 15);
	print("sqrt(1): ", 15);
	print(sqrt_teststr9, 15);
	
	uint8_t aes_key[32];
	generate_aes256_key(aes_key);

	print("\nAES-256 Key: ", text_colour);
	print_hex(aes_key, 32, 0); // Display all 32 key bytes without separators.
	terry_quote();
	print("\n", 0);
}