# VWAP Execution Research Project

## Overview

This project implements a C++ MapReduce-style VWAP execution simulator using high-frequency SPY trade data. It started from a homework-style large-file processing assignment and is being extended into a more complete execution research project.

The current version focuses on a Phase 1 baseline: build a historical intraday volume profile from past trade data, split a parent buy order across 15-minute buckets, simulate execution using bucket-level trade prices, and evaluate the resulting execution VWAP against the market VWAP benchmark.

This project is not designed to generate trading alpha or predict future price movement. Instead, it focuses on the execution layer: given a parent order, how should the order be split and evaluated?

## Current Status

Phase 1 baseline is completed.

Implemented features:

- C++ project structure with separate include/ and src/ directories
- MapReduce-style processing using multiple mapper threads
- 15-minute intraday volume profile construction
- Historical volume profile based on 5/1/2012–5/20/2012
- Baseline buy-first execution schedule for 5/21/2012
- MyVWAP calculation
- MarketVWAP calculation
- Difference and difference bps calculation
- CSV output generation

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

text Training days found: 14 Number of buckets: 27 Market quantity on test date: 169,197,102 My VWAP: 131.210 Market VWAP: 131.142 Difference: 0.0674 Difference bps: 5.14 

Interpretation:

The buy-first baseline execution VWAP was approximately 5.14 bps worse than the market VWAP. Since this is a buy order, a higher execution price means higher execution cost.

For a 50,000-share order, the additional cost relative to market VWAP is approximately:

text 0.0674 * 50,000 ≈ $3,371 

This result should be interpreted as execution cost relative to a benchmark, not as trading profit or loss from a directional strategy.

## Output Files

The current Phase 1 pipeline generates:

text outputs/volume_profile.csv outputs/execution_schedule_buy_first.csv outputs/performance_summary.csv 

### volume_profile.csv

Contains:

text bucket, avg_qty, relative_volume, execution_price 

### execution_schedule_buy_first.csv

Contains:

text bucket, avg_qty, relative_volume, order_size, execution_price, executed_value 

### performance_summary.csv

Contains:

text target_shares, my_vwap, market_vwap, difference, difference_bps, result 

## Repository Structure

text include/   Config.h   Types.h   TimeUtils.h   CsvParser.h   MapReduceProcessor.h   VolumeProfile.h   VWAPExecution.h   OutputWriter.h  src/   main.cpp   TimeUtils.cpp   CsvParser.cpp   MapReduceProcessor.cpp   VolumeProfile.cpp   VWAPExecution.cpp   OutputWriter.cpp  data/   SPY_May_2012.csv        # local only, gitignored  outputs/   volume_profile.csv   execution_schedule_buy_first.csv   performance_summary.csv  docs/   data_access.md   phase1_baseline_notes.md  legacy/   original homework implementation files  notebooks/   result_analysis.ipynb   # planned  scripts/   utility scripts 

## Build Instructions

From the repository root:

bash rm -rf build mkdir build cd build cmake .. make 

This creates:

text build/vwap_execution 

## Run Instructions

From the repository root:

bash ./build/vwap_execution data/SPY_May_2012.csv 50000 

The program will generate CSV files under:

text outputs/ 

## Example Console Output

text Training days found: 14 Market quantity on test date: 169197102 Market notional on test date: 2.21889e+10 Volume profile built. Training days used: 14 Total average quantity: 172623637 Number of buckets: 27 Buy-first execution schedule built. Target shares: 50000 Total scheduled order size: 50000 Performance summary computed. My VWAP: 131.21 Market VWAP: 131.142 Difference: 0.0674137 Difference bps: 5.14051 Buy execution result: worse than market VWAP. 

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