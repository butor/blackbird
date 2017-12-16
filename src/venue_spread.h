#ifndef VENUE_SPREAD_H
#define VENUE_SPREAD_H

#include <ostream>
#include <ctime>
#include <string>
#include <list>


struct VenueSpread {
  
  unsigned id;
  unsigned idExchLong;
  unsigned idExchShort;
  //double exposure;
  double feesLong;
  double feesShort;
  std::time_t entryTime;
  std::time_t exitTime;
  std::string exchNameLong;
  std::string exchNameShort;
  double priceLongIn;
  double priceShortIn;
  //double priceLongOut;
  //double priceShortOut;
  double spreadIn;
  double spreadOut;
  double exitTarget;

  //VenueSpread();

};

#endif
