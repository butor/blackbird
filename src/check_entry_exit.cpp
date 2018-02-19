#include "check_entry_exit.h"
#include "bitcoin.h"
#include "result.h"
#include "parameters.h"
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <cmath>


// Not sure to understand what's going on here ;-)
template <typename T>
static typename std::iterator_traits<T>::value_type compute_sd(T first, const T &last) {
  using namespace std;
  typedef typename iterator_traits<T>::value_type value_type;
  
  auto n  = distance(first, last);
  auto mu = accumulate(first, last, value_type()) / n;
  auto squareSum = inner_product(first, last, first, value_type());
  return sqrt(squareSum / n - mu * mu);
}

// Returns a double as a string '##.##%'
std::string percToStr(double perc) {
  std::ostringstream s;
  if (perc >= 0.0) s << " ";
  s << std::fixed << std::setprecision(2) << perc * 100.0 << "%";
  return s.str();
}

bool checkEntry(Bitcoin* btcLong, Bitcoin* btcShort, Result &trade, Parameters& params) {
  
  if (!btcShort->getHasShort()) return false;

  // Gets the prices and computes the spread
  double priceLong = btcLong->getAsk();
  double priceShort = btcShort->getBid();
  // If the prices are null we return a null spread
  // to avoid false opportunities
  if (priceLong > 0.0 && priceShort > 0.0) {
    trade.spreadIn = (priceShort - priceLong) / priceLong;
  } else {
    trade.spreadIn = 0.0;
  }
  int longId = btcLong->getId();
  int shortId = btcShort->getId();

  // We update the max and min spread if necessary
  trade.maxSpread = std::max(trade.spreadIn, trade.maxSpread);
  trade.minSpread = std::min(trade.spreadIn, trade.minSpread);

  if (params.verbose) {
    params.logFile->precision(2);
    *params.logFile << "   " << btcLong->getExchName() << "/" << btcShort->getExchName() << ":\t" << percToStr(trade.spreadIn);
    *params.logFile << " [target " << percToStr(params.spreadEntry) << ", min " << percToStr(trade.minSpread) << ", max " << percToStr(trade.maxSpread) << "]";
    // The short-term volatility is computed and
    // displayed. No other action with it for
    // the moment.
    if (params.useVolatility) {
      if (trade.volatility.size() >= params.volatilityPeriod) {
        auto stdev = compute_sd(begin(trade.volatility), end(trade.volatility));
        *params.logFile << "  volat. " << stdev * 100.0 << "%";
      } else {
        *params.logFile << "  volat. n/a " << trade.volatility.size() << "<" << params.volatilityPeriod << " ";
      }
    }
    // Updates the trailing spread
    // TODO: explain what a trailing spread is.
    // See #12 on GitHub for the moment
    if (trade.trailing != -1.0) {
      *params.logFile << "   trailing " << percToStr(trade.trailing) << "  " << trade.trailingWaitCount << "/" << params.trailingCount;
    }
    // If one of the exchanges (or both) hasn't been implemented,
    // we mention in the log file that this spread is for info only.
    if ((!btcLong->getIsImplemented() || !btcShort->getIsImplemented()) && !params.demoMode)
      *params.logFile << "   info only";

    *params.logFile << std::endl;
  }
  // We need both exchanges to be implemented,
  // otherwise we return False regardless of
  // the opportunity found.
  if (!btcLong->getIsImplemented() ||
      !btcShort->getIsImplemented() ||
      trade.spreadIn == 0.0)
    return false;

  // the trailing spread is reset for this pair,
  // because once the spread is *below*
  // SpreadEndtry. Again, see #12 on GitHub for
  // more details.
  if (trade.spreadIn < params.spreadEntry) {
    trade.trailing = -1.0;
    trade.trailingWaitCount = 0;
    return false;
  }

  // Updates the trailingSpread with the new value
  double newTrailValue = trade.spreadIn - params.trailingLim;
  if (trade.trailing == -1.0) {
    trade.trailing = std::max(newTrailValue, params.spreadEntry);
    return false;
  }

  if (newTrailValue >= trade.trailing) {
    trade.trailing = newTrailValue;
    trade.trailingWaitCount = 0;
  }
  if (trade.spreadIn >= trade.trailing) {
    trade.trailingWaitCount = 0;
    return false;
  }

  if (trade.trailingWaitCount < params.trailingCount) {
    trade.trailingWaitCount++;
    return false;
  }

  // Updates the Result structure with the information about
  // the two trades and return True (meaning an opportunity
  // was found).
  trade.idExchLong = longId;
  trade.idExchShort = shortId;
  trade.feesLong = btcLong->getFees();
  trade.feesShort = btcShort->getFees();
  trade.exchNameLong = btcLong->getExchName();
  trade.exchNameShort = btcShort->getExchName();
  trade.priceLongIn = priceLong;
  trade.priceShortIn = priceShort;
  trade.exitTarget = trade.spreadIn - params.spreadTarget - 2.0*(trade.feesLong + trade.feesShort);
  trade.trailingWaitCount = 0;
  return true;
}

