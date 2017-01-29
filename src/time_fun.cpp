#include <iostream>
#include "time_fun.h"

time_t getTime_t(int y, int m, int d, int h, int n, int s) {
  tm ttm = {0};
  ttm.tm_year = y - 1900;
  ttm.tm_mon = m - 1;
  ttm.tm_mday = d;
  ttm.tm_hour = h;
  ttm.tm_min = n;
  ttm.tm_sec = s;
  ttm.tm_isdst = -1;
  return mktime(&ttm);
}

std::string printDateTime(time_t t) {
  struct tm timeinfo;
  char buff[20];
  strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime_r(&t, &timeinfo));
  std::string str(buff);
  return str;
}

std::string printDateTime() {
  time_t now = time(NULL);
  struct tm timeinfo;
  char buff[20];
  strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime_r(&now, &timeinfo));
  std::string str(buff);
  return str;
}

std::string printDateTimeCsv(time_t t) {
  struct tm timeinfo;
  char buff[20];
  strftime(buff, 20, "%Y-%m-%d_%H:%M:%S", localtime_r(&t, &timeinfo));
  std::string str(buff);
  return str;
}

std::string printDateTimeDb(time_t t) {
  struct tm timeinfo;
  char buff[20];
  strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &timeinfo));
  std::string str(buff);
  return str;
}

std::string printDateTimeFileName() {
  time_t now = time(NULL);
  struct tm timeinfo;
  char buff[20];
  strftime(buff, 20, "%Y%m%d_%H%M%S", localtime_r(&now, &timeinfo));
  std::string str(buff);
  return str;
}

