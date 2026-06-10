#include "VWAPExecution.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Config.h"
#include "Types.h"
#include "ExecutionStrategy.h"

/*
 * Convert MyVWAP vs MarketVWAP into a readable result string.
 *
 * For a buy order:
 * - lower execution VWAP is better
 * - higher execution VWAP is worse
 */
static std::string getBuyOrderResult(double myVWAP, double marketVWAP) {
    if (myVWAP <= 0.0 || marketVWAP <= 0.0) {
        return "invalid_vwap";
    }

    if (myVWAP < marketVWAP) {
        return "better_than_market_vwap_for_buy_order";
    }

    if (myVWAP > marketVWAP) {
        return "worse_than_market_vwap_for_buy_order";
    }

    return "equal_to_market_vwap_for_buy_order";
}

/*
 * Build a strategy-specific execution schedule.
 *
 * Each bucket uses:
 * - historical relative volume for order size
 * - strategy-selected test-date trade price for execution price
 */
std::vector<ExecutionScheduleRow> buildExecutionSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile,
    const ReducedResult& reduced,
    ExecutionStrategyType strategy,
    const StrategyConfig& strategyConfig
) {
    std::vector<ExecutionScheduleRow> schedule;

    if (volumeProfile.empty()) {
        std::cerr << "Warning: volume profile is empty." << std::endl;
        return schedule;
    }

    std::string name = strategyName(strategy);

    double totalOrderSize = 0.0;
    double totalExecutedValue = 0.0;

    for (size_t i = 0; i < volumeProfile.size(); i++) {
        const VolumeProfileRow& profileRow = volumeProfile[i];

        ExecutionScheduleRow scheduleRow;
        scheduleRow.strategy = name;
        scheduleRow.bucket = profileRow.bucket;
        scheduleRow.avgQty = profileRow.avgQty;
        scheduleRow.relativeVolume = profileRow.relativeVolume;
        scheduleRow.orderSize =
            static_cast<double>(config.targetShares) * profileRow.relativeVolume;

        std::map<std::string, BucketAggregate>::const_iterator bucketIt =
            reduced.bucketMap.find(profileRow.bucket);

        if (bucketIt == reduced.bucketMap.end()) {
            scheduleRow.executionPrice = 0.0;
            scheduleRow.executedValue = 0.0;
            scheduleRow.decisionReason = "bucket_not_found_in_reduced_result";

            std::cerr << "Warning: bucket not found in reduced result: "
                      << profileRow.bucket << std::endl;

            schedule.push_back(scheduleRow);
            continue;
        }

        const BucketAggregate& bucket = bucketIt->second;

        ExecutionPriceDecision decision = selectExecutionPrice(
            bucket.testTrades,
            strategy,
            strategyConfig
        );

        if (!decision.hasPrice) {
            scheduleRow.executionPrice = 0.0;
            scheduleRow.executedValue = 0.0;
            scheduleRow.decisionReason = decision.reason;

            std::cerr << "Warning: no execution price for bucket "
                      << profileRow.bucket
                      << ", reason: "
                      << decision.reason
                      << std::endl;

            schedule.push_back(scheduleRow);
            continue;
        }

        scheduleRow.executionGmtTime = decision.gmtTime;
        scheduleRow.executionPrice = decision.price;
        scheduleRow.executedValue =
            scheduleRow.orderSize * scheduleRow.executionPrice;
        scheduleRow.decisionReason = decision.reason;

        totalOrderSize += scheduleRow.orderSize;
        totalExecutedValue += scheduleRow.executedValue;

        schedule.push_back(scheduleRow);
    }

    std::cout << "Execution schedule built for strategy: " << name << std::endl;
    std::cout << "Target shares: " << config.targetShares << std::endl;
    std::cout << "Total scheduled order size: " << totalOrderSize << std::endl;
    std::cout << "Total scheduled executed value: " << totalExecutedValue << std::endl;

    return schedule;
}

/*
 * Phase 1 compatibility wrapper.
 *
 * This lets main.cpp still say "build buy-first schedule", but internally
 * the price selection now goes through ExecutionStrategy.
 */
std::vector<ExecutionScheduleRow> buildBuyFirstSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile,
    const ReducedResult& reduced
) {
    StrategyConfig strategyConfig;

    return buildExecutionSchedule(
        config,
        volumeProfile,
        reduced,
        ExecutionStrategyType::BUY_FIRST,
        strategyConfig
    );
}

/*
 * Compute the performance summary.
 *
 * MyVWAP:
 *   This is the VWAP of our simulated execution schedule.
 *
 *   MyVWAP = sum(OrderSize_i * ExecutionPrice_i) / sum(OrderSize_i)
 *
 * MarketVWAP:
 *   This is the actual market VWAP on the test date using all trade messages
 *   inside the same market window.
 *
 *   MarketVWAP = sum(MarketTradePrice_j * MarketTradeQty_j)
 *              / sum(MarketTradeQty_j)
 */
PerformanceSummary computePerformanceSummary(
    const Config& config,
    const std::vector<ExecutionScheduleRow>& schedule,
    const ReducedResult& reduced
) {
    PerformanceSummary summary;
    summary.targetShares = config.targetShares;

    if (schedule.empty()) {
        std::cerr << "Warning: execution schedule is empty." << std::endl;
        return summary;
    }

    /*
     * Store strategy name if the schedule rows contain it.
     */
    if (!schedule[0].strategy.empty()) {
        summary.strategy = schedule[0].strategy;
    }

    double totalOrderSize = 0.0;
    double totalExecutedValue = 0.0;

    for (size_t i = 0; i < schedule.size(); i++) {
        const ExecutionScheduleRow& row = schedule[i];

        /*
         * Skip rows with invalid order size or price.
         *
         * In a clean SPY run, all buckets should have positive order size
         * and valid execution price.
         */
        if (row.orderSize <= 0.0) {
            continue;
        }

        if (row.executionPrice <= 0.0) {
            std::cerr << "Warning: missing execution price for bucket "
                      << row.bucket << std::endl;
            continue;
        }

        totalOrderSize += row.orderSize;
        totalExecutedValue += row.executedValue;
    }

    if (totalOrderSize > 0.0) {
        summary.myVWAP = totalExecutedValue / totalOrderSize;
    } else {
        std::cerr << "Warning: total order size is zero." << std::endl;
        summary.myVWAP = 0.0;
    }

    if (reduced.marketQtyTotal > 0) {
        summary.marketVWAP =
            reduced.marketNotionalTotal / static_cast<double>(reduced.marketQtyTotal);
    } else {
        std::cerr << "Warning: market quantity total is zero." << std::endl;
        summary.marketVWAP = 0.0;
    }

    if (summary.marketVWAP > 0.0) {
        summary.difference = summary.myVWAP - summary.marketVWAP;
        summary.differenceBps =
            (summary.myVWAP / summary.marketVWAP - 1.0) * 10000.0;
    } else {
        summary.difference = 0.0;
        summary.differenceBps = 0.0;
    }

    summary.result = getBuyOrderResult(summary.myVWAP, summary.marketVWAP);

    std::cout << "Performance summary computed." << std::endl;
    std::cout << "Strategy: " << summary.strategy << std::endl;
    std::cout << "My VWAP: " << summary.myVWAP << std::endl;
    std::cout << "Market VWAP: " << summary.marketVWAP << std::endl;
    std::cout << "Difference: " << summary.difference << std::endl;
    std::cout << "Difference bps: " << summary.differenceBps << std::endl;
    std::cout << "Buy execution result: " << summary.result << std::endl;

    return summary;
}