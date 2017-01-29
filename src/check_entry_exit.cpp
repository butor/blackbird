#include "check_entry_exit.h"
#include "bitcoin.h"
#include "result.h"
#include "parameters.h"
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <algorithm>

template <typename T>
static typename std::iterator_traits<T>::value_type compute_sd(T first, const T &last)
{
  using namespace std;
  typedef typename iterator_traits<T>::value_type value_type;
  
  auto n  = distance(first, last);
  auto mu = accumulate(first, last, value_type()) / n;
  auto squareSum = inner_product(first, last, first, value_type());
  return sqrt(squareSum / n - mu * mu);
}

std::string percToStr(double perc)
{
  std::ostringstream s;
  if (perc >= 0.0) s << " ";
  s << std::fixed << std::setprecision(2) << perc * 100.0 << "%";

  return s.str();
}

bool checkEntry(Bitcoin* btcLong, Bitcoin* btcShort, Result& res, Parameters& params)
{
  if (!btcShort->getHasShort()) return false;

  double priceLong = btcLong->getAsk();
  double priceShort = btcShort->getBid();
  if (priceLong > 0.0 && priceShort > 0.0) {
    res.spreadIn = (priceShort - priceLong) / priceLong;
  } else {
    res.spreadIn = 0.0;
  }
  int longId = btcLong->getId();
  int shortId = btcShort->getId();

  res.maxSpread[longId][shortId] = std::max(res.spreadIn, res.maxSpread[longId][shortId]);
  res.minSpread[longId][shortId] = std::min(res.spreadIn, res.minSpread[longId][shortId]);

  if (params.verbose) {
    *params.logFile << "   " << btcLong->getExchName() << "/" << btcShort->getExchName() << ":\t" << percToStr(res.spreadIn);
    *params.logFile << " [target " << percToStr(params.spreadEntry) << ", min " << percToStr(res.minSpread[longId][shortId]) << ", max " << percToStr(res.maxSpread[longId][shortId]) << "]";
    if (params.useVolatility) {
      if (res.volatility[longId][shortId].size() >= params.volatilityPeriod) {
        auto stdev = compute_sd(begin(res.volatility[longId][shortId]), end(res.volatility[longId][shortId]));
        *params.logFile << "  volat. " << stdev * 100.0 << "%";
      } else {
        *params.logFile << "  volat. n/a " << res.volatility[longId][shortId].size() << "<" << params.volatilityPeriod << " ";
      }
    }
    if (res.trailing[longId][shortId] != -1.0) {
      *params.logFile << "   trailing " << percToStr(res.trailing[longId][shortId]) << "  " << res.trailingWaitCount[longId][shortId] << "/" << params.trailingCount;
    }
    if ((btcLong->getIsImplemented() == false || btcShort->getIsImplemented() == false) && params.demoMode == false) {
      *params.logFile << "   info only"  << std::endl;
    } else {
      *params.logFile << std::endl;
    }
  }
  if (!btcLong->getIsImplemented() ||
      !btcShort->getIsImplemented() ||
      res.spreadIn == 0.0)
    return false;

  if (res.spreadIn >= params.spreadEntry) {
    double newTrailValue = res.spreadIn - params.trailingLim;
    if (res.trailing[longId][shortId] == -1.0) {
      res.trailing[longId][shortId] = std::max(newTrailValue, params.spreadEntry);
    } else {
      if (newTrailValue >= res.trailing[longId][shortId]) {
        res.trailing[longId][shortId] = newTrailValue;
        res.trailingWaitCount[longId][shortId] = 0;
      }
      if (res.spreadIn < res.trailing[longId][shortId]) {
        if (res.trailingWaitCount[longId][shortId] < params.trailingCount) {
          res.trailingWaitCount[longId][shortId]++;
        } else {
          res.idExchLong = longId;
          res.idExchShort = shortId;
          res.feesLong = btcLong->getFees();
          res.feesShort = btcShort->getFees();
          res.exchNameLong = btcLong->getExchName();
          res.exchNameShort = btcShort->getExchName();
          res.priceLongIn = priceLong;
          res.priceShortIn = priceShort;
          res.exitTarget = res.spreadIn - params.spreadTarget - (2.0 * btcLong->getFees() + 2.0 * btcShort->getFees());
          res.trailingWaitCount[longId][shortId] = 0;
          return true;
        }
      } else {
        res.trailingWaitCount[longId][shortId] = 0;
      }
    }
  } else {
    res.trailing[longId][shortId] = -1.0;
    res.trailingWaitCount[longId][shortId] = 0;
  }

  return false;
}

