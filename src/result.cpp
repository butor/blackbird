#include "result.h"
#include "time_fun.h"
#include <type_traits>
#include <iostream>
#include <fstream>


double Result::targetPerfLong() const {
  return (priceLongOut - priceLongIn) / priceLongIn - 2.0 * feesLong;
}

double Result::targetPerfShort() const {
  return (priceShortIn - priceShortOut) / priceShortIn - 2.0 * feesShort;
}

double Result::actualPerf() const {
  if (exposure == 0.0) return 0.0;
  // The performance is computed as an "actual" performance,
  // by simply comparing what amount was on our leg2 account
  // before, and after, the arbitrage opportunity. This way,
  // we are sure that every fees was taking out of the performance
  // computation. Hence the "actual" performance.
  return (leg2TotBalanceAfter - leg2TotBalanceBefore) / (exposure * 2.0);
}

double Result::getTradeLengthInMinute() const {
  if (entryTime > 0 && exitTime > 0) return (exitTime - entryTime) / 60.0;
  return 0;
}

void Result::printEntryInfo(std::ostream &logFile) const {
  logFile.precision(2);
  logFile << "\n[ ENTRY FOUND ]" << std::endl;
  logFile << "   Date & Time:       "  << printDateTime(entryTime) << std::endl;
  logFile << "   Exchange Long:     "  << exchNameLong <<  " (id " << idExchLong  << ")" << std::endl;
  logFile << "   Exchange Short:    "  << exchNameShort << " (id " << idExchShort << ")" << std::endl;
  logFile << "   Fees:              "  << feesLong * 100.0 << "% / " << feesShort * 100.0 << "%" << std::endl;
  logFile << "   Price Long:        " << priceLongIn << " (target)" << std::endl;
  logFile << "   Price Short:       " << priceShortIn << " (target)" << std::endl;
  logFile << "   Spread:            "  << spreadIn * 100.0 << "%" << std::endl;
  logFile << "   Cash used:         " << exposure << " on each exchange" << std::endl;
  logFile << "   Exit Target:       "  << exitTarget * 100.0 << "%" << std::endl;
  logFile << std::endl;
}

void Result::printExitInfo(std::ostream &logFile) const {
  logFile.precision(2);
  logFile << "\n[ EXIT FOUND ]" << std::endl;
  logFile << "   Date & Time:       "  << printDateTime(exitTime) << std::endl;
  logFile << "   Duration:          "  << getTradeLengthInMinute() << " minutes" << std::endl;
  logFile << "   Price Long:        " << priceLongOut << " (target)" << std::endl;
  logFile << "   Price Short:       " << priceShortOut << " (target)" << std::endl;
  logFile << "   Spread:            "  << spreadOut * 100.0 << "%" << std::endl;
  logFile << "   ---------------------------"  << std::endl;
  logFile << "   Target Perf Long:  "  << targetPerfLong()  * 100.0 << "% (fees incl.)" << std::endl;
  logFile << "   Target Perf Short: "  << targetPerfShort() * 100.0 << "% (fees incl.)" << std::endl;
  logFile << "   ---------------------------\n"  << std::endl;
}

// not sure to understand how this function is implemented ;-)
void Result::reset() {

  Result tmp {};
  std::swap(tmp, *this);

}

void Result::removePair(std::vector<Result>& vec, int id){
  vec.erase(
    std::remove_if(vec.begin(), vec.end(), [&](Result const & idExchLong){
      return idExchLong.idExchLong == id;
    }),
    vec.end());
}
