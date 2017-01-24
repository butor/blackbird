#ifndef CHECK_ENTRY_EXIT_H
#define CHECK_ENTRY_EXIT_H

#include <string>
#include <ctime>


struct Bitcoin;
struct Result;
struct Parameters;

std::string percToStr(double perc);

// check for entry opportunity between two exchanges
bool checkEntry(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters &params);

// check for exit opportunity between two exchanges
bool checkExit(Bitcoin *btcLong, Bitcoin *btcShort, Result &res, Parameters &params, std::time_t period);

#endif

