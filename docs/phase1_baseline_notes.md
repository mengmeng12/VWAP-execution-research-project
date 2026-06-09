# Phase 1 Baseline Notes

## 1. Objective

The goal of Phase 1 is to build a baseline VWAP execution engine from high-frequency SPY trade data.

The original assignment only required computing historical average quantity and a bucket-level execution price. This project extends that baseline into a complete execution evaluation pipeline.

Phase 1 answers the following question:

text If we need to buy 50,000 shares of SPY on 21-MAY-2012, and we split the order according to the historical 15-minute volume profile from 01-MAY-2012 to 20-MAY-2012, how does our simulated execution VWAP compare against the actual market VWAP? 

## 2. What This Project Is and Is Not

This project is an execution simulation project.

It is not an alpha strategy project.

The difference is important:

text Alpha strategy: Decides what to buy or sell, when to trade, and how large the parent order should be.  Execution strategy: Takes a given parent order and decides how to split and execute it over time. 

In this project, the parent order is assumed:

text Buy 50,000 shares of SPY on 21-MAY-2012 

The project focuses on how to execute that order and how to evaluate execution quality.

## 3. Data Window

### Training Period

text 01-MAY-2012 to 20-MAY-2012 

This period is used to estimate the historical intraday volume profile.

The program found 14 trading days in this period. This is reasonable because weekends are included in the calendar range but do not contribute trading data.

### Test Date

text 21-MAY-2012 

This date is used for simulated execution and market VWAP evaluation.

## 4. Trade Messages Only

Phase 1 only uses Trade messages.

Quote messages are ignored.

This is consistent with the original assignment requirement. The quote data may be used in a later phase for quote-aware execution simulation.

## 5. Time Zone Convention

The raw timestamps are GMT.

In May 2012, New York was in daylight saving time:

text New York time = GMT - 4 hours 

Therefore:

text 09:30:00 New York = 13:30:00 GMT 16:00:00 New York = 20:00:00 GMT 

## 6. Why the Market End Time Is 20:15 GMT

The original reference version used:

text TIME_MKT_END = 20:15:00 

Although the regular U.S. session ends at 20:00 GMT, this project keeps 20:15 GMT as the right boundary of the final bucket.

This means the final bucket is:

text 20:00:00–20:15:00 GMT 16:00:00–16:15:00 New York time 

This bucket may contain close-related trade records, closing auction prints, late trade reports, or small after-hours records.

Keeping this bucket helps preserve consistency with the original homework output.

## 7. Number of Buckets

The pipeline produced 27 buckets.

This is expected under the current time-window convention.

A regular 9:30–16:00 session with 15-minute intervals has 26 standard buckets ending at 16:00. By keeping the additional 16:00 bucket with a 20:15 GMT right boundary, the project includes 27 bucket labels.

The last bucket is labeled:

text 16:00:00 

## 8. Historical Volume Profile

For each 15-minute bucket, the program aggregates trade quantity across the training dates.

Then it computes:

text AvgQty_i = HistoricalQty_i / NumberOfTrainingDays 

For this run:

text NumberOfTrainingDays = 14 

After computing average quantity, the program calculates relative volume:

text RelativeVolume_i = AvgQty_i / Sum(AvgQty_i) 

This relative volume is used to split the parent order.

## 9. Parent Order Allocation

The parent order is:

text 50,000 shares 

For each bucket:

text OrderSize_i = 50,000 * RelativeVolume_i 

The sum of all scheduled order sizes is approximately 50,000 shares.

In the current Phase 1 implementation, orderSize is stored as a double. This avoids rounding complexity in the baseline version.

A future improvement could round order sizes to integer shares while ensuring the total remains exactly 50,000 shares.

## 10. Baseline Execution Price

The Phase 1 strategy is called the buy-first baseline.

For each 15-minute bucket on 21-MAY-2012, the program uses the first trade price in that bucket as the simulated execution price.

This is a simple baseline assumption:

text ExecutionPrice_i = first trade price in bucket i on the test date 

This is not necessarily optimal. It is only the first baseline to compare against future strategies.

## 11. MyVWAP

The simulated execution VWAP is:

text MyVWAP = Sum(OrderSize_i * ExecutionPrice_i) / Sum(OrderSize_i) 

For the Phase 1 run:

text MyVWAP = 131.210 

This means the simulated strategy bought SPY at an average price of approximately $131.210 per share.

## 12. MarketVWAP

