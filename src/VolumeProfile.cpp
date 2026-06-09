#include "VolumeProfile.h"
#include "Config.h"
#include "Types.h"

#include <iostream>
#include <vector>
#include <map>
#include <string>


/*
    Build the historical volume profile.

    Input:
        ReducedResult reduced

    reduced contains:
        - historicalQty for each 15-minute bucket
        - trainingDates from 01-MAY-2012 to 20-MAY-2012
        - executionPrice from 21-MAY-2012 for each bucket

    Output:
        vector<VolumeProfileRow>

    Each row contains:
        - bucket
        - avgQty
        - relativeVolume
        - executionPrice

    Formula:
        AvgQty_i = HistoricalQty_i / NumberOfTrainingDays

        RelativeVolume_i = AvgQty_i / Sum(AvgQty_i)
*/


std::vector<VolumeProfileRow> buildVolumeProfile(const Config& config,
                                                 const ReducedResult& reduced)
{
    std::vector<VolumeProfileRow> rows;

    /*
        Number of historical trading days found by all mappers.

        For this project, we expect dates between:
            01-MAY-2012 and 20-MAY-2012

        The actual number should only include trading days found in the data.
    */
    int numTrainingDays = static_cast<int>(reduced.trainingDates.size());

    if (numTrainingDays <= 0)
    {
        std::cerr << "Warning: no training dates found." << std::endl;
        return rows;
    }

    /*
        First pass:
        calculate average quantity for each bucket.

        We also calculate totalAvgQty, which is needed for relative volume.
    */
    long totalAvgQty = 0;

    for (std::map<std::string, BucketAggregate>::const_iterator it =
             reduced.bucketMap.begin();
         it != reduced.bucketMap.end();
         ++it)
    {
        std::string bucketName = it->first;
        const BucketAggregate& bucket = it->second;

        VolumeProfileRow row;

        row.bucket = bucketName;

        /*
            The output used integer average quantity.

                total historical quantity / number of trading days

            Since avgQty is long, this uses integer division.
        */
        row.avgQty = bucket.historicalQty / numTrainingDays;

        row.relativeVolume = 0.0;
        row.executionPrice = bucket.executionPrice;

        rows.push_back(row);

        totalAvgQty += row.avgQty;
    }

    if (totalAvgQty <= 0)
    {
        std::cerr << "Warning: total average quantity is zero." << std::endl;
        return rows;
    }

    /*
        Second pass:
        calculate relative volume for each bucket.

        Formula:
            RelativeVolume_i = AvgQty_i / Sum(AvgQty_i)
    */
    for (size_t i = 0; i < rows.size(); i++)
    {
        rows[i].relativeVolume =
            static_cast<double>(rows[i].avgQty) /
            static_cast<double>(totalAvgQty);
    }

    /*
        Print a small summary for debugging.
    */
    std::cout << "Volume profile built." << std::endl;
    std::cout << "Training days used: " << numTrainingDays << std::endl;
    std::cout << "Total average quantity: " << totalAvgQty << std::endl;
    std::cout << "Number of buckets: " << rows.size() << std::endl;

    return rows;
}