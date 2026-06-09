#pragma once

#include <string>

std::string stripMilliseconds(const std::string& time);
bool isWithinMarketWindow(const std::string& gmtTime,
                          const std::string& marketStartGmt,
                          const std::string& marketEndGmt);

std::string convertGmtTimeToNyBucket(const std::string& gmtTime);
bool isTrainingDate(const std::string& date,
                    const std::string& trainingStartDate,
                    const std::string& trainingEndDate);

bool isTestDate(const std::string& date,
                const std::string& testDate);