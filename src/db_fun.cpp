#include <iostream>
#include <sstream>
#include <mysql.h>
#include "db_fun.h"

int createDbConnection(Parameters &params) {
    params.dbConn = mysql_init(NULL);
    std::stringstream query;
    if (params.dbConn == NULL) {
        std::cout << mysql_error(params.dbConn) << std::endl;
        return -1;
    }
    if (mysql_real_connect(params.dbConn, params.dbHost.c_str(), params.dbUser.c_str(), params.dbPassword.c_str(), NULL,
                           0, NULL, 0) == NULL) {
        std::cout << mysql_error(params.dbConn) << std::endl;
        mysql_close(params.dbConn);
        return -1;
    }
    query << "USE " << params.dbName;
    return mysql_query(params.dbConn, query.str().c_str());
}

int createTable(std::string exchangeName, Parameters &params) {
    std::stringstream query;
    query << "CREATE TABLE IF NOT EXISTS `" << exchangeName
          << "` (Datetime DATETIME NOT NULL, bid DECIMAL(8, 2), ask DECIMAL(8, 2));";
    return mysql_query(params.dbConn, query.str().c_str());
}

int addBidAskToDb(std::string exchangeName, std::string datetime, double bid, double ask, Parameters &params) {
    std::stringstream query;
    query << "INSERT INTO `" << exchangeName << "` VALUES ('" << datetime << "'," << bid << "," << ask << ");";
    return mysql_query(params.dbConn, query.str().c_str());
}