bool checkExit(Bitcoin* btcLong, Bitcoin* btcShort, Result &trade, Parameters& params, time_t period) {
  double priceLong  = btcLong->getBid();
  double priceShort = btcShort->getAsk();
  if (priceLong > 0.0 && priceShort > 0.0) {
    trade.spreadOut = (priceShort - priceLong) / priceLong;
  } else {
    trade.spreadOut = 0.0;
  }

  trade.maxSpread = std::max(trade.spreadOut, trade.maxSpread);
  trade.minSpread = std::min(trade.spreadOut, trade.minSpread);

  if (params.verbose) {
    params.logFile->precision(2);
    *params.logFile << "   " << btcLong->getExchName() << "/" << btcShort->getExchName() << ":\t" << percToStr(trade.spreadOut);
    *params.logFile << " [target " << percToStr(trade.exitTarget) << ", min " << percToStr(trade.minSpread) << ", max " << percToStr(trade.maxSpread) << "]";
    // The short-term volatility is computed and
    // displayed. No other action with it for
    // the moment.
    if (params.useVolatility) {
      if (trade.volatility.size() >= params.volatilityPeriod) {
        auto stdev = compute_sd(begin(trade.volatility), end(trade.volatility));
        *params.logFile << "  volat. " << stdev * 100.0 << "%";
      } else {
        *params.logFile << "  volat. n/a " << trade.volatility.size() << "<" << params.volatilityPeriod << " ";
      }
    }
    if (trade.trailing != 1.0) {
      *params.logFile << "   trailing " << percToStr(trade.trailing) << "  " << trade.trailingWaitCount << "/" << params.trailingCount;
    }
  }
  *params.logFile << std::endl;
  if (period - trade.entryTime >= int(params.maxLength)) {
    trade.priceLongOut  = priceLong;
    trade.priceShortOut = priceShort;
    return true;
  }
  if (trade.spreadOut == 0.0) return false;
  if (trade.spreadOut > trade.exitTarget) {
    trade.trailing = 1.0;
    trade.trailingWaitCount = 0;
    return false;
  }

  double newTrailValue = trade.spreadOut + params.trailingLim;
  if (trade.trailing == 1.0) {
    trade.trailing = std::min(newTrailValue, trade.exitTarget);
    return false;
  }
  if (newTrailValue <= trade.trailing) {
    trade.trailing = newTrailValue;
    trade.trailingWaitCount = 0;
  }
  if (trade.spreadOut <= trade.trailing) {
    trade.trailingWaitCount = 0;
    return false;
  }
  if (trade.trailingWaitCount < params.trailingCount) {
    trade.trailingWaitCount++;
    return false;
  }

  trade.priceLongOut  = priceLong;
  trade.priceShortOut = priceShort;
  trade.trailingWaitCount = 0;
  return true;
}
