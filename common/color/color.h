#ifndef __COLOR__
#define __COLOR__
#include <memory.h>
#include <algorithm>

class Color
{
public:

	enum RGBA
	{
		r = 0,
		g = 1,
		b = 2,
		a = 3,
	};

	Color()
	{
		memset(values, 0, sizeof(unsigned int) * 4);
	}

	Color(float t)
	{
		std::fill(values, values + 4, t);
	}

	Color(const Color& other)
	{
		memcpy(values, (void*)(other.values), sizeof(float) * 4);
	}

	Color operator =(const Color& other)
	{
		Color result;
		memcpy(result.values, (void*)(other.values), sizeof(float) * 4);

		return result;
	}

	Color(float rVal, float gVal, float bVal)
	{
		values[r] = rVal;
		values[g] = gVal;
		values[b] = bVal;
		values[a] = 1.0f;
	}

	Color(float rVal, float gVal, float bVal, float aVal)
	{
		values[r] = rVal;
		values[g] = gVal;
		values[b] = bVal;
		values[a] = aVal;
	}

	void SetValues(float rVal, float gVal, float bVal, float aVal)
	{
		values[r] = rVal;
		values[g] = gVal;
		values[b] = bVal;
		values[a] = aVal;
	}

	void SetValues(float t)
	{
		std::fill(values, values + 4, t);
	}

	const float* GetValues()
	{
		return values;
	}

	static void Clamp(Color& color, float minValue, float maxValue)
	{
		if (color.values[r] < minValue) color.values[r] = minValue;
		if (color.values[r] > maxValue) color.values[r] = maxValue;

		if (color.values[g] < minValue) color.values[g] = minValue;
		if (color.values[g] > maxValue) color.values[g] = maxValue;

		if (color.values[b] < minValue) color.values[b] = minValue;
		if (color.values[b] > maxValue) color.values[b] = maxValue;

		if (color.values[a] < minValue) color.values[a] = minValue;
		if (color.values[a] > maxValue) color.values[a] = maxValue;
	}

	static Color Lerp(const Color& lhs, const Color& rhs, const float& t)
	{
		float OneMinusT = 1.0f - t;
		Color result;
		result.values[r] = (OneMinusT * lhs.values[r]) + (t * rhs.values[r]);
		result.values[g] = (OneMinusT * lhs.values[g]) + (t * rhs.values[g]);
		result.values[b] = (OneMinusT * lhs.values[b]) + (t * rhs.values[b]);
		result.values[a] = (OneMinusT * lhs.values[a]) + (t * rhs.values[a]);

		return result;
	}

private:
	float values[4];
};

#endif