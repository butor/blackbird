#include "bitcoin.h"
#include "result.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "parameters.h"
#include "exchange/exchanges.h"
#include "db_fun.h"
#include "check_entry_exit.h"
#include "utils/send_email.h"

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <unistd.h>   // getpid
#include <math.h>
#include <algorithm>


// typedef declarations needed for the function arrays

struct Balance
{
  double btc, usd;
  double btcAfter, usdAfter;
};

int main(int argc, char** argv) {
  std::cout << "Blackbird Bitcoin Arbitrage" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK\n" << std::endl;
  std::locale mylocale("");
  // load the parameters
  Parameters params("blackbird.conf");
  if (!params.demoMode) {
    if (!params.useFullCash) {
      if (params.cashForTesting < 10.0) {
        std::cout << "ERROR: Minimum test cash needed: $10.00" << std::endl;
        std::cout << "       Otherwise some exchanges will reject the orders\n" << std::endl;
        return -1;
      }
      if (params.cashForTesting > params.maxExposure) {
        std::cout << "ERROR: Test cash ($" << params.cashForTesting << ") is above max exposure ($" << params.maxExposure << ")\n" << std::endl;
        return -1;
      }
    }
  }

  if (createDbConnection(params) != 0)
  {
    std::cerr << "ERROR: cannot connect to the database \'" << params.dbFile << "\'\n" << std::endl;
    return -1;
  }

  // init exchanges 
  int index = initExchanges(params);
  // add the exchange functions to the arrays for all the defined exchanges
  
  if (index < 2) {
    std::cout << "ERROR: Blackbird needs at least two Bitcoin exchanges. Please edit the config.json file to add new exchanges\n" << std::endl;
    return -1;
  }

  // create the csv file
  std::string currDateTime = printDateTimeFileName();
  std::string csvFileName = "blackbird_result_" + currDateTime + ".csv";
  std::ofstream csvFile(csvFileName, std::ofstream::trunc);
  csvFile << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,"
          << "TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN"
          << std::endl;
  // create the log file
  std::string logFileName = "blackbird_log_" + currDateTime + ".log";
  std::ofstream logFile(logFileName, std::ofstream::trunc);
  logFile.imbue(mylocale);
  logFile.precision(2);
  logFile << std::fixed;
  params.logFile = &logFile;
  logFile << "--------------------------------------------" << std::endl;
  logFile << "|   Blackbird Bitcoin Arbitrage Log File   |" << std::endl;
  logFile << "--------------------------------------------\n" << std::endl;
  logFile << "Blackbird started on " << printDateTime() << "\n" << std::endl;

  logFile << "Connected to database \'" << params.dbFile << "\'\n" << std::endl;

  if (params.demoMode) {
    logFile << "Demo mode: trades won't be generated\n" << std::endl;
  }
  std::cout << "Log file generated: " << logFileName << "\nBlackbird is running... (pid " << getpid() << ")\n" << std::endl;
  int numExch = params.nbExch();
  std::vector<Bitcoin> btcVec;
  btcVec.reserve(numExch);
  // create a new Bitcoin structure within btcVec for every exchange
  for (int i = 0; i < numExch; ++i) {
    btcVec.push_back(Bitcoin(i, params.exchName[i], params.fees[i], params.canShort[i], params.isImplemented[i]));
  }

  params.curl = curl_easy_init();
  logFile << "[ Targets ]\n"
          << std::setprecision(2)
          << "   Spread Entry:  " << params.spreadEntry * 100.0 << "%\n"
          << "   Spread Target: " << params.spreadTarget * 100.0  << "%\n";
  if (params.spreadEntry <= 0.0) {
    logFile << "   WARNING: Spread Entry should be positive" << std::endl;
  }
  if (params.spreadTarget <= 0.0) {
    logFile << "   WARNING: Spread Target should be positive" << std::endl;
  }
  logFile << std::endl;
  logFile << "[ Current balances ]" << std::endl;
  // get the BTC and USD balances for every exchange
  std::vector<Balance> balance(numExch);
  if (!params.demoMode)
    std::transform(getAvail, getAvail + numExch,
                   begin(balance),
                   [&params]( decltype(*getAvail) apply )
                   {
                     Balance tmp {};
                     tmp.btc = apply(params, "btc");
                     tmp.usd = apply(params, "usd");
                     return tmp;
                   } );

  // Check for restore.txt to see if the program exited with an open position.
  Result res;
  res.reset();
  bool inMarket = res.loadPartialResult("restore.txt");

  // write the balances into the log file
  for (int i = 0; i < numExch; ++i) {
    logFile << "   " << params.exchName[i] << ":\t";
    if (params.demoMode) {
      logFile << "n/a (demo mode)" << std::endl;
    } else if (!params.isImplemented[i]) {
      logFile << "n/a (API not implemented)" << std::endl;
    } else {
      logFile << std::setprecision(2) << balance[i].usd << " USD\t"
              << std::setprecision(6) << balance[i].btc << " BTC" << std::endl;
    }
    if (balance[i].btc > 0.0050 && !inMarket) {
      logFile << "ERROR: All BTC accounts must be empty before starting Blackbird" << std::endl;
      return -1;
    }
  }
  logFile << std::endl;
  logFile << "[ Cash exposure ]\n";
  if (params.demoMode) {
    logFile << "   No cash - Demo mode\n";
  } else {
    if (params.useFullCash) {
      logFile << "   FULL cash used!\n";
    } else {
      logFile << "   TEST cash used\n   Value: $"
              << std::setprecision(2) << params.cashForTesting << '\n';
    }
  }
  logFile << std::endl;
  time_t rawtime = time(nullptr);
  tm timeinfo = *localtime(&rawtime);
  using std::this_thread::sleep_for;
  using millisecs = std::chrono::milliseconds;
  using secs      = std::chrono::seconds;
  // wait for the next 'gapSec' seconds before starting
  while ((int)timeinfo.tm_sec % params.gapSec != 0) {
    sleep_for(millisecs(100));
    time(&rawtime);
    timeinfo = *localtime(&rawtime);
  }
  if (!params.verbose) {
    logFile << "Running..." << std::endl;
  }

  int resultId = 0;
  unsigned currIteration = 0;
  bool stillRunning = true;
  time_t currTime;
  time_t diffTime;

  // main analysis loop
  while (stillRunning) {
    currTime = mktime(&timeinfo);
    time(&rawtime);
    diffTime = difftime(rawtime, currTime);
    // check if we are already too late
    // if that's the case we wait until the next iteration
    if (diffTime > 0) {
      logFile << "WARNING: " << diffTime << " second(s) too late at " << printDateTime(currTime) << std::endl;
      timeinfo.tm_sec += (ceil(diffTime / params.gapSec) + 1) * params.gapSec;
      currTime = mktime(&timeinfo);
      sleep_for(secs(params.gapSec - (diffTime % params.gapSec)));
      logFile << std::endl;
    } else if (diffTime < 0) {
      sleep_for(secs(-diffTime));
    }
    if (params.verbose) {
      if (!inMarket) {
        logFile << "[ " << printDateTime(currTime) << " ]" << std::endl;
      } else {
        logFile << "[ " << printDateTime(currTime) << " IN MARKET: Long " << res.exchNameLong << " / Short " << res.exchNameShort << " ]" << std::endl;
      }
    }
    // get the bid and ask of all the exchanges
    for (int i = 0; i < numExch; ++i) {
      auto quote = getQuote[i](params);
      double bid = quote.bid();
      double ask = quote.ask();

      addBidAskToDb(dbTableName[i], printDateTimeDb(currTime), bid, ask, params);

      if (bid == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " bid is null" << std::endl;
      }
      if (ask == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " ask is null" << std::endl;
      }
      if (params.verbose) {
        logFile << "   " << params.exchName[i] << ": \t"
                << std::setprecision(2)
                << bid << " / " << ask << std::endl;
      }
      btcVec[i].updateData(quote);
      curl_easy_reset(params.curl);
    }
    if (params.verbose) {
      logFile << "   ----------------------------" << std::endl;
    }
    // store all the spreads
    // will be used later to compute the volatility
    if (params.useVolatility) {
      for (int i = 0; i < numExch; ++i) {
        for (int j = 0; j < numExch; ++j) {
          if (i != j) {
            if (btcVec[j].getHasShort()) {
              double longMidPrice = btcVec[i].getMidPrice();
              double shortMidPrice = btcVec[j].getMidPrice();
              if (longMidPrice > 0.0 && shortMidPrice > 0.0) {
                if (res.volatility[i][j].size() >= params.volatilityPeriod) {
                  res.volatility[i][j].pop_back();
                }
                res.volatility[i][j].push_front((longMidPrice - shortMidPrice) / longMidPrice);
              }
            }
          }
        }
      }
    }
    // look for arbitrage opportunities on all the exchange combinations
    if (!inMarket) {
      for (int i = 0; i < numExch; ++i) {
        for (int j = 0; j < numExch; ++j) {
          if (i != j) {
            if (checkEntry(&btcVec[i], &btcVec[j], res, params)) {
              // entry opportunity has been found
              res.exposure = std::min(balance[res.idExchLong].usd, balance[res.idExchShort].usd);
              if (params.demoMode) {
                logFile << "INFO: Opportunity found but no trade will be generated (Demo mode)" << std::endl;
                break;
              }
              if (res.exposure == 0.0) {
                logFile << "WARNING: Opportunity found but no cash available. Trade canceled" << std::endl;
                break;
              }
              if (params.useFullCash == false && res.exposure <= params.cashForTesting) {
                logFile << "WARNING: Opportunity found but no enough cash. Need more than TEST cash (min. $"
                        << std::setprecision(2) << params.cashForTesting << "). Trade canceled" << std::endl;
                break;
              }
              if (params.useFullCash) {
                // remove 1% of the cash
                res.exposure -= 0.01 * res.exposure;
                if (res.exposure > params.maxExposure) {
                  logFile << "WARNING: Opportunity found but exposure ($"
                          << std::setprecision(2)
                          << res.exposure << ") above the limit\n"
                          << "         Max exposure will be used instead ($" << params.maxExposure << ")" << std::endl;
                  res.exposure = params.maxExposure;
                }
              } else {
                res.exposure = params.cashForTesting;
              }
              // check the volumes and compute the limit prices that will be sent to the exchanges
              double volumeLong = res.exposure / btcVec[res.idExchLong].getAsk();
              double volumeShort = res.exposure / btcVec[res.idExchShort].getBid();
              double limPriceLong = getLimitPrice[res.idExchLong](params, volumeLong, false);
              double limPriceShort = getLimitPrice[res.idExchShort](params, volumeShort, true);
              if (limPriceLong == 0.0 || limPriceShort == 0.0) {
                logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
                logFile.precision(2);
                logFile << "         Long limit price:  " << limPriceLong << std::endl;
                logFile << "         Short limit price: " << limPriceShort << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              if (limPriceLong - res.priceLongIn > params.priceDeltaLim || res.priceShortIn - limPriceShort > params.priceDeltaLim) {
                logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
                logFile.precision(2);
                logFile << "         Target long price:  " << res.priceLongIn << ", Real long price:  " << limPriceLong << std::endl;
                logFile << "         Target short price: " << res.priceShortIn << ", Real short price: " << limPriceShort << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              inMarket = true;
              resultId++;
              res.id = resultId;
              res.entryTime = currTime;
              res.priceLongIn = limPriceLong;
              res.priceShortIn = limPriceShort;
              res.printEntryInfo(*params.logFile);
              res.maxSpread[res.idExchLong][res.idExchShort] = -1.0;
              res.minSpread[res.idExchLong][res.idExchShort] = 1.0;
              res.trailing[res.idExchLong][res.idExchShort] = 1.0;

              auto longOrderId = sendLongOrder[res.idExchLong](params, "buy", volumeLong, limPriceLong);
              auto shortOrderId = sendShortOrder[res.idExchShort](params, "sell", volumeShort, limPriceShort);
              logFile << "Waiting for the two orders to be filled..." << std::endl;
              sleep_for(millisecs(5000));
              bool isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
              bool isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
              while (!isLongOrderComplete || !isShortOrderComplete) {
                sleep_for(millisecs(3000));
                if (!isLongOrderComplete) {
                  logFile << "Long order on " << params.exchName[res.idExchLong] << " still open..." << std::endl;
                  isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
                }
                if (!isShortOrderComplete) {
                  logFile << "Short order on " << params.exchName[res.idExchShort] << " still open..." << std::endl;
                  isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
                }
              }
              logFile << "Done" << std::endl;

              // Store the partial result to file in case
              // the program exits before closing the position.
              res.savePartialResult("restore.txt");

              longOrderId  = "0";
              shortOrderId = "0";
              break;
            }
          }
        }
        if (inMarket) {
          break;
        }
      }
      if (params.verbose) {
        logFile << std::endl;
      }
    } else if (inMarket) {
      // in market, looking for an exit opportunity
      if (checkExit(&btcVec[res.idExchLong], &btcVec[res.idExchShort], res, params, currTime)) {
        // exit opportunity has been found
        // check current BTC exposure
        std::vector<double> btcUsed(numExch);
        for (int i = 0; i < numExch; ++i) {
          btcUsed[i] = getActivePos[i](params);
        }
        // check the volumes and compute the limit prices that will be sent to the exchanges
        double volumeLong = btcUsed[res.idExchLong];
        double volumeShort = btcUsed[res.idExchShort];
        double limPriceLong = getLimitPrice[res.idExchLong](params, volumeLong, true);
        double limPriceShort = getLimitPrice[res.idExchShort](params, volumeShort, false);
        if (limPriceLong == 0.0 || limPriceShort == 0.0) {
          logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
          logFile.precision(2);
          logFile << "         Long limit price:  " << limPriceLong << std::endl;
          logFile << "         Short limit price: " << limPriceShort << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else if (res.priceLongOut - limPriceLong > params.priceDeltaLim || limPriceShort - res.priceShortOut > params.priceDeltaLim) {
          logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
          logFile.precision(2);
          logFile << "         Target long price:  " << res.priceLongOut << ", Real long price:  " << limPriceLong << std::endl;
          logFile << "         Target short price: " << res.priceShortOut << ", Real short price: " << limPriceShort << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else {
          res.exitTime = currTime;
          res.priceLongOut = limPriceLong;
          res.priceShortOut = limPriceShort;
          res.printExitInfo(*params.logFile);

          logFile.precision(6);
          logFile << "BTC exposure on " << params.exchName[res.idExchLong] << ": " << volumeLong << '\n'
                  << "BTC exposure on " << params.exchName[res.idExchShort] << ": " << volumeShort << '\n'
                  << std::endl;
          auto longOrderId = sendLongOrder[res.idExchLong](params, "sell", fabs(btcUsed[res.idExchLong]), limPriceLong);
          auto shortOrderId = sendShortOrder[res.idExchShort](params, "buy", fabs(btcUsed[res.idExchShort]), limPriceShort);
          logFile << "Waiting for the two orders to be filled..." << std::endl;
          sleep_for(millisecs(5000));
          bool isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
          bool isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
          while (!isLongOrderComplete || !isShortOrderComplete) {
            sleep_for(millisecs(3000));
            if (!isLongOrderComplete) {
              logFile << "Long order on " << params.exchName[res.idExchLong] << " still open..." << std::endl;
              isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
            }
            if (!isShortOrderComplete) {
              logFile << "Short order on " << params.exchName[res.idExchShort] << " still open..." << std::endl;
              isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
            }
          }
          logFile << "Done\n" << std::endl;
          longOrderId  = "0";
          shortOrderId = "0";
          inMarket = false;
          for (int i = 0; i < numExch; ++i) {
            balance[i].usdAfter = getAvail[i](params, "usd");
            balance[i].btcAfter = getAvail[i](params, "btc");
          }
          for (int i = 0; i < numExch; ++i) {
            logFile << "New balance on " << params.exchName[i] << ":  \t";
            logFile.precision(2);
            logFile << balance[i].usdAfter << " USD (perf $" << balance[i].usdAfter - balance[i].usd << "), ";
            logFile << std::setprecision(6) << balance[i].btcAfter << " BTC\n";
          }
          logFile << std::endl;
          // update total USD balance
          for (int i = 0; i < numExch; ++i) {
            res.usdTotBalanceBefore += balance[i].usd;
            res.usdTotBalanceAfter += balance[i].usdAfter;
          }
          // update current balances
          for (int i = 0; i < numExch; ++i) {
            balance[i].usd = balance[i].usdAfter;
            balance[i].btc = balance[i].btcAfter;
          }
          logFile.precision(2);
          logFile << "ACTUAL PERFORMANCE: " << "$" << res.usdTotBalanceAfter - res.usdTotBalanceBefore << " (" << res.actualPerf() * 100.0 << "%)\n" << std::endl;
          csvFile << res.id << ","
                  << res.exchNameLong << ","
                  << res.exchNameShort << ","
                  << printDateTimeCsv(res.entryTime) << ","
                  << printDateTimeCsv(res.exitTime) << ","
                  << res.getTradeLengthInMinute() << ","
                  << res.exposure * 2.0 << ","
                  << res.usdTotBalanceBefore << ","
                  << res.usdTotBalanceAfter << ","
                  << res.actualPerf() << std::endl;
          if (params.sendEmail) {
            sendEmail(res, params);
            logFile << "Email sent" << std::endl;
          }
          res.reset();
          // Remove restore.txt since this trade is done.
          std::ofstream resFile("restore.txt", std::ofstream::trunc);
          resFile.close();
        }
      }
      if (params.verbose) logFile << '\n';
    }
    timeinfo.tm_sec += params.gapSec;
    currIteration++;
    if (currIteration >= params.debugMaxIteration) {
      logFile << "Max iteration reached (" << params.debugMaxIteration << ")" <<std::endl;
      stillRunning = false;
    }
    // exit if a 'stop_after_notrade' file is found
    std::ifstream infile("stop_after_notrade");
    if (infile && !inMarket)
    {
      logFile << "Exit after last trade (file stop_after_notrade found)\n";
      stillRunning = false;
    }
  }
  // analysis loop exited, do some cleanup
  curl_easy_cleanup(params.curl);

  csvFile.close();
  logFile.close();

  return 0;
}
