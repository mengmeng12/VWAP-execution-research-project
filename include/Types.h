#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>

struct TradeRecord {
    std::string symbol;
    std::string date;
    std::string gmtTime;
    std::string nyBucket;
    double price = 0.0;
    long quantity = 0;
};

struct BucketAggregate {
    long historicalQty = 0;
    double executionPrice = 0.0;
    std::string executionGmtTime = "";

    long marketQty = 0;
    double marketNotional = 0.0;
};

struct MapResult {
    std::map<std::string, BucketAggregate> bucketMap;
    std::set<std::string> trainingDates;

    long marketQtyTotal = 0;
    double marketNotionalTotal = 0.0;

    int rc = 0;
    std::string errorMessage = "";
};

struct ReducedResult {
    std::map<std::string, BucketAggregate> bucketMap;
    std::set<std::string> trainingDates;

    long marketQtyTotal = 0;
    double marketNotionalTotal = 0.0;
};

struct VolumeProfileRow {
    std::string bucket;
    long avgQty = 0;
    double relativeVolume = 0.0;
    double executionPrice = 0.0;
};

struct ExecutionScheduleRow {
    std::string bucket;
    long avgQty = 0;
    double relativeVolume = 0.0;
    double orderSize = 0.0;
    double executionPrice = 0.0;
    double executedValue = 0.0;
};

struct PerformanceSummary {
    double myVWAP = 0.0;
    double marketVWAP = 0.0;
    double difference = 0.0;
    double differenceBps = 0.0;
    long targetShares = 0;
};