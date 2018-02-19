#include "parameters.h"
#include "nulllongexch.h"
#include <iomanip>
#include <vector>
#include <array>
#include <ctime>
#include <cmath>

namespace NullLongExch
{

static long double timeStart = time(0);

quote_t getQuote(Parameters &params){
 
    // steadily increase bid so exits will execute
    const long double sysTime = time(0);
    double timeElapsed = sysTime - timeStart;
    // start at 9000 (buy at 9000, sell at 10000 in NullShortExch)
    double bidValue = 9000;
    // climb to 11,000 with a little randomness
    if (0 < timeElapsed){
        bidValue = bidValue + ( 200-100*((double)rand()/(double)RAND_MAX)+100); 
    }
    if (60 <= timeElapsed){
        // a minute has gone by, reset timeValue
        bidValue = 11000 - (200-100*((double)rand()/(double)RAND_MAX)+100);
    }
    // pingpong between 9000 and 11000 with a bit of randomness
    double askValue = bidValue+100;
    return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency) {
    double availBal = 0;
    symbolTransform(params, currency);
    // do a real avail bal curl

    if (currency.compare("XXBT")==0){
        availBal = 1000;
        return availBal;
    } else if (currency.compare("ZUSD")==0){
        availBal = 0.0;
    }
    return availBal;
}

double getActivePos(Parameters &params, std::string orderId){
    double activeSize = std::stod(orderId);
    return activeSize;
}

double getLimitPrice(Parameters &params, double volume, bool isBid){
    double atBid = 10000;
    // pingpong
    const long double sysTime = time(0);
    //double atAsk = 9000 + (sysTime - timeStart)*35 + ( 10-1*((double)rand()/(double)RAND_MAX)+1);
    double atAsk = 11000;
    return atAsk;
}

std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price){
    double executedAt = price*quantity;
    std::string execute = std::to_string(executedAt);
    return execute;
}

bool isOrderComplete(Parameters &params, std::string orderId){
    return true;
}
void reset(){
    timeStart = time(0);
}
std::string symbolTransform(Parameters& params, std::string leg){
    std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
    if (leg.compare("BTC")==0){
        return "XXBT";
    }
    else if (leg.compare("USD")==0){
        return "ZUSD";
    }
    else { 
    *params.logFile << "<NullLongExch> WARNING: Currency Not Supported" << std::endl;
    return "";
    }
}
}