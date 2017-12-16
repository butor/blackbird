#ifndef CHECK_VENUE_SPREAD_H
#define CHECK_VENUE_SPREAD_H

#include <string>
#include <ctime>


class  Bitcoin;
struct VenueSpread;
struct Parameters;


// Checks for entry opportunity between two exchanges
// and returns True if an opporunity is found.
void checkVenueSpread(Bitcoin* btcLong, Bitcoin* btcShort, VenueSpread& res, Parameters& params);

#endif