bool checkExit(Bitcoin* btcLong, Bitcoin* btcShort, Result& res, Parameters& params, time_t period) {
  double priceLong  = btcLong->getBid();
  double priceShort = btcShort->getAsk();
  if (priceLong > 0.0 && priceShort > 0.0) {
    res.spreadOut = (priceShort - priceLong) / priceLong;
  } else {
    res.spreadOut = 0.0;
  }
  int longId = btcLong->getId();
  int shortId = btcShort->getId();

  res.maxSpread[longId][shortId] = std::max(res.spreadOut, res.maxSpread[longId][shortId]);
  res.minSpread[longId][shortId] = std::min(res.spreadOut, res.minSpread[longId][shortId]);

  if (params.verbose) {
    *params.logFile << "   " << btcLong->getExchName() << "/" << btcShort->getExchName() << ":\t" << percToStr(res.spreadOut);
    *params.logFile << " [target " << percToStr(res.exitTarget) << ", min " << percToStr(res.minSpread[longId][shortId]) << ", max " << percToStr(res.maxSpread[longId][shortId]) << "]";
    if (params.useVolatility) {
      if (res.volatility[longId][shortId].size() >= params.volatilityPeriod) {
        auto stdev = compute_sd(begin(res.volatility[longId][shortId]), end(res.volatility[longId][shortId]));
        *params.logFile << "  volat. " << stdev * 100.0 << "%";
      } else {
        *params.logFile << "  volat. n/a " << res.volatility[longId][shortId].size() << "<" << params.volatilityPeriod << " ";
      }
    }
    if (res.trailing[longId][shortId] != 1.0) {
      *params.logFile << "   trailing " << percToStr(res.trailing[longId][shortId]) << "  " << res.trailingWaitCount[longId][shortId] << "/" << params.trailingCount;
    }
  }
  *params.logFile << std::endl;
  if (period - res.entryTime >= int(params.maxLength)) {
    res.priceLongOut  = priceLong;
    res.priceShortOut = priceShort;
    return true;
  }
  if (res.spreadOut == 0.0) return false;

  if (res.spreadOut <= res.exitTarget) {
    double newTrailValue = res.spreadOut + params.trailingLim;
    if (res.trailing[longId][shortId] == 1.0) {
      res.trailing[longId][shortId] = std::min(newTrailValue, res.exitTarget);
    } else {
      if (newTrailValue <= res.trailing[longId][shortId]) {
        res.trailing[longId][shortId] = newTrailValue;
        res.trailingWaitCount[longId][shortId] = 0;
      }
      if (res.spreadOut > res.trailing[longId][shortId]) {
        if (res.trailingWaitCount[longId][shortId] < params.trailingCount) {
          res.trailingWaitCount[longId][shortId]++;
        } else {
          res.priceLongOut  = priceLong;
          res.priceShortOut = priceShort;
          res.trailingWaitCount[longId][shortId] = 0;
          return true;
        }
      } else {
        res.trailingWaitCount[longId][shortId] = 0;
      }
    }
  } else {
    res.trailing[longId][shortId] = 1.0;
    res.trailingWaitCount[longId][shortId] = 0;
  }

  return false;
}
