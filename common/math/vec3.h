#ifndef __VEC3__
#define __VEC3__
#include <memory.h>
#include <math.h>

class Vec3
{
	const int ValueSize = sizeof(float) * 3;

	enum XYZ
	{
		x = 0,
		y = 1,
		z = 2
	};

public:
	Vec3()
	{
		memset(values, 0.0f, ValueSize);
	}

	Vec3(float xVal, float yVal, float zVal)
	{
		values[x] = xVal;
		values[y] = yVal;
		values[z] = zVal;
	}

	Vec3(const Vec3& other)
	{
		memcpy(values, other.values, ValueSize);
	}

	Vec3 operator +(const Vec3& rhs)
	{
		Vec3 result(values[x] + rhs.values[x],
					values[y] + rhs.values[y],
					values[z] + rhs.values[z]);

		return result;

	}

	Vec3 operator -(const Vec3& rhs)
	{
		Vec3 result(values[x] - rhs.values[x],
					values[y] - rhs.values[y],
					values[z] - rhs.values[z]);

		return result;

	}

	Vec3 operator *(const float& rhs)
	{
		Vec3 result(values[x] * rhs,
					values[y] * rhs,
					values[z] * rhs);

		return result;

	}

	Vec3 operator /(const float& rhs)
	{
		Vec3 result(values[x] / rhs,
					values[y] / rhs,
					values[z] / rhs);

		return result;
	}

	float MagnitudeSqaured()
	{
		float result =
			values[x] * values[x] +
			values[y] * values[y] +
			values[z] * values[z];

		return result;
	}

	float Magnitude()
	{
		float result = sqrt(
			values[x] * values[x] +
			values[y] * values[y] +
			values[z] * values[z]
			);

		return result;
	}

	float InverseMagnitude()
	{
		float result = sqrt(
			values[x] * values[x] +
			values[y] * values[y] +
			values[z] * values[z]
			);

		return 1.0f / result;
	}

	float InverseMagnitudeSquared()
	{
		float result =
			values[x] * values[x] +
			values[y] * values[y] +
			values[z] * values[z];

		return 1.0f / result;
	}

	void Normalize()
	{
		float invMagnitude = InverseMagnitude();

		values[x] = values[x] * invMagnitude;
		values[y] = values[y] * invMagnitude;
		values[z] = values[z] * invMagnitude;
	}

	Vec3 GetNormalized()
	{
		float invMagnitude = InverseMagnitude();

		return Vec3(values[x] * invMagnitude, values[y] * invMagnitude, values[z] * invMagnitude);
	}

	float* GetValues()
	{
		return values;
	}

	static float Dot(const Vec3& lhs, const Vec3& rhs)
	{
		float result =	lhs.values[x] * rhs.values[x] +
						lhs.values[y] * rhs.values[y] +
						lhs.values[z] * rhs.values[z];

		return result;
	}

	static Vec3 Cross(const Vec3& lhs, const Vec3& rhs)
	{
		Vec3 result;

		result.values[x] = rhs.values[y] * lhs.values[z] - rhs.values[z] * lhs.values[y];
		result.values[y] = rhs.values[z] * lhs.values[x] - rhs.values[x] * lhs.values[z];
		result.values[z] = rhs.values[x] * lhs.values[y] - rhs.values[y] * lhs.values[x];

		return result;
	}

	static Vec3 Project(const Vec3& a, const Vec3& b)
	{
		Vec3 result = b * (Vec3::Dot(a,b) * b.InverseMagnitudeSquared());

		return result;
	}

	static float Length(const Vec3& lhs, const Vec3& rhs)
	{
		float result = sqrt(
			lhs.values[x] * rhs.values[x] +
			lhs.values[y] * rhs.values[y] +
			lhs.values[z] * rhs.values[z]
			);

		return result;
	}

	static Vec3 Lerp(const Vec3& lhs, const Vec3& rhs, const float& t)
	{
		float OneMinusT = 1.0f - t;
		Vec3 result;
		result.values[x] = (OneMinusT * lhs.values[x]) + (t * rhs.values[x]);
		result.values[y] = (OneMinusT * lhs.values[y]) + (t * rhs.values[y]);
		result.values[z] = (OneMinusT * lhs.values[z]) + (t * rhs.values[z]);

		return result;
	}


private:
	float values[3];
};

#endif