#ifndef _UNISTD_H
#define _UNISTD_H    1

#include <WinSock2.h>
#include <Windows.h>
#include <getopt.h> /* getopt at: https://gist.github.com/ashelly/7776712 */
#include <process.h> /* for getpid() and the exec..() family */
#include <time.h>

#define sleep(seconds) ::Sleep(seconds * 1000)

typedef unsigned __int64  uint64_t;

inline int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	::GetSystemTime(&system_time);
	::SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

inline struct tm *gmtime_r(time_t *_clock, struct tm *_result)
{
	struct tm *p = gmtime(_clock);
	*(_result) = *p;
	return _result;
}

inline struct tm *localtime_r(time_t *_clock, struct tm *_result)
{
	struct tm *p = localtime(_clock);
	*(_result) = *p;
	return _result;
}

#endif /* unistd.h  */