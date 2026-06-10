# Phase 2 Strategy Expansion Notes

## Objective

Phase 2 extends the Phase 1 VWAP baseline engine into a multi-strategy trade-only execution framework.

Phase 1 implemented one hard-coded baseline strategy:

text buy-first 

Phase 2 refactors this baseline into a strategy option and adds two additional trade-only strategies:

text buy-first buy-tick sell-tick 

The purpose of Phase 2 is to compare different execution-price selection rules while keeping the same historical volume profile and market VWAP benchmark.

## Design Principle

The project is intentionally organized into separate components:

text MapReduceProcessor     Reads the raw CSV file.     Filters trade messages.     Aggregates historical bucket volume.     Computes test-date market VWAP components.     Stores test-date bucket trades.  VolumeProfile     Computes AvgQty and RelativeVolume for each bucket.  ExecutionStrategy     Selects execution price from ordered test-date bucket trades.  VWAPExecution     Combines volume profile and selected execution prices.     Builds strategy-specific schedules.     Computes MyVWAP and comparison metrics.  OutputWriter     Writes volume profile, execution schedules, performance summary, and strategy comparison CSV files.  main.cpp     Orchestrates the full pipeline. 

The strategy logic is intentionally not placed inside the mapper or main function.

## Data Scope

Phase 2 uses the same raw input as Phase 1:

text data/SPY_May_2012.csv 

The raw file is large and is not committed to GitHub.

Phase 2 only uses Trade messages. Quote messages are ignored.

The project uses:

text Training period: 05/01/2012 to 05/20/2012 Test date:       05/21/2012 Market window:   13:30:00 <= GMT time < 20:15:00 Bucket size:     15 minutes Target order:    Buy 50,000 shares of SPY 

The timestamps are in GMT. In May 2012, New York was in daylight saving time, so:

text NY time = GMT - 4 hours 

The market window ends at 20:15:00 GMT to include the final 16:00 NY bucket.

## Data Structure Changes

Phase 2 adds a bucket-level trade list for the test date.

cpp struct BucketTrade {     std::string gmtTime;     double price = 0.0;     long quantity = 0; }; 

BucketAggregate now includes:

cpp std::vector<BucketTrade> testTrades; 

The mapper stores test-date trades in their corresponding bucket. The reducer merges these trade lists and sorts them by GMT time.

This is required because buy-tick and sell-tick need access to ordered trades inside each bucket.

## Strategy 1: Buy-First

Buy-first uses the first trade price in each bucket.

text execution_price_i = first_trade_price_i 

This is the Phase 1 baseline. In Phase 2, the logic is moved into ExecutionStrategy.cpp.

The main purpose of this strategy is to preserve a stable baseline and confirm that the Phase 2 refactor does not change the original result.

## Strategy 2: Buy-Tick

Buy-tick uses a simple early price movement heuristic.

For each bucket:

text N = 5 price_1 = first trade price price_N = N-th trade price 

Decision rule:

text if price_N > price_1:     execution_price = price_1 else:     execution_price = price_N 

Interpretation:

- If the first few trades move upward, buy early before the price becomes more expensive.
- If the first few trades do not move upward, wait until the N-th trade.

This is a simple baseline heuristic and not a full microstructure model.

## Strategy 3: Sell-Tick

Sell-tick uses a price decline as an approximation of short-term selling pressure.

For each bucket, scan the ordered trades:

text if price_t < price_{t-1}:     execution_price = price_{t+1} 

If no sell tick is found, the strategy falls back to the first trade price.

This is only a trade-price approximation. It is not true trade direction classification because Phase 2 does not use quote data, bid-ask data, or aggressor-side labels.

## Output Files

Phase 2 generates one schedule file per strategy:

text outputs/execution_schedule_buy_first.csv outputs/execution_schedule_buy_tick.csv outputs/execution_schedule_sell_tick.csv 

Each schedule file contains:

text strategy bucket avg_qty relative_volume order_size execution_gmt_time execution_price executed_value decision_reason 

Phase 2 also generates:

text outputs/strategy_comparison.csv 

The comparison file contains:

text strategy my_vwap market_vwap difference difference_bps result 

For compatibility with Phase 1, the project also writes:

text outputs/performance_summary.csv 

This file stores the buy-first baseline summary.

## Performance Metrics

For each strategy:

text MyVWAP = Sum(OrderSize_i * ExecutionPrice_i) / Sum(OrderSize_i) 

Market VWAP is computed from all test-date trade messages inside the same market window:

text MarketVWAP = Sum(MarketTradePrice_j * MarketTradeQty_j) / Sum(MarketTradeQty_j) 

Difference:

text Difference = MyVWAP - MarketVWAP 

Difference in basis points:

text DifferenceBps = (MyVWAP / MarketVWAP - 1) * 10000 

For a buy order:

text negative DifferenceBps => better than market VWAP positive DifferenceBps => worse than market VWAP 

## Current Limitations

Phase 2 is intentionally simple.

It does not model:

text bid-ask spread quote updates order book depth queue position partial fills market impact aggressor-side labels transaction fees latency 

Therefore, Phase 2 results should be interpreted as trade-price strategy comparisons, not as fully realistic execution simulations.

## Future Extensions

Possible Phase 3 or advanced extensions:

text quote-aware strategy bid-ask spread model imbalance price strategy partial-fill simulation market-impact model implementation shortfall metrics regression-based execution strategy strategy selection based on intraday volatility or drift 

A useful next step is to add quote data and compare whether quote-aware execution prices are more realistic than trade-only price approximations.