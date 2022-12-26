#include <float.h>
#include <math.h>

/*
### float max_float

DESCRIPTION: returns the maximum value of a float
 */
float max_float()
{
	return FLT_MAX;
}

/*
### double max_double

DESCRIPTION: returns the maximum value of a double
 */
double max_double()
{
	return DBL_MAX;
}

/*
### long double max_long_double

DESCRIPTION: returns the maximum value of a long double
 */
long double max_long_double()
{
	return LDBL_MAX;
}

/*
### int isfloat_nan

DESCRIPTION: returns 1 if the input float is not a number
INPUT:
	float f
 */
int isfloat_nan(float f)
{
	return isnan(f);
}

/*
### int isfloat_inf

DESCRIPTION: returns 1 if the input float is infinity
INPUT:
	float f
 */
int isfloat_inf(float f)
{
	return isinf(f);
}

/*
### int isdouble

DESCRIPTION: returns 1 if the input double is not a number
INPUT:
	double d
 */
int isdouble_nan(double f)
{
	return isnan(f);
}

/*
### int isdouble_inf

DESCRIPTION: returns 1 if the input double is infinity
INPUT:
	double d
 */
int isdouble_inf(double f)
{
	return isinf(f);
}

