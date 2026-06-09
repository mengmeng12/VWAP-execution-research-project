#include "Config.h"
#include "Types.h"
#include "MapReduceProcessor.h"
#include "VolumeProfile.h"
#include "VWAPExecution.h"
#include "OutputWriter.h"

#include <iostream>
#include <string>
#include <cstdlib>


/*
    Main entry point for Phase 1 baseline.

    Phase 1 goal:
        Build a baseline VWAP execution engine using MapReduce-style C++.

    Input:
        data/SPY_May_2012.csv

    Outputs:
        outputs/volume_profile.csv
        outputs/execution_schedule_buy_first.csv
        outputs/performance_summary.csv

    Default task:
        Buy 50,000 shares of SPY on 21-MAY-2012 using historical
        15-minute volume profile from 01-MAY-2012 to 20-MAY-2012.
*/


static void printConfig(const Config& config)
{
    std::cout << "==============================" << std::endl;
    std::cout << "VWAP Execution Baseline Config" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout << "Input file: " << config.inputFile << std::endl;
    std::cout << "Symbol: " << config.symbol << std::endl;

    std::cout << "Training start date: "
              << config.trainingStartDate
              << std::endl;

    std::cout << "Training end date: "
              << config.trainingEndDate
              << std::endl;

    std::cout << "Test date: " << config.testDate << std::endl;

    std::cout << "Market start GMT: "
              << config.marketStartGmt
              << std::endl;

    std::cout << "Market end GMT: "
              << config.marketEndGmt
              << std::endl;

    std::cout << "Number of mappers: "
              << config.numMappers
              << std::endl;

    std::cout << "Target shares: "
              << config.targetShares
              << std::endl;

    std::cout << "Output directory: "
              << config.outputDir
              << std::endl;

    std::cout << "==============================" << std::endl;
}


static void updateConfigFromCommandLine(Config& config,
                                        int argc,
                                        char* argv[])
{
    if (argc >= 2)
    {
        config.inputFile = argv[1];
    }

    if (argc >= 3)
    {
        long targetShares = std::atol(argv[2]);

        if (targetShares > 0)
        {
            config.targetShares = targetShares;
        }
        else
        {
            std::cerr << "Warning: invalid target shares argument. "
                      << "Using default value: "
                      << config.targetShares
                      << std::endl;
        }
    }
}


int main(int argc, char* argv[])
{
    /*
        Step 1:
        Create default config.
    */
    Config config;

    /*
        Step 2:
        Optionally update config from command-line arguments.
    */
    updateConfigFromCommandLine(config, argc, argv);

    /*
        Step 3:
        Print config for debugging and reproducibility.
    */
    printConfig(config);

    /*
        Step 4:
        Run MapReduce-style processing.

        This reads the large CSV file and returns:
            - historical bucket quantities
            - 5/21 execution prices
            - 5/21 market VWAP components
    */
    ReducedResult reduced = runMapReduce(config);

    if (reduced.trainingDates.empty())
    {
        std::cerr << "Error: no training dates found. "
                  << "Please check the input file path and date format."
                  << std::endl;

        return 1;
    }

    /*
        Step 5:
        Build historical volume profile.

        This computes:
            AvgQty_i
            RelativeVolume_i
    */
    std::vector<VolumeProfileRow> volumeProfile =
        buildVolumeProfile(config, reduced);

    if (volumeProfile.empty())
    {
        std::cerr << "Error: volume profile is empty." << std::endl;
        return 1;
    }

    /*
        Step 6:
        Build baseline buy-first execution schedule.

        This computes:
            OrderSize_i
            ExecutedValue_i
    */
    std::vector<ExecutionScheduleRow> schedule =
        buildBuyFirstSchedule(config, volumeProfile);

    if (schedule.empty())
    {
        std::cerr << "Error: execution schedule is empty." << std::endl;
        return 1;
    }

    /*
        Step 7:
        Compute performance summary.

        This computes:
            MyVWAP
            MarketVWAP
            Difference
            DifferenceBps
    */
    PerformanceSummary summary =
        computePerformanceSummary(config, schedule, reduced);

    /*
        Step 8:
        Write output CSV files.
    */
    std::string volumeProfilePath =
        config.outputDir + "/volume_profile.csv";

    std::string executionSchedulePath =
        config.outputDir + "/execution_schedule_buy_first.csv";

    std::string performanceSummaryPath =
        config.outputDir + "/performance_summary.csv";

    writeVolumeProfile(volumeProfilePath, volumeProfile);
    writeExecutionSchedule(executionSchedulePath, schedule);
    writePerformanceSummary(performanceSummaryPath, summary);

    /*
        Step 9:
        Print final result.
    */
    std::cout << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Phase 1 Baseline Finished" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout << "My VWAP: "
              << summary.myVWAP
              << std::endl;

    std::cout << "Market VWAP: "
              << summary.marketVWAP
              << std::endl;

    std::cout << "Difference: "
              << summary.difference
              << std::endl;

    std::cout << "Difference bps: "
              << summary.differenceBps
              << std::endl;

    if (summary.myVWAP > 0.0 && summary.marketVWAP > 0.0)
    {
        if (summary.myVWAP < summary.marketVWAP)
        {
            std::cout << "Result: better than market VWAP for buy order."
                      << std::endl;
        }
        else if (summary.myVWAP > summary.marketVWAP)
        {
            std::cout << "Result: worse than market VWAP for buy order."
                      << std::endl;
        }
        else
        {
            std::cout << "Result: equal to market VWAP."
                      << std::endl;
        }
    }

    std::cout << "Output files:" << std::endl;
    std::cout << "  " << volumeProfilePath << std::endl;
    std::cout << "  " << executionSchedulePath << std::endl;
    std::cout << "  " << performanceSummaryPath << std::endl;

    return 0;
}