#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

/*
 * One parsed trade record from the raw CSV file.
 */
struct TradeRecord {
    std::string symbol;
    std::string date;
    std::string gmtTime;
    std::string nyBucket;

    double price = 0.0;
    long quantity = 0;
};

/*
 * One trade inside a test-date bucket.
 *
 * Phase 2 strategies need access to the ordered trades inside each bucket.
 * For example:
 * - buy-first uses the first trade in the bucket
 * - buy-tick compares the first several trades
 * - sell-tick scans for a price drop and buys the next trade
 */
struct BucketTrade {
    std::string gmtTime;
    double price = 0.0;
    long quantity = 0;
};

/*
 * Aggregated information for one 15-minute bucket.
 *
 * historicalQty:
 *   Total trade quantity in this bucket during the training period.
 *
 * executionPrice / executionGmtTime:
 *   Phase 1 buy-first fields.
 *   Keep them for now so the current baseline still compiles and runs.
 *   Later, Phase 2 will move execution-price selection into ExecutionStrategy.
 *
 * marketQty / marketNotional:
 *   Test-date market VWAP components.
 *
 * testTrades:
 *   All test-date trades in this bucket.
 *   This is the main Phase 2 addition.
 */
struct BucketAggregate {
    long historicalQty = 0;

    double executionPrice = 0.0;
    std::string executionGmtTime = "";

    long marketQty = 0;
    double marketNotional = 0.0;

    std::vector<BucketTrade> testTrades;
};

/*
 * Output from one mapper.
 */
struct MapResult {
    std::map<std::string, BucketAggregate> bucketMap;
    std::set<std::string> trainingDates;

    long marketQtyTotal = 0;
    double marketNotionalTotal = 0.0;

    int rc = 0;
    std::string errorMessage = "";
};

/*
 * Final reduced result after merging all mapper outputs.
 */
struct ReducedResult {
    std::map<std::string, BucketAggregate> bucketMap;
    std::set<std::string> trainingDates;

    long marketQtyTotal = 0;
    double marketNotionalTotal = 0.0;
};

/*
 * Historical volume profile row.
 * Later can stop relying on this field once buy-first becomes
 * a strategy function.
 */
struct VolumeProfileRow {
    std::string bucket;

    long avgQty = 0;
    double relativeVolume = 0.0;

    double executionPrice = 0.0;
};

/*
 * One row in a strategy-specific execution schedule.
 *
 * Phase 1 output only needs:
 * bucket, avg_qty, relative_volume, order_size, execution_price, executed_value
 *
 * Phase 2 adds:
 * strategy, executionGmtTime, decisionReason
 *
 * These extra fields are useful for debugging different strategies.
 */
struct ExecutionScheduleRow {
    std::string strategy;
    std::string bucket;

    long avgQty = 0;
    double relativeVolume = 0.0;
    double orderSize = 0.0;

    std::string executionGmtTime;
    double executionPrice = 0.0;
    double executedValue = 0.0;

    std::string decisionReason;
};

/*
 * Performance summary for one strategy.
 *
 * Phase 1 only used one summary.
 * Phase 2 will create one summary per strategy and write them into
 * strategy_comparison.csv.
 */
struct PerformanceSummary {
    std::string strategy;

    double myVWAP = 0.0;
    double marketVWAP = 0.0;
    double difference = 0.0;
    double differenceBps = 0.0;

    long targetShares = 0;

    std::string result;
};