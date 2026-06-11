#include "maths.h"

int power(int number, int power)
{
	int current_output = 1;

	while (power > 0)
	{
		current_output *= number;
		power--;
	}

	return current_output;
}