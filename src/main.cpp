#include "Config.h"
#include "Types.h"
#include "MapReduceProcessor.h"
#include "VolumeProfile.h"
#include "VWAPExecution.h"
#include "ExecutionStrategy.h"
#include "OutputWriter.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

/*
 * Print the configuration used by this run.
 *
 * This helps keep the experiment reproducible.
 */
static void printConfig(const Config& config) {
    std::cout << "==============================" << std::endl;
    std::cout << "VWAP Execution Phase 2 Config" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout << "Input file: " << config.inputFile << std::endl;
    std::cout << "Symbol: " << config.symbol << std::endl;
    std::cout << "Training start date: " << config.trainingStartDate << std::endl;
    std::cout << "Training end date: " << config.trainingEndDate << std::endl;
    std::cout << "Test date: " << config.testDate << std::endl;
    std::cout << "Market start GMT: " << config.marketStartGmt << std::endl;
    std::cout << "Market end GMT: " << config.marketEndGmt << std::endl;
    std::cout << "Number of mappers: " << config.numMappers << std::endl;
    std::cout << "Target shares: " << config.targetShares << std::endl;
    std::cout << "Output directory: " << config.outputDir << std::endl;

    std::cout << "==============================" << std::endl;
}

/*
 * Optional command-line arguments:
 *
 * argv[1]: input file path
 * argv[2]: target shares
 *
 * Example:
 *   ./build/vwap_execution data/SPY_May_2012.csv 50000
 */
static void updateConfigFromCommandLine(Config& config, int argc, char* argv[]) {
    if (argc >= 2) {
        config.inputFile = argv[1];
    }

    if (argc >= 3) {
        long targetShares = std::atol(argv[2]);

        if (targetShares > 0) {
            config.targetShares = targetShares;
        } else {
            std::cerr << "Warning: invalid target shares argument. "
                      << "Using default value: "
                      << config.targetShares
                      << std::endl;
        }
    }
}

/*
 * Print one strategy result in a compact format.
 */
static void printStrategyResult(const PerformanceSummary& summary) {
    std::cout << summary.strategy
              << " | My VWAP: " << summary.myVWAP
              << " | Market VWAP: " << summary.marketVWAP
              << " | Difference: " << summary.difference
              << " | Difference bps: " << summary.differenceBps
              << " | Result: " << summary.result
              << std::endl;
}

int main(int argc, char* argv[]) {
    /*
     * Step 1:
     * Create default config and optionally update it from command-line arguments.
     */
    Config config;
    updateConfigFromCommandLine(config, argc, argv);

    /*
     * Step 2:
     * Print config for debugging and reproducibility.
     */
    printConfig(config);

    /*
     * Step 3:
     * Run MapReduce-style processing.
     *
     * This reads the large CSV file and returns:
     * - historical bucket quantities
     * - test-date bucket trades
     * - test-date market VWAP components
     */
    ReducedResult reduced = runMapReduce(config);

    if (reduced.trainingDates.empty()) {
        std::cerr << "Error: no training dates found. "
                  << "Please check the input file path and date format."
                  << std::endl;
        return 1;
    }

    /*
     * Step 4:
     * Build historical volume profile.
     *
     * This computes:
     * - AvgQty_i
     * - RelativeVolume_i
     */
    std::vector<VolumeProfileRow> volumeProfile = buildVolumeProfile(config, reduced);

    if (volumeProfile.empty()) {
        std::cerr << "Error: volume profile is empty." << std::endl;
        return 1;
    }

    /*
     * Step 5:
     * Write volume profile once.
     *
     * Volume profile is shared by all execution strategies.
     */
    std::string volumeProfilePath = config.outputDir + "/volume_profile.csv";
    writeVolumeProfile(volumeProfilePath, volumeProfile);

    /*
     * Step 6:
     * Define Phase 2 strategies.
     *
     * These are all trade-only strategies.
     * No quote-aware, bid-ask, imbalance, or partial-fill logic is used here.
     */
    std::vector<ExecutionStrategyType> strategies;
    strategies.push_back(ExecutionStrategyType::BUY_FIRST);
    strategies.push_back(ExecutionStrategyType::BUY_TICK);
    strategies.push_back(ExecutionStrategyType::SELL_TICK);

    /*
     * Strategy parameters.
     *
     * buyTickLookahead = 5 means BUY_TICK checks the first 5 trades
     * inside each bucket.
     */
    StrategyConfig strategyConfig;
    strategyConfig.buyTickLookahead = 5;

    /*
     * Step 7:
     * Run every strategy and collect performance summaries.
     */
    std::vector<PerformanceSummary> strategySummaries;

    for (size_t i = 0; i < strategies.size(); i++) {
        ExecutionStrategyType strategy = strategies[i];
        std::string name = strategyName(strategy);

        std::cout << std::endl;
        std::cout << "Running strategy: " << name << std::endl;

        /*
         * Build one strategy-specific execution schedule.
         */
        std::vector<ExecutionScheduleRow> schedule = buildExecutionSchedule(
            config,
            volumeProfile,
            reduced,
            strategy,
            strategyConfig
        );

        if (schedule.empty()) {
            std::cerr << "Warning: execution schedule is empty for strategy: "
                      << name
                      << std::endl;
            continue;
        }

        /*
         * Compute strategy-specific performance.
         */
        PerformanceSummary summary = computePerformanceSummary(
            config,
            schedule,
            reduced
        );

        /*
         * Make sure strategy name is set.
         */
        summary.strategy = name;

        /*
         * Write strategy-specific execution schedule.
         */
        std::string schedulePath =
            config.outputDir + "/execution_schedule_" + name + ".csv";

        writeExecutionSchedule(schedulePath, schedule);

        /*
         * Save summary for final comparison table.
         */
        strategySummaries.push_back(summary);
    }

    if (strategySummaries.empty()) {
        std::cerr << "Error: no strategy summaries were generated." << std::endl;
        return 1;
    }

    /*
     * Step 8:
     * Write Phase 2 strategy comparison.
     */
    std::string strategyComparisonPath =
        config.outputDir + "/strategy_comparison.csv";

    writeStrategyComparison(strategyComparisonPath, strategySummaries);

    /*
     * Step 9:
     * Keep Phase 1-compatible performance_summary.csv.
     *
     * This writes the buy_first result as the baseline summary.
     */
    std::string performanceSummaryPath =
        config.outputDir + "/performance_summary.csv";

    writePerformanceSummary(performanceSummaryPath, strategySummaries[0]);

    /*
     * Step 10:
     * Print final results.
     */
    std::cout << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Phase 2 Strategy Expansion Finished" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout << "Strategy comparison:" << std::endl;

    for (size_t i = 0; i < strategySummaries.size(); i++) {
        printStrategyResult(strategySummaries[i]);
    }

    std::cout << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  " << volumeProfilePath << std::endl;

    for (size_t i = 0; i < strategies.size(); i++) {
        std::string name = strategyName(strategies[i]);
        std::cout << "  "
                  << config.outputDir
                  << "/execution_schedule_"
                  << name
                  << ".csv"
                  << std::endl;
    }

    std::cout << "  " << strategyComparisonPath << std::endl;
    std::cout << "  " << performanceSummaryPath << std::endl;

    return 0;
}