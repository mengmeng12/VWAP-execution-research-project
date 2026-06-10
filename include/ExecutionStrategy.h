#pragma once

#include <string>
#include <vector>

#include "Types.h"

/*
 * Phase 2 trade-only execution strategies.
 *
 * For now, we only implement BUY_FIRST.
 * BUY_TICK and SELL_TICK will be added in later steps.
 */
enum class ExecutionStrategyType {
    BUY_FIRST,
    BUY_TICK,
    SELL_TICK
};

/*
 * Strategy-level configuration.
 *
 * buyTickLookahead:
 *   Number of early trades to inspect for buy-tick logic.
 *   This will be used in Step 5.
 */
struct StrategyConfig {
    int buyTickLookahead = 5;
};

/*
 * The result of selecting one execution price from a bucket.
 *
 * hasPrice:
 *   False if the bucket has no test-date trades.
 *
 * gmtTime:
 *   The timestamp of the selected trade.
 *
 * price:
 *   The selected execution price.
 *
 * reason:
 *   Human-readable explanation, useful for debugging CSV output later.
 */
struct ExecutionPriceDecision {
    bool hasPrice = false;

    std::string gmtTime = "";
    double price = 0.0;

    std::string reason = "";
};

/*
 * Convert strategy enum to a file-friendly name.
 *
 * Example:
 *   BUY_FIRST -> "buy_first"
 */
std::string strategyName(ExecutionStrategyType strategy);

/*
 * Main strategy selector.
 *
 * Input:
 *   trades:
 *     Ordered test-date trades inside one bucket.
 *
 *   strategy:
 *     Which execution strategy to apply.
 *
 *   config:
 *     Strategy parameters.
 *
 * Output:
 *   ExecutionPriceDecision containing selected price, time, and reason.
 */
ExecutionPriceDecision selectExecutionPrice(
    const std::vector<BucketTrade>& trades,
    ExecutionStrategyType strategy,
    const StrategyConfig& config
);