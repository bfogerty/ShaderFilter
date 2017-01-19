#ifndef __CommonMath__
#define __CommonMath__

float clamp(float value, float min, float max)
{
	if (value < min)
	{
		value = min;
	}
	else if (value > max)
	{
		value = max;
	}

	return value;
}

float clamp01(float value)
{
	return clamp(value, 0.000f, 1.0f);
}


#endif