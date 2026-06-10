# VWAP Execution Research Project

## Overview

This project implements a C++ MapReduce-style VWAP execution simulator using high-frequency SPY trade data. It started from a homework-style large-file processing assignment and is being extended into a more complete execution research project.

The current version focuses on a Phase 1 baseline: build a historical intraday volume profile from past trade data, split a parent buy order across 15-minute buckets, simulate execution using bucket-level trade prices, and evaluate the resulting execution VWAP against the market VWAP benchmark.

This project is not designed to generate trading alpha or predict future price movement. Instead, it focuses on the execution layer: given a parent order, how should the order be split and evaluated?

## Current Status

Phase 1 completed: baseline VWAP execution engine

Phase 2 completed: trade-only strategy expansion and strategy comparison

## Data

The raw input file is:

text SPY_May_2012.csv 

The file contains SPY trade and quote messages for May 2012 and is larger than 2GB. It is not stored in this GitHub repository.

Expected local path:

text data/SPY_May_2012.csv 

The data file is excluded from Git tracking through .gitignore.


## Important Time Convention

The raw timestamps are in GMT.

For May 2012, New York was observing daylight saving time, so:

text New York time = GMT - 4 hours 

The regular U.S. market session is 9:30–16:00 New York time, corresponding to 13:30–20:00 GMT.

However, this project keeps the original assignment convention:

text TIME_MKT_END = 20:15:00 GMT 

This preserves the final 16:00 New York bucket:

text 20:00:00–20:15:00 GMT 16:00:00–16:15:00 New York time 

This last bucket may contain closing auction prints, close-related trade messages, late trade reports, or small after-hours records. It is kept for consistency with the original homework reference output.

## Methodology

### Parent Order

The assumed parent order is:

text Buy 50,000 shares of SPY on 21-MAY-2012 

### Historical Volume Profile

The model uses trade messages from:

text 01-MAY-2012 to 20-MAY-2012 

For each 15-minute bucket:

text AvgQty_i = HistoricalQty_i / NumberOfTrainingDays 

Then it computes relative volume:

text RelativeVolume_i = AvgQty_i / Sum(AvgQty_i) 

### Execution Schedule

The parent order is split across time buckets:

text OrderSize_i = 50000 * RelativeVolume_i 

For the Phase 1 baseline, each bucket uses the first trade price on 21-MAY-2012 as the execution price:

text ExecutedValue_i = OrderSize_i * ExecutionPrice_i 

### MyVWAP

The simulated execution VWAP is:

text MyVWAP = Sum(OrderSize_i * ExecutionPrice_i) / Sum(OrderSize_i) 

### MarketVWAP

The market VWAP on the test date is calculated using all trade messages in the market window:

text MarketVWAP = Sum(MarketTradePrice_j * MarketTradeQty_j) / Sum(MarketTradeQty_j) 

### Performance Difference

text Difference = MyVWAP - MarketVWAP 

text DifferenceBps = (MyVWAP / MarketVWAP - 1) * 10000 

For a buy order:

- If MyVWAP < MarketVWAP, the execution is better than market VWAP.
- If MyVWAP > MarketVWAP, the execution is worse than market VWAP.

## Phase 1 Result

The Phase 1 baseline produced the following result on 21-MAY-2012:

text Training days found: 14 
Number of buckets: 27 
Market quantity on test date: 169,197,102 
My VWAP: 131.210 
Market VWAP: 131.142 
Difference: 0.0674 
Difference bps: 5.14 

Interpretation:

The buy-first baseline execution VWAP was approximately 5.14 bps worse than the market VWAP. Since this is a buy order, a higher execution price means higher execution cost.

For a 50,000-share order, the additional cost relative to market VWAP is approximately:

text 0.0674 * 50,000 ≈ $3,371 

This result should be interpreted as execution cost relative to a benchmark, not as trading profit or loss from a directional strategy.

## Phase 2: Trade-Only Strategy Expansion

Phase 2 extends the Phase 1 baseline VWAP execution engine into a small multi-strategy execution framework.

The goal is not to build a full market microstructure simulator yet. Instead, Phase 2 keeps the project trade-only and compares several simple execution-price selection rules on top of the same historical volume profile.

### Phase 2 Scope

Included:

- Trade-message-only execution strategies
- Same 15-minute bucket structure as Phase 1
- Same historical volume profile from 05/01/2012 to 05/20/2012
- Same test date: 05/21/2012
- Same target order: buy 50,000 shares of SPY
- Strategy-specific execution schedules
- Strategy comparison output

