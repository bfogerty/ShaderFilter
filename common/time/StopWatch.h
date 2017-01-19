#include <windows.h>

class StopWatch
{
public:
	StopWatch()
	{
		QueryPerformanceFrequency(&countsPerSecond);
		invCountsPerSecond = 1.0f / countsPerSecond.QuadPart;
	}

	inline void Start()
	{
		QueryPerformanceCounter(&beginCounts);
	}

	inline float Stop()
	{
		QueryPerformanceCounter(&endCounts);
		float delta = float(endCounts.QuadPart - beginCounts.QuadPart);
		float elapsedTimeInSeconds = delta * invCountsPerSecond;
		float elapsedTimeInMS = elapsedTimeInSeconds * 1000.0f;

		return elapsedTimeInMS;
	}

private:
	LARGE_INTEGER countsPerSecond;
	float invCountsPerSecond;
	LARGE_INTEGER beginCounts;
	LARGE_INTEGER endCounts;
};
