#include "bitcoin.h"
#include <math.h>

Bitcoin::Bitcoin(unsigned i, std::string n, double f, bool h, bool m) {
    id = i;
    exchName = n;
    fees = f;
    hasShort = h;
    isImplemented = m;
    bid = 0.0;
    ask = 0.0;
}

void Bitcoin::updateData(double b, double a) {
    bid = b;
    ask = a;
}

unsigned Bitcoin::getId() const {
    return id;
}

double Bitcoin::getBid() const {
    return bid;
}

double Bitcoin::getAsk() const {
    return ask;
}

double Bitcoin::getMidPrice() const {
    if (bid > 0.0 && ask > 0.0) {
        return (bid + ask) / 2.0;
    } else {
        return 0.0;
    }
}

std::string Bitcoin::getExchName() const {
    return exchName;
}

double Bitcoin::getFees() const {
    return fees;
}

bool Bitcoin::getHasShort() const {
    return hasShort;
}

bool Bitcoin::getIsImplemented() const {
    return isImplemented;
}

