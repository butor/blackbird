#include "parameters.h"
#include "nullshortexch.h"
#include <iomanip>
#include <vector>
#include <array>
#include <ctime>
#include <cmath>

namespace NullShortExch
{
static long double timeStart = time(0);
quote_t getQuote(Parameters &params){
    //double bidValue = 0.0;
    // steadily decrease but with a bit of randomness for trailing
    double bidValue = 10000;
    double askValue = bidValue + 100;
    return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency) {
    double availBal = 0;
    std::transform(currency.begin(),currency.end(), currency.begin(), ::toupper);
    if (currency.compare("USD")==0){
        availBal = 1000;
        return availBal;
    } else if (currency.compare("BTC")==0){
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
    double atAsk = atBid +100;
    return atBid;
}

std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price){
    double executedAt = price*quantity;
    std::string execute = std::to_string(executedAt);
    return execute;
}
std::string sendShortOrder(Parameters &params, std::string direction, double quantity, double price){
    double executedAt = price*quantity;
    std::string execute = std::to_string(executedAt);
    return execute;
}

bool isOrderComplete(Parameters &params, std::string orderId){
    return true;
}

}