#ifndef TIME_FUN_H
#define TIME_FUN_H

#include <string>
#include <time.h>
#include <unistd.h>

time_t getTime_t(int y, int m, int d, int h, int n, int s);

// return 'mm/dd/yyyy hh:nn:ss'
std::string printDateTime(time_t t);

// return current 'mm/dd/yyyy hh:mm:ss'
std::string printDateTime();

// return 'yyyy-mm-dd_hh:nn:ss'
std::string printDateTimeCsv(time_t t);

// return 'yyyy-mm-dd hh:nn:ss'
std::string printDateTimeDb(time_t t);

// return 'yyymmdd_hhnnss'
std::string printDateTimeFileName();

#endif

