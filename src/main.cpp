#include "bitcoin.h"
#include "result.h"
#include "venue_spread.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "db_fun.h"
#include "parameters.h"
#include "check_entry_exit.h"
#include "check_venue_spread.h"
#include "exchanges/bitfinex.h"
#include "exchanges/okcoin.h"
#include "exchanges/bitstamp.h"
#include "exchanges/gemini.h"
#include "exchanges/kraken.h"
#include "exchanges/quadrigacx.h"
#include "exchanges/itbit.h"
#include "exchanges/btce.h"
#include "exchanges/poloniex.h"
#include "exchanges/gdax.h"
#include "exchanges/cexio.h"
#include "utils/send_email.h"
#include "getpid.h"

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>


// The 'typedef' declarations needed for the function arrays
// These functions contain everything needed to communicate with
// the exchanges, like getting the quotes or the active positions.
// Each function is implemented in the files located in the 'exchanges' folder.
typedef quote_t (*getQuoteType) (Parameters& params);
typedef double (*getAvailType) (Parameters& params, std::string currency);
typedef std::string (*sendOrderType) (Parameters& params, std::string direction, double quantity, double price);
typedef bool (*isOrderCompleteType) (Parameters& params, std::string orderId);
typedef double (*getActivePosType) (Parameters& params);
typedef double (*getLimitPriceType) (Parameters& params, double volume, bool isBid);


// This structure contains the balance of both exchanges,
// *before* and *after* an arbitrage trade.
// This is used to compute the performance of the trade,
// by comparing the balance before and after the trade.
struct Balance {
  double leg1, leg2;
  double leg1After, leg2After;
};


