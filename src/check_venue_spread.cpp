#include "check_venue_spread.h"
#include "bitcoin.h"
#include "venue_spread.h"
#include "parameters.h"
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <cmath>



void checkVenueSpread(Bitcoin* btcLong, Bitcoin* btcShort, VenueSpread& res, Parameters& params) {
  
  // Gets the prices and computes the spread
  double priceLong = btcLong->getAsk();
  double priceShort = btcShort->getBid();
  // If the prices are null we return a null spread
  // to avoid false opportunities
  if (priceLong > 0.0 && priceShort > 0.0) {
    res.spreadIn = (priceShort - priceLong) / priceLong;
  } else {
    res.spreadIn = 0.0;
  }
  int longId = btcLong->getId();
  int shortId = btcShort->getId();

  // Updates the Result structure with the information about
  // the two trades and return True (meaning an opportunity
  // was found).
  res.idExchLong = longId;
  res.idExchShort = shortId;
  res.feesLong = btcLong->getFees();
  res.feesShort = btcShort->getFees();
  res.exchNameLong = btcLong->getExchName();
  res.exchNameShort = btcShort->getExchName();
  res.priceLongIn = priceLong;
  res.priceShortIn = priceShort;

}