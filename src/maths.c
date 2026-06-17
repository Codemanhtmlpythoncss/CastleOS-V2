#include "maths.h"

double pi = 3.141592653589793;

double cos(double x)
{
	double result = 1.0;
	double term = 1.0;
	short int n = 1;

	while (n <= 15)
	{
		term *= -(x * x) / ((2.0 * n - 1.0) * (2.0 * n));
		result += term;
		n += 1;
	}

	return result;
}

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
