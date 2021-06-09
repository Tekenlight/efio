#include <float.h>
#include <math.h>

float max_float()
{
	return FLT_MAX;
}

double max_double()
{
	return DBL_MAX;
}

long double max_long_double()
{
	return LDBL_MAX;
}

int isfloat_nan(float f)
{
	return isnan(f);
}

int isfloat_inf(float f)
{
	return isinf(f);
}

int isdouble_nan(double f)
{
	return isnan(f);
}

int isdouble_inf(double f)
{
	return isinf(f);
}

