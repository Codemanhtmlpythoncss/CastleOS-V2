#include "maths.h"

double pi = 3.141592653589793;

// Approximate cosine with the first 16 terms of its Maclaurin series.
double cos(double x)
{
	double result = 1.0;
	double term = 1.0;
	short int n = 1;

	while (n <= 15)
	{
		// Derive the next term from the previous one to avoid factorial calculations.
		term *= -(x * x) / ((2.0 * n - 1.0) * (2.0 * n));
		result += term;
		n += 1;
	}

	return result;
}

// Multiply the base by itself exponent times; non-positive exponents return one.
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