Not included yet:

- Quote-aware execution
- Bid-ask spread modeling
- Order book imbalance
- Partial fills
- Market impact modeling
- Aggressor-side classification
- Regression-based strategy

These can be added in later advanced phases.

### Implemented Strategies

#### 1. Buy-First

This is the Phase 1 baseline strategy.

For each 15-minute bucket on the test date, the strategy uses the first trade price in that bucket as the execution price.

text execution_price_i = first_trade_price_i 

This strategy is simple and serves as the regression check for Phase 2. After refactoring, the buy-first result should remain close to the original Phase 1 result.

#### 2. Buy-Tick

This is a trade-only price movement heuristic.

For each bucket, the strategy looks at the first N trades. The current default is:

text N = 5 

If the N-th trade price is higher than the first trade price, the strategy treats this as early upward pressure and buys immediately at the first trade price. Otherwise, it waits until the N-th trade.

text if price_N > price_1:     execution_price = price_1 else:     execution_price = price_N 

This is only a simple heuristic. It does not use quotes, bid-ask spread, order book state, or true trade direction labels.

#### 3. Sell-Tick

This is also a trade-only approximation.

For each bucket, the strategy scans trades from the beginning of the bucket. If it finds a price decline,

text price_t < price_{t-1} 

then it treats this as a sell tick and buys at the next trade price.

text if price_t < price_{t-1}:     execution_price = price_{t+1} 

If no sell tick is found, the strategy falls back to the first trade price in the bucket.

This is not true aggressor-side classification. It is only a price-movement approximation because Phase 2 does not use quote or order book data.

### Phase 2 Output Files

After running the project, Phase 2 writes:

text outputs/volume_profile.csv outputs/execution_schedule_buy_first.csv outputs/execution_schedule_buy_tick.csv outputs/execution_schedule_sell_tick.csv outputs/performance_summary.csv outputs/strategy_comparison.csv 

The strategy-specific schedule files contain:

text strategy bucket avg_qty relative_volume order_size execution_gmt_time execution_price executed_value decision_reason 

The comparison file contains:

text strategy my_vwap market_vwap difference difference_bps result 

### Running the Project

From the repository root:

bash rm -rf build cmake -S . -B build cmake --build build  ./build/vwap_execution 

The raw data file is not included in GitHub because it is large. The expected local path is:

text data/SPY_May_2012.csv 

The program should be run from the repository root so that the relative data path resolves correctly.

### Interpreting the Results

For a buy order:

text MyVWAP < MarketVWAP  => better than market VWAP MyVWAP > MarketVWAP  => worse than market VWAP 

The difference in basis points is computed as:

text DifferenceBps = (MyVWAP / MarketVWAP - 1) * 10000 

The Phase 1 buy-first baseline result on 05/21/2012 was approximately:

text My VWAP:      131.210 Market VWAP:  131.142 Difference:   0.0674 Difference bps: 5.14 

This means the buy-first baseline was about 5.14 bps worse than the market VWAP for a 50,000-share buy order.

Phase 2 compares whether the simple buy-tick and sell-tick heuristics improve or worsen execution relative to this baseline.




## Run Instructions

From the repository root:

bash ./build/vwap_execution data/SPY_May_2012.csv 50000 

The program will generate CSV files under:

text outputs/ 


## Project Scope

This project focuses on the execution layer.

It assumes that a parent order has already been generated by an external strategy or portfolio process. The current task is not to decide whether SPY should be bought or sold. Instead, the task is to evaluate how well a large order can be executed relative to a VWAP benchmark.

## Roadmap

### Phase 1: Baseline VWAP Execution Engine

Completed.

- MapReduce-style C++ processing
- Historical 15-minute volume profile
- Buy-first execution schedule
- MyVWAP and MarketVWAP comparison
- CSV outputs

### Phase 2: Strategy Expansion

Planned.

Potential extensions:

- Buy-first strategy
- Buy-tick strategy
- Sell-tick strategy
- Regression-based execution price strategy
- Quote-aware execution simulation
- Bid/ask imbalance price
- Partial fill simulation
- Carryover of unfilled quantity

### Phase 3: Multi-Day Backtest

Planned.

Potential extensions:

- Rolling 20-day volume profile
- Multi-day VWAP execution testing
- Average performance
- Win rate
- Standard deviation
- Best and worst execution days

### Phase 4: Python Analysis Notebook

Planned.

Potential visualizations:

- MyVWAP vs MarketVWAP
- Difference bps over time
- Order size distribution
- Historical volume profile
- Strategy comparison