// 'main' function.
// Blackbird doesn't require any arguments for now.
int main(int argc, char** argv) {
  std::cout << "Blackbird Bitcoin Arbitrage" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK\n" << std::endl;
  // Replaces the C++ global locale with the user-preferred locale
  std::locale mylocale("");
  // Loads all the parameters
  Parameters params("blackbird.conf");
  // Does some verifications about the parameters
  if (!params.demoMode) {
    if (!params.useFullExposure) {
      if (params.testedExposure < 10.0 && params.leg2.compare("USD") == 0) {
        // TODO do the same check for other currencies. Is there a limi?
        std::cout << "ERROR: Minimum USD needed: $10.00" << std::endl;
        std::cout << "       Otherwise some exchanges will reject the orders\n" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (params.testedExposure > params.maxExposure) {
        std::cout << "ERROR: Test exposure (" << params.testedExposure << ") is above max exposure (" << params.maxExposure << ")\n" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

// Connects to the SQLite3 database.
// This database is used to collect bid and ask information
// from the exchanges. Not really used for the moment, but
// would be useful to collect historical bid/ask data.
  if (createDbConnection(params) != 0) {
    std::cerr << "ERROR: cannot connect to the database \'" << params.dbFile << "\'\n" << std::endl;
    exit(EXIT_FAILURE);
  }

  // We only trade BTC/USD for the moment
//  if (params.leg1.compare("BTC") != 0 || params.leg2.compare("USD") != 0) {
//    std::cout << "ERROR: Valid currency pair is only BTC/USD for now.\n" << std::endl;
//    exit(EXIT_FAILURE);
//  }

  // Function arrays containing all the exchanges functions
  // using the 'typedef' declarations from above.
  getQuoteType getQuote[10];
  getAvailType getAvail[10];
  sendOrderType sendLongOrder[10];
  sendOrderType sendShortOrder[10];
  isOrderCompleteType isOrderComplete[10];
  getActivePosType getActivePos[10];
  getLimitPriceType getLimitPrice[10];
  std::string dbTableName[10];


  // Adds the exchange functions to the arrays for all the defined exchanges
  int index = 0;
  if (params.bitfinexEnable &&
     (params.bitfinexApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Bitfinex", params.bitfinexFees, true, true);
    getQuote[index] = Bitfinex::getQuote;
    getAvail[index] = Bitfinex::getAvail;
    sendLongOrder[index] = Bitfinex::sendLongOrder;
    sendShortOrder[index] = Bitfinex::sendShortOrder;
    isOrderComplete[index] = Bitfinex::isOrderComplete;
    getActivePos[index] = Bitfinex::getActivePos;
    getLimitPrice[index] = Bitfinex::getLimitPrice;

    dbTableName[index] = "bitfinex";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.okcoinEnable &&
     (params.okcoinApi.empty() == false || params.demoMode == true)) {
    params.addExchange("OKCoin", params.okcoinFees, false, true);
    getQuote[index] = OKCoin::getQuote;
    getAvail[index] = OKCoin::getAvail;
    sendLongOrder[index] = OKCoin::sendLongOrder;
    sendShortOrder[index] = OKCoin::sendShortOrder;
    isOrderComplete[index] = OKCoin::isOrderComplete;
    getActivePos[index] = OKCoin::getActivePos;
    getLimitPrice[index] = OKCoin::getLimitPrice;

    dbTableName[index] = "okcoin";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.bitstampEnable &&
     (params.bitstampClientId.empty() == false || params.demoMode == true)) {
    params.addExchange("Bitstamp", params.bitstampFees, false, true);
    getQuote[index] = Bitstamp::getQuote;
    getAvail[index] = Bitstamp::getAvail;
    sendLongOrder[index] = Bitstamp::sendLongOrder;
    isOrderComplete[index] = Bitstamp::isOrderComplete;
    getActivePos[index] = Bitstamp::getActivePos;
    getLimitPrice[index] = Bitstamp::getLimitPrice;

    dbTableName[index] = "bitstamp";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.geminiEnable &&
     (params.geminiApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Gemini", params.geminiFees, false, true);
    getQuote[index] = Gemini::getQuote;
    getAvail[index] = Gemini::getAvail;
    sendLongOrder[index] = Gemini::sendLongOrder;
    isOrderComplete[index] = Gemini::isOrderComplete;
    getActivePos[index] = Gemini::getActivePos;
    getLimitPrice[index] = Gemini::getLimitPrice;

    dbTableName[index] = "gemini";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.krakenEnable &&
     (params.krakenApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Kraken", params.krakenFees, false, true);
    getQuote[index] = Kraken::getQuote;
    getAvail[index] = Kraken::getAvail;
    sendLongOrder[index] = Kraken::sendLongOrder;
    isOrderComplete[index] = Kraken::isOrderComplete;
    getActivePos[index] = Kraken::getActivePos;
    getLimitPrice[index] = Kraken::getLimitPrice;

    dbTableName[index] = "kraken";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.itbitEnable &&
     (params.itbitApi.empty() == false || params.demoMode == true)) {
    params.addExchange("ItBit", params.itbitFees, false, false);
    getQuote[index] = ItBit::getQuote;
    getAvail[index] = ItBit::getAvail;
    getActivePos[index] = ItBit::getActivePos;
    getLimitPrice[index] = ItBit::getLimitPrice;

    dbTableName[index] = "itbit";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.btceEnable &&
     (params.btceApi.empty() == false || params.demoMode == true)) {
    params.addExchange("BTC-e", params.btceFees, false, true);
    getQuote[index] = BTCe::getQuote;
    getAvail[index] = BTCe::getAvail;
    sendLongOrder[index] = BTCe::sendLongOrder;
    isOrderComplete[index] = BTCe::isOrderComplete;
    getActivePos[index] = BTCe::getActivePos;
    getLimitPrice[index] = BTCe::getLimitPrice;

    dbTableName[index] = "btce";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.poloniexEnable &&
     (params.poloniexApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Poloniex", params.poloniexFees, true, false);
    getQuote[index] = Poloniex::getQuote;
    getAvail[index] = Poloniex::getAvail;
    sendLongOrder[index] = Poloniex::sendLongOrder;
    sendShortOrder[index] = Poloniex::sendShortOrder;
    isOrderComplete[index] = Poloniex::isOrderComplete;
    getActivePos[index] = Poloniex::getActivePos;
    getLimitPrice[index] = Poloniex::getLimitPrice;

    dbTableName[index] = "poloniex";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.gdaxEnable &&
     (params.gdaxApi.empty() == false || params.demoMode == true)) {
    params.addExchange("GDAX", params.gdaxFees, false, false);
    getQuote[index] = GDAX::getQuote;
    getAvail[index] = GDAX::getAvail;
    getActivePos[index] = GDAX::getActivePos;
    getLimitPrice[index] = GDAX::getLimitPrice;

    dbTableName[index] = "gdax";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.quadrigaEnable &&
         (params.quadrigaApi.empty() == false || params.demoMode == true)) {
    params.addExchange("QuadrigaCX", params.quadrigaFees, false, true);
    getQuote[index] = QuadrigaCX::getQuote;
    getAvail[index] = QuadrigaCX::getAvail;
    sendLongOrder[index] = QuadrigaCX::sendLongOrder;
    isOrderComplete[index] = QuadrigaCX::isOrderComplete;
    getActivePos[index] = QuadrigaCX::getActivePos;
    getLimitPrice[index] = QuadrigaCX::getLimitPrice;

    dbTableName[index] = "quadriga";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.cexioEnable &&
      (params.cexioApi.empty() == false || params.demoMode == true)) {
    params.addExchange("CEXIO", params.cexioFees, false, true);
    getQuote[index] = CEXIO::getQuote;
    getAvail[index] = CEXIO::getAvail;
    getActivePos[index] = CEXIO::getActivePos;
    getLimitPrice[index] = CEXIO::getLimitPrice;

    dbTableName[index] = "cexio";
    createTable(dbTableName[index], params);

    index++;
  }
  // We need at least two exchanges to run Blackbird
  if (index < 2) {
    std::cout << "ERROR: Blackbird needs at least two Bitcoin exchanges. Please edit the config.json file to add new exchanges\n" << std::endl;
    exit(EXIT_FAILURE);
  }
  // Creates the CSV file that will collect the trade results
  std::string currDateTime = printDateTimeFileName();
  std::string csvFileName = "output/blackbird_result_" + currDateTime + ".csv";
  std::ofstream csvFile(csvFileName, std::ofstream::trunc);
  csvFile << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,"
          << "TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN"
          << std::endl;
  // Creates the log file where all events will be saved
  std::string logFileName = "output/blackbird_log_" + currDateTime + ".log";
  std::ofstream logFile(logFileName, std::ofstream::trunc);
  logFile.imbue(mylocale);
  logFile.precision(6);
  logFile << std::fixed;
  params.logFile = &logFile;
  // Log file header
  logFile << "--------------------------------------------" << std::endl;
  logFile << "|   Blackbird Bitcoin Arbitrage Log File   |" << std::endl;
  logFile << "--------------------------------------------\n" << std::endl;
  logFile << "Blackbird started on " << printDateTime() << "\n" << std::endl;

  logFile << "Connected to database \'" << params.dbFile << "\'\n" << std::endl;

  if (params.demoMode) {
    logFile << "Demo mode: trades won't be generated\n" << std::endl;
  }

  // Shows which pair we are trading (BTC/USD only for the moment)
  logFile << "Pair traded: " << params.leg1 << "/" << params.leg2 << "\n" << std::endl;

  std::cout << "Log file generated: " << logFileName << "\nBlackbird is running... (pid " << getpid() << ")\n" << std::endl;
  int numExch = params.nbExch();
  // The btcVec vector contains details about every exchange,
  // like fees, as specified in bitcoin.h
  std::vector<Bitcoin> btcVec;
  btcVec.reserve(numExch);
  // Creates a new Bitcoin structure within btcVec for every exchange we want to trade on
  for (int i = 0; i < numExch; ++i) {
    btcVec.push_back(Bitcoin(i, params.exchName[i], params.fees[i], params.canShort[i], params.isImplemented[i]));
  }

  // Inits cURL connections
  params.curl = curl_easy_init();
  // Shows the spreads
  logFile << "[ Targets ]\n"
          << std::setprecision(6)
          << "   Spread Entry:  " << params.spreadEntry * 100.0 << "%\n"
          << "   Spread Target: " << params.spreadTarget * 100.0  << "%\n";

  // SpreadEntry and SpreadTarget have to be positive,
  // Otherwise we will loose money on every trade.
  if (params.spreadEntry <= 0.0) {
    logFile << "   WARNING: Spread Entry should be positive" << std::endl;
  }
  if (params.spreadTarget <= 0.0) {
    logFile << "   WARNING: Spread Target should be positive" << std::endl;
  }
  logFile << std::endl;
  logFile << "[ Current balances ]" << std::endl;
  // Gets the the balances from every exchange
  // This is only done when not in Demo mode.
  std::vector<Balance> balance(numExch);
  if (!params.demoMode)
    std::transform(getAvail, getAvail + numExch,
                   begin(balance),
                   [&params]( decltype(*getAvail) apply )
                   {
                     Balance tmp {};
                     tmp.leg1 = apply(params, params.leg1);
                     tmp.leg2 = apply(params, params.leg2);
                     return tmp;
                   } );

  // Checks for a restore.txt file, to see if
  // the program exited with an open position.
  Result res;
  res.reset();
  bool inMarket = res.loadPartialResult("restore.txt");

  // Writes the current balances into the log file
  for (int i = 0; i < numExch; ++i) {
    logFile << "   " << params.exchName[i] << ":\t";
    if (params.demoMode) {
      logFile << "n/a (demo mode)" << std::endl;
    } else if (!params.isImplemented[i]) {
      logFile << "n/a (API not implemented)" << std::endl;
    } else {
      logFile << std::setprecision(6) << balance[i].leg2 << " " << params
              .leg2 << "\t"
              << std::setprecision(6) << balance[i].leg1 << " " << params.leg1 << std::endl;
    }
    if (balance[i].leg1 > 0.0050 && !inMarket) { // FIXME: hard-coded number
      logFile << "ERROR: All " << params.leg1 << " accounts must be empty before starting Blackbird" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  logFile << std::endl;
  logFile << "[ Cash exposure ]\n";
  if (params.demoMode) {
    logFile << "   No cash - Demo mode\n";
  } else {
    if (params.useFullExposure) {
      logFile << "   FULL exposure used!\n";
    } else {
      logFile << "   TEST exposure used\n   Value: "
              << std::setprecision(6) << params.testedExposure << '\n';
    }
  }
  logFile << std::endl;
  // Code implementing the loop function, that runs
  // every 'Interval' seconds.
  time_t rawtime = time(nullptr);
  tm timeinfo = *localtime(&rawtime);
  using std::this_thread::sleep_for;
  using millisecs = std::chrono::milliseconds;
  using secs      = std::chrono::seconds;
  // Waits for the next 'interval' seconds before starting the loop
  while ((int)timeinfo.tm_sec % params.interval != 0) {
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

  // Main analysis loop
  while (stillRunning) {
    currTime = mktime(&timeinfo);
    time(&rawtime);
    diffTime = difftime(rawtime, currTime);
    // Checks if we are already too late in the current iteration
    // If that's the case we wait until the next iteration
    // and we show a warning in the log file.
    if (diffTime > 0) {
      logFile << "WARNING: " << diffTime << " second(s) too late at " << printDateTime(currTime) << std::endl;
      timeinfo.tm_sec += (ceil(diffTime / params.interval) + 1) * params.interval;
      currTime = mktime(&timeinfo);
      sleep_for(secs(params.interval - (diffTime % params.interval)));
      logFile << std::endl;
    } else if (diffTime < 0) {
      sleep_for(secs(-diffTime));
    }
    // Header for every iteration of the loop
    if (params.verbose) {
      if (!inMarket) {
        logFile << "[ " << printDateTime(currTime) << " ]" << std::endl;
      } else {
        logFile << "[ " << printDateTime(currTime) << " IN MARKET: Long " << res.exchNameLong << " / Short " << res.exchNameShort << " ]" << std::endl;
      }
    }
    // Gets the bid and ask of all the exchanges
    for (int i = 0; i < numExch; ++i) {
      auto quote = getQuote[i](params);
      double bid = quote.bid();
      double ask = quote.ask();

      // Saves the bid/ask into the SQLite database
      addBidAskToDb(dbTableName[i], printDateTimeDb(currTime), bid, ask, params);

      // If there is an error with the bid or ask (i.e. value is null),
      // we show a warning but we don't stop the loop.
      if (bid == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " bid is null" << std::endl;
      }
      if (ask == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " ask is null" << std::endl;
      }
      // Shows the bid/ask information in the log file
      if (params.verbose) {
        logFile << "   " << params.exchName[i] << ": \t"
                << std::setprecision(6)
                << bid << " / " << ask << std::endl;
      }
      // Updates the Bitcoin vector with the latest bid/ask data
      btcVec[i].updateData(quote);
      curl_easy_reset(params.curl);
    }
    if (params.verbose) {
      logFile << "   ----------------------------" << std::endl;
    }
    // Stores all the spreads in arrays to
    // compute the volatility. The volatility
    // is not used for the moment.
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

    //#################################TODO###############################
    // Looks for opportunities for venue spread
    std::vector<VenueSpread> vsList;
    for (int i = 0; i < numExch; ++i) {
      for (int j = 0; j < numExch; ++j) {
        if (i != j) {
          VenueSpread vs;
          checkVenueSpread(&btcVec[i], &btcVec[j], vs, params);
          if (vs.spreadIn > params.venueSpreadTh) { // add VenueSpreadTh=0.08 to blackbird.conf
            vsList.push_back(vs);
          }
          
        }
      }
    }

    // vsList will contains all the spread between venue larger than 0.5
    // TODO send via email

    for (unsigned i = 0; i < vsList.size(); i++) {
      VenueSpread vs = vsList[i];
      std::cout << "Buy   at: " << vs.priceLongIn  << " on " << vs.exchNameLong << ";" << std::endl;
      std::cout << "Sell  at: " << vs.priceShortIn << " on " << vs.exchNameShort << ";" << std::endl;
      std::cout << "Spread  : " << vs.spreadIn << ";" << std::endl;
    }



    // Looks for arbitrage opportunities on all the exchange combinations
    if (!inMarket) {
      for (int i = 0; i < numExch; ++i) {
        for (int j = 0; j < numExch; ++j) {
          if (i != j) {
            if (checkEntry(&btcVec[i], &btcVec[j], res, params)) {
              // An entry opportunity has been found!
              res.exposure = std::min(balance[res.idExchLong].leg2, balance[res.idExchShort].leg2);
              if (params.demoMode) {
                logFile << "INFO: Opportunity found but no trade will be generated (Demo mode)" << std::endl;
                break;
              }
              if (res.exposure == 0.0) {
                logFile << "WARNING: Opportunity found but no cash available. Trade canceled" << std::endl;
                break;
              }
              if (params.useFullExposure == false && res.exposure <= params.testedExposure) {
                logFile << "WARNING: Opportunity found but no enough cash. Need more than TEST cash (min. $"
                        << std::setprecision(6) << params.testedExposure <<
                                                                         "). Trade canceled" << std::endl;
                break;
              }
              if (params.useFullExposure) {
                // Removes 1% of the exposure to have
                // a little bit of margin.
                res.exposure -= 0.01 * res.exposure;
                if (res.exposure > params.maxExposure) {
                  logFile << "WARNING: Opportunity found but exposure ("
                          << std::setprecision(6)
                          << res.exposure << ") above the limit\n"
                          << "         Max exposure will be used instead (" << params.maxExposure << ")" << std::endl;
                  res.exposure = params.maxExposure;
                }
              } else {
                res.exposure = params.testedExposure;
              }
              // Checks the volumes and, based on that, computes the limit prices
              // that will be sent to the exchanges
              double volumeLong = res.exposure / btcVec[res.idExchLong].getAsk();
              double volumeShort = res.exposure / btcVec[res.idExchShort].getBid();
              double limPriceLong = getLimitPrice[res.idExchLong](params, volumeLong, false);
              double limPriceShort = getLimitPrice[res.idExchShort](params, volumeShort, true);
              if (limPriceLong == 0.0 || limPriceShort == 0.0) {
                logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
                logFile.precision(6);
                logFile << "         Long limit price:  " << limPriceLong << std::endl;
                logFile << "         Short limit price: " << limPriceShort << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              if (limPriceLong - res.priceLongIn > params.priceDeltaLim || res.priceShortIn - limPriceShort > params.priceDeltaLim) {
                logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
                logFile.precision(6);
                logFile << "         Target long price:  " << res.priceLongIn << ", Real long price:  " << limPriceLong << std::endl;
                logFile << "         Target short price: " << res.priceShortIn << ", Real short price: " << limPriceShort << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              // We are in market now, meaning we have positions on leg1 (the hedged on)
              // We store the details of that first trade into the Result structure.
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

              // Send the orders to the two exchanges
              auto longOrderId = sendLongOrder[res.idExchLong](params, "buy", volumeLong, limPriceLong);
              auto shortOrderId = sendShortOrder[res.idExchShort](params, "sell", volumeShort, limPriceShort);
              logFile << "Waiting for the two orders to be filled..." << std::endl;
              sleep_for(millisecs(5000));
              bool isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
              bool isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
              // Loops until both orders are completed
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
              // Both orders are now fully executed
              logFile << "Done" << std::endl;

              // Stores the partial result to file in case
              // the program exits before closing the position.
              res.savePartialResult("restore.txt");

              // Resets the order ids
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
      // We are in market and looking for an exit opportunity
      if (checkExit(&btcVec[res.idExchLong], &btcVec[res.idExchShort], res, params, currTime)) {
        // An exit opportunity has been found!
        // We check the current leg1 exposure
        std::vector<double> btcUsed(numExch);
        for (int i = 0; i < numExch; ++i) {
          btcUsed[i] = getActivePos[i](params);
        }
        // Checks the volumes and computes the limit prices that will be sent to the exchanges
        double volumeLong = btcUsed[res.idExchLong];
        double volumeShort = btcUsed[res.idExchShort];
        double limPriceLong = getLimitPrice[res.idExchLong](params, volumeLong, true);
        double limPriceShort = getLimitPrice[res.idExchShort](params, volumeShort, false);
        if (limPriceLong == 0.0 || limPriceShort == 0.0) {
          logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
          logFile.precision(6);
          logFile << "         Long limit price:  " << limPriceLong << std::endl;
          logFile << "         Short limit price: " << limPriceShort << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else if (res.priceLongOut - limPriceLong > params.priceDeltaLim || limPriceShort - res.priceShortOut > params.priceDeltaLim) {
          logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
          logFile.precision(6);
          logFile << "         Target long price:  " << res.priceLongOut << ", Real long price:  " << limPriceLong << std::endl;
          logFile << "         Target short price: " << res.priceShortOut << ", Real short price: " << limPriceShort << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else {
          res.exitTime = currTime;
          res.priceLongOut = limPriceLong;
          res.priceShortOut = limPriceShort;
          res.printExitInfo(*params.logFile);

          logFile.precision(6);
          logFile << params.leg1 << " exposure on " << params.exchName[res.idExchLong] << ": " << volumeLong << '\n'
                  << params.leg1 << " exposure on " << params.exchName[res.idExchShort] << ": " << volumeShort << '\n'
                  << std::endl;
          auto longOrderId = sendLongOrder[res.idExchLong](params, "sell", fabs(btcUsed[res.idExchLong]), limPriceLong);
          auto shortOrderId = sendShortOrder[res.idExchShort](params, "buy", fabs(btcUsed[res.idExchShort]), limPriceShort);
          logFile << "Waiting for the two orders to be filled..." << std::endl;
          sleep_for(millisecs(5000));
          bool isLongOrderComplete = isOrderComplete[res.idExchLong](params, longOrderId);
          bool isShortOrderComplete = isOrderComplete[res.idExchShort](params, shortOrderId);
          // Loops until both orders are completed
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
            balance[i].leg2After = getAvail[i](params, "usd"); // FIXME: currency hard-coded
            balance[i].leg1After = getAvail[i](params, "btc"); // FIXME: currency hard-coded
          }
          for (int i = 0; i < numExch; ++i) {
            logFile << "New balance on " << params.exchName[i] << ":  \t";
            logFile.precision(2);
            logFile << balance[i].leg2After << " " << params.leg2 << " (perf " << balance[i].leg2After - balance[i].leg2 << "), ";
            logFile << std::setprecision(6) << balance[i].leg1After << " " << params.leg1 << "\n";
          }
          logFile << std::endl;
          // Update total leg2 balance
          for (int i = 0; i < numExch; ++i) {
            res.leg2TotBalanceBefore += balance[i].leg2;
            res.leg2TotBalanceAfter  += balance[i].leg2After;
          }
          // Update current balances
          for (int i = 0; i < numExch; ++i) {
            balance[i].leg2 = balance[i].leg2After;
            balance[i].leg1 = balance[i].leg1After;
          }
          // Prints the result in the result CSV file
          logFile.precision(2);
          logFile << "ACTUAL PERFORMANCE: " << "$" << res.leg2TotBalanceAfter - res.leg2TotBalanceBefore << " (" << res.actualPerf() * 100.0 << "%)\n" << std::endl;
          csvFile << res.id << ","
                  << res.exchNameLong << ","
                  << res.exchNameShort << ","
                  << printDateTimeCsv(res.entryTime) << ","
                  << printDateTimeCsv(res.exitTime) << ","
                  << res.getTradeLengthInMinute() << ","
                  << res.exposure * 2.0 << ","
                  << res.leg2TotBalanceBefore << ","
                  << res.leg2TotBalanceAfter << ","
                  << res.actualPerf() << std::endl;
          // Sends an email with the result of the trade
          if (params.sendEmail) {
            sendEmail(res, params);
            logFile << "Email sent" << std::endl;
          }
          res.reset();
          // Removes restore.txt since this trade is done.
          std::ofstream resFile("restore.txt", std::ofstream::trunc);
          resFile.close();
        }
      }
      if (params.verbose) logFile << '\n';
    }
    // Moves to the next iteration, unless
    // the maxmum is reached.
    timeinfo.tm_sec += params.interval;
    currIteration++;
    if (currIteration >= params.debugMaxIteration) {
      logFile << "Max iteration reached (" << params.debugMaxIteration << ")" <<std::endl;
      stillRunning = false;
    }
    // Exits if a 'stop_after_notrade' file is found
    // Warning: by default on GitHub the file has a underscore
    // at the end, so Blackbird is not stopped by default.
    std::ifstream infile("stop_after_notrade");
    if (infile && !inMarket) {
      logFile << "Exit after last trade (file stop_after_notrade found)\n";
      stillRunning = false;
    }
  }
  // Analysis loop exited, does some cleanup
  curl_easy_cleanup(params.curl);
  csvFile.close();
  logFile.close();

  return 0;
}