The market VWAP is calculated using all trade messages on the test date inside the configured time window:

text MarketVWAP = Sum(MarketTradePrice_j * MarketTradeQty_j) / Sum(MarketTradeQty_j) 

For the Phase 1 run:

text Market quantity: 169,197,102 shares Market notional: approximately 22.1889 billion MarketVWAP: 131.142 

This means the overall market traded SPY at an average price of approximately $131.142 per share during the selected window.

## 13. Difference and Difference Bps

The raw difference is:

text Difference = MyVWAP - MarketVWAP 

For this run:

text Difference = 0.0674137 

The bps difference is:

text DifferenceBps = (MyVWAP / MarketVWAP - 1) * 10000 

For this run:

text DifferenceBps = 5.14051 

This means the baseline execution was approximately 5.14 basis points above the market VWAP.

## 14. Buy Order Interpretation

For a buy order, lower execution price is better.

Therefore:

text MyVWAP < MarketVWAP  => better than market VWAP MyVWAP > MarketVWAP  => worse than market VWAP 

In this run:

text MyVWAP = 131.210 MarketVWAP = 131.142 

Because MyVWAP is higher than MarketVWAP, the buy-first baseline was worse than the market VWAP benchmark.

## 15. Approximate Cost Impact

The per-share difference is:

text 0.0674137 dollars per share 

For a 50,000-share order:

text 0.0674137 * 50,000 ≈ 3,371 dollars 

So the baseline execution cost was approximately $3,371 worse than the market VWAP benchmark.

This should be interpreted as execution cost relative to a benchmark, not as overall trading profit or loss.

## 16. Why This Does Not Mean the Strategy Lost Money

This project does not decide whether SPY should be bought.

It assumes that another strategy, portfolio manager, or parent order generator has already decided to buy 50,000 shares.

The VWAP execution engine only evaluates how efficiently the order was executed.

Even if MyVWAP < MarketVWAP, that does not mean the trader made money. It only means the trader bought more cheaply than the market VWAP benchmark.

Even if MyVWAP > MarketVWAP, that does not necessarily mean the overall trade was unprofitable. It only means the execution was worse than the VWAP benchmark.

## 17. Current Output Files

Phase 1 generates three output files.

### outputs/volume_profile.csv

Contains the historical volume profile:

text bucket, avg_qty, relative_volume, execution_price 

### outputs/execution_schedule_buy_first.csv

Contains the simulated execution schedule:

text bucket, avg_qty, relative_volume, order_size, execution_price, executed_value 

### outputs/performance_summary.csv

Contains the final performance evaluation:

text target_shares, my_vwap, market_vwap, difference, difference_bps, result 

## 18. Current Limitations

The Phase 1 baseline has several limitations.

First, it only tests one day:

text 21-MAY-2012 

Second, the execution price is simplified as the first trade price in each bucket.

Third, it uses trade data only and ignores quote information.

Fourth, it does not model market impact, bid-ask spread, partial fills, queue position, or unfilled quantity carryover.

Fifth, order sizes are not rounded to integer shares in the current baseline implementation.

These limitations are acceptable for Phase 1 because the goal is to build a clean baseline pipeline before adding more realistic execution logic.

## 19. Next Steps

Possible Phase 2 extensions:

text 1. Buy-first strategy refinement 2. Buy-tick strategy 3. Sell-tick strategy 4. Regression-based execution price prediction 5. Quote-aware execution simulation 6. Bid/ask imbalance price 7. Partial fill simulation 8. Carryover of unfilled quantity 

Possible Phase 3 extensions:

text 1. Multi-day backtest 2. Rolling 20-day historical volume profile 3. Average performance 4. Win rate 5. Standard deviation 6. Best and worst execution days 

Possible Phase 4 extensions:

text 1. Python result analysis notebook 2. MyVWAP vs MarketVWAP plot 3. Difference bps plot 4. Order size distribution plot 5. Strategy comparison charts 

## 20. Summary

Phase 1 successfully converts the original homework-style MapReduce assignment into a structured VWAP execution baseline.

The current pipeline can:

text 1. Read a large 2GB+ trade/quote CSV file 2. Process the file using multiple mapper threads 3. Build a 15-minute historical volume profile 4. Allocate a 50,000-share parent order 5. Simulate buy-first execution 6. Compute MyVWAP 7. Compute MarketVWAP 8. Report execution difference in bps 9. Write structured CSV outputs 

This provides a clean foundation for more advanced execution strategies and multi-day backtesting.