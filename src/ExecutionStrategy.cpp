#include "ExecutionStrategy.h"

#include <iostream>
#include <algorithm>

/*
 * BUY_FIRST strategy:
 *
 * Use the first trade price inside the bucket.
 *
 * This reproduces the Phase 1 baseline logic, but moves the price-selection
 * decision into the strategy framework.
 */
static ExecutionPriceDecision selectBuyFirst(
    const std::vector<BucketTrade>& trades
) {
    ExecutionPriceDecision decision;

    if (trades.empty()) {
        decision.hasPrice = false;
        decision.reason = "no_trades_in_bucket";
        return decision;
    }

    const BucketTrade& firstTrade = trades.front();

    decision.hasPrice = true;
    decision.gmtTime = firstTrade.gmtTime;
    decision.price = firstTrade.price;
    decision.reason = "first_trade_in_bucket";

    return decision;
}

/*
 * BUY_TICK strategy:
 *
 * This is a simple trade-only price-movement heuristic.
 *
 * For each bucket:
 * - Look at the first N trades.
 * - If the N-th trade price is higher than the first trade price,
 *   we treat it as early upward pressure and buy immediately at the first trade.
 * - Otherwise, we wait until the N-th trade.
 *
 * Formula:
 *   if price_N > price_1:
 *       execution_price = price_1
 *   else:
 *       execution_price = price_N
 *
 * This is not a full order-book or aggressor-side model.
 * It only uses trade prices.
 */
static ExecutionPriceDecision selectBuyTick(
    const std::vector<BucketTrade>& trades,
    int lookahead
) {
    ExecutionPriceDecision decision;

    if (trades.empty()) {
        decision.hasPrice = false;
        decision.reason = "no_trades_in_bucket";
        return decision;
    }

    /*
     * Defensive fallback:
     * If lookahead is invalid, use the first trade.
     */
    if (lookahead <= 1) {
        const BucketTrade& firstTrade = trades.front();

        decision.hasPrice = true;
        decision.gmtTime = firstTrade.gmtTime;
        decision.price = firstTrade.price;
        decision.reason = "invalid_or_small_lookahead_fallback_to_first";

        return decision;
    }

    /*
     * If the bucket has fewer than N trades, use all available trades.
     */
    int n = std::min(lookahead, static_cast<int>(trades.size()));

    if (n <= 1) {
        const BucketTrade& firstTrade = trades.front();

        decision.hasPrice = true;
        decision.gmtTime = firstTrade.gmtTime;
        decision.price = firstTrade.price;
        decision.reason = "only_one_trade_fallback_to_first";

        return decision;
    }

    const BucketTrade& firstTrade = trades.front();
    const BucketTrade& nthTrade = trades[static_cast<size_t>(n - 1)];

    decision.hasPrice = true;

    if (nthTrade.price > firstTrade.price) {
        /*
         * Early upward pressure:
         * For a buy order, buy earlier before price moves higher.
         */
        decision.gmtTime = firstTrade.gmtTime;
        decision.price = firstTrade.price;
        decision.reason = "upward_pressure_buy_first_trade";
    } else {
        /*
         * No early upward pressure:
         * Wait until the N-th trade.
         */
        decision.gmtTime = nthTrade.gmtTime;
        decision.price = nthTrade.price;
        decision.reason = "no_upward_pressure_buy_nth_trade";
    }

    return decision;
}
/*
 * SELL_TICK strategy:
 *
 * This is a trade-only approximation.
 *
 * Since Phase 2 does not use quotes, order book, or aggressor-side labels,
 * we approximate a sell tick as:
 *
 *   current_trade_price < previous_trade_price
 *
 * For a buy order:
 * - A sell tick may indicate short-term selling pressure.
 * - We wait until a sell tick appears.
 * - Then we buy at the next trade price.
 *
 * If no sell tick is found, we fall back to the first trade price.
 *
 * This is not true trade direction classification.
 */
static ExecutionPriceDecision selectSellTick(
    const std::vector<BucketTrade>& trades
) {
    ExecutionPriceDecision decision;

    if (trades.empty()) {
        decision.hasPrice = false;
        decision.reason = "no_trades_in_bucket";
        return decision;
    }

    /*
     * Need at least three trades to:
     * - compare trade i with trade i - 1
     * - then buy at trade i + 1
     *
     * If there are fewer trades, fall back to buy-first.
     */
    if (trades.size() < 3) {
        const BucketTrade& firstTrade = trades.front();

        decision.hasPrice = true;
        decision.gmtTime = firstTrade.gmtTime;
        decision.price = firstTrade.price;
        decision.reason = "not_enough_trades_fallback_to_first";

        return decision;
    }

    /*
     * Scan for the first price decline.
     *
     * We stop at i + 1 < trades.size() because the strategy buys at
     * the next trade after the sell tick.
     */
    for (size_t i = 1; i + 1 < trades.size(); i++) {
        double previousPrice = trades[i - 1].price;
        double currentPrice = trades[i].price;

        if (currentPrice < previousPrice) {
            const BucketTrade& nextTrade = trades[i + 1];

            decision.hasPrice = true;
            decision.gmtTime = nextTrade.gmtTime;
            decision.price = nextTrade.price;
            decision.reason = "sell_tick_detected_buy_next_trade";

            return decision;
        }
    }

    /*
     * No sell tick found in this bucket.
     * Fall back to buy-first.
     */
    const BucketTrade& firstTrade = trades.front();

    decision.hasPrice = true;
    decision.gmtTime = firstTrade.gmtTime;
    decision.price = firstTrade.price;
    decision.reason = "no_sell_tick_fallback_to_first";

    return decision;
}
/*
 * Convert strategy enum to file-friendly string.
 */
std::string strategyName(ExecutionStrategyType strategy) {
    switch (strategy) {
        case ExecutionStrategyType::BUY_FIRST:
            return "buy_first";

        case ExecutionStrategyType::BUY_TICK:
            return "buy_tick";

        case ExecutionStrategyType::SELL_TICK:
            return "sell_tick";

        default:
            return "unknown";
    }
}

/*
 * General strategy selector.
 *
 * Step 3 only supports BUY_FIRST.
 * BUY_TICK and SELL_TICK are placeholders for later steps.
 */
ExecutionPriceDecision selectExecutionPrice(
    const std::vector<BucketTrade>& trades,
    ExecutionStrategyType strategy,
    const StrategyConfig& config
) {
    /*
     * config is not used by BUY_FIRST yet.
     * This line avoids an unused-parameter warning.
     */

    switch (strategy) {
        case ExecutionStrategyType::BUY_FIRST:
            return selectBuyFirst(trades);

        case ExecutionStrategyType::BUY_TICK: {
            return selectBuyTick(trades, config.buyTickLookahead);
        }

        case ExecutionStrategyType::SELL_TICK: {
            return selectSellTick(trades);
        }

        default: {
            ExecutionPriceDecision decision;
            decision.hasPrice = false;
            decision.reason = "unknown_strategy";
            return decision;
        }
    }
}