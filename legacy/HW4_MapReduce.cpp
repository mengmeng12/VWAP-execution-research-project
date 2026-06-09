
#include <vector>
#include <thread>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <string>
#include <map>
#include <utility>
#include <time.h>

using namespace std;

const string FILE_NAME = "SPY_May_2012.csv";
const string SYMBOL = "SPY";
const char DELIMITER = ',';

const int BUF_SIZE = 2048;
const int DATE_SIZE = 12;
const int TIME_SIZE = 13;
const int LEN_TIME_POINTS = 15;
const int NUM_MAPPERS = 26;

typedef pair<string, string> TimePointPair;

// Time offset is -4
const string TIME_MKT_START = "13:30:00";
const string TIME_MKT_END = "20:15:00";

const string TRADING_DATE = "21-MAY-2012";

TimePointPair sTimePoints[] = {
	TimePointPair(string("09:30:00"), string("13:30:00")),
	TimePointPair(string("09:45:00"), string("13:45:00")),
	TimePointPair(string("10:00:00"), string("14:00:00")),
	TimePointPair(string("10:15:00"), string("14:15:00")),
	TimePointPair(string("10:30:00"), string("14:30:00")),
	TimePointPair(string("10:45:00"), string("14:45:00")),
	TimePointPair(string("11:00:00"), string("15:00:00")),
	TimePointPair(string("11:15:00"), string("15:15:00")),
	TimePointPair(string("11:30:00"), string("15:30:00")),
	TimePointPair(string("11:45:00"), string("15:45:00")),
	TimePointPair(string("12:00:00"), string("16:00:00")),
	TimePointPair(string("12:15:00"), string("16:15:00")),
	TimePointPair(string("12:30:00"), string("16:30:00")),
	TimePointPair(string("12:45:00"), string("16:45:00")),
	TimePointPair(string("13:00:00"), string("17:00:00")),
	TimePointPair(string("13:15:00"), string("17:15:00")),
	TimePointPair(string("13:30:00"), string("17:30:00")),
	TimePointPair(string("13:45:00"), string("17:45:00")),
	TimePointPair(string("14:00:00"), string("18:00:00")),
	TimePointPair(string("14:15:00"), string("18:15:00")),
	TimePointPair(string("14:30:00"), string("18:30:00")),
	TimePointPair(string("14:45:00"), string("18:45:00")),
	TimePointPair(string("15:00:00"), string("19:00:00")),
	TimePointPair(string("15:15:00"), string("19:15:00")),
	TimePointPair(string("15:30:00"), string("19:30:00")),
	TimePointPair(string("15:45:00"), string("19:45:00")),
	TimePointPair(string("16:00:00"), string("20:00:00")),	
	TimePointPair(string("16:00:00"), string("20:15:00"))
};

struct MapResult
{

	map< string, pair<long, double> > QtyPriceMap;
	map< string, string > PriceTimeMap;
	set<string> TradingDates;
	int rc;
	MapResult() : rc(0) {}
};

void initQtyPriceMap(map< string, pair<long, double> > &QtyPriceMap);
void mapperTask(long startPos, long endPos, MapResult &result);
int reduceAndOutput(vector<MapResult> &results);
string sConvertToKey(char *pTime);

int main(int argc, char* argv[])
{
	auto start = time(NULL);
    FILE *inputFile = fopen(FILE_NAME.c_str(), "r");

    if (inputFile == NULL)
    {
	    cout << "Open file failed" << endl;
	    return -1;
    }

    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fclose(inputFile);

    vector<MapResult> results(NUM_MAPPERS);
    vector<thread> workers;

    long chunkSize = fileSize / NUM_MAPPERS;

    for (int i = 0; i < NUM_MAPPERS; i++)
    {
	    long startPos = i * chunkSize;
	    long endPos = (i == NUM_MAPPERS - 1) ? fileSize : (i + 1) * chunkSize;
	    workers.push_back(thread(mapperTask, startPos, endPos, ref(results[i])));
    }   

    for (int i = 0; i < NUM_MAPPERS; i++)
    {
	    workers[i].join();
    }

    for (int i = 0; i < NUM_MAPPERS; i++)
    {
	    if (results[i].rc != 0)
	    {
		    cerr << "Mapper " << i << " failed" << endl;
		    return -1;
	    }
    }

    int iRC = reduceAndOutput(results);

    if (iRC != 0)
    {
	    cerr << "Reducer failed" << endl;
	    return -1;
    }
	cout << endl;

	auto end = time(NULL);
	cout << "Time taken to fetch all the data: " << (end - start) << " seconds" << endl;

	return 0;
}

void mapperTask(long startPos, long endPos, MapResult &result)
{
	initQtyPriceMap(result.QtyPriceMap);
	FILE *f = fopen(FILE_NAME.c_str(), "r");
	if (f == NULL)
	{
		cerr << "Open file failed in mapper" << endl;
		result.rc = -1;
		return;
	}
	fseek(f, startPos, SEEK_SET);
	char buffer[BUF_SIZE];
	if (startPos != 0)
	{
		fgets(buffer, BUF_SIZE, f);
	}
	while ( ftell(f) < endPos && fgets(buffer, BUF_SIZE, f) )
	{
		char date1[DATE_SIZE + 1];
		char time[TIME_SIZE + 1];
		double price = 0.0;
		int qty = 0;
		char *p = NULL;
		char *p1 = NULL;
		char *p2 = NULL;
		char *p3 = NULL;
		memset(date1, '\0', DATE_SIZE + 1);
		memset(time, '\0', TIME_SIZE + 1);
        p = strstr(buffer, "Trade");
		if ( p != NULL )
		{
			p1 = strstr(buffer, SYMBOL.c_str());
			if ( p1 == 0 )
			{
				cout << "Invalid line in the file "<< buffer << endl;
				continue;
			}
			p2 = strchr(p1, DELIMITER);
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			*p2 = '\0';
			strncpy(date1, p1, DATE_SIZE);
			date1[DATE_SIZE] = '\0';
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			*p2 = '\0';
			strncpy(time, p1, TIME_SIZE);
			time[TIME_SIZE] = '\0';
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			*p2 = '\0';
			price = (double)atof(p1);
			p2++;
			p1 = p2;
			p2 = strchr(p1, DELIMITER);
			*p2 = '\0';
			qty = atoi(p1);

			p3 = strchr(time, '.');
			if (p3 != NULL)
			{
				*p3 = '\0';
			}

			if ( string(time).compare(TIME_MKT_START) < 0 || string(time).compare(TIME_MKT_END) >= 0 )
				continue;

			string key = sConvertToKey(time);

            if (string(date1).compare(TRADING_DATE) < 0)
            {
	            result.QtyPriceMap[key].first += qty;
	            result.TradingDates.insert(string(date1));
            }
            else if (string(date1).compare(TRADING_DATE) == 0)
            {
	            if (result.PriceTimeMap.find(key) == result.PriceTimeMap.end() ||
		            string(time).compare(result.PriceTimeMap[key]) < 0)
	            {
		            result.PriceTimeMap[key] = string(time);
		            result.QtyPriceMap[key].second = price;
	            }
            }
            else
            {
	            break;
            }
		}
	}
    fclose(f);
	if (result.QtyPriceMap.empty()) {
		cerr << "QtyPriceMap is empty" << endl;
		result.rc = -1;
        return;
	}

	// As count also includes the 5/21/2012 which is for trading price, not trading qty,
	result.rc = 0;
	return;
}

int reduceAndOutput(vector<MapResult> &results)
{
	map< string, pair<long, double> > FinalMap;
	map< string, string > FinalPriceTimeMap;
	set<string> AllTradingDates;

	initQtyPriceMap(FinalMap);

	for (unsigned int i = 0; i < results.size(); i++)
	{
		for (map< string, pair<long, double> >::iterator it = results[i].QtyPriceMap.begin();
			 it != results[i].QtyPriceMap.end(); it++)
		{
			string key = it->first;

			FinalMap[key].first += it->second.first;

			if (it->second.second != 0.0)
			{
				if (FinalPriceTimeMap.find(key) == FinalPriceTimeMap.end() ||
					results[i].PriceTimeMap[key].compare(FinalPriceTimeMap[key]) < 0)
				{
					FinalPriceTimeMap[key] = results[i].PriceTimeMap[key];
					FinalMap[key].second = it->second.second;
				}
			}
		}

		for (set<string>::iterator dateIt = results[i].TradingDates.begin();
			 dateIt != results[i].TradingDates.end(); dateIt++)
		{
			AllTradingDates.insert(*dateIt);
		}
	}

	int numDays = AllTradingDates.size();

	if (numDays == 0)
	{
		cerr << "No trading dates found for quantity calculation" << endl;
		return -1;
	}

	cout << "num of days for qty calculation = " << numDays << endl;

	ofstream fout;
	fout.open("outputs.csv");

	if (!fout.is_open())
	{
		cerr << "Cannot open outputs.csv" << endl;
		return -1;
	}

	fout << "Time,AvgQty,Price" << endl;

	for (unsigned int i = 0; i < sizeof(sTimePoints) / sizeof(sTimePoints[0]) - 1; i++)
	{
		string key = sTimePoints[i].first;
		long avgQty = (long)(FinalMap[key].first / numDays);

		cout << key
			 << setiosflags(ios::fixed) << setprecision(2)
			 << setw(15) << avgQty << "	"
			 << FinalMap[key].second << endl;

		fout << key << ","
			 << avgQty << ","
			 << fixed << setprecision(2)
			 << FinalMap[key].second << endl;
	}

	fout.close();

	return 0;
}

string sConvertToKey(char *pTime)
{
	string sTime = string(pTime);
	for (unsigned int i = 0; i< sizeof(sTimePoints); i++)
	{
		if ( sTime.compare(sTimePoints[i].second) == 0 )
			return sTimePoints[i].first;
		else if ( sTime.compare(sTimePoints[i].second) < 0 )
			return sTimePoints[--i].first;
	}
	cout << "Wrong sTime = " << sTime << endl;
	return sTime;
}

void initQtyPriceMap(map< string, pair<long, double> > &QtyPriceMap)
{
	pair<long, double> Zero(0, 0.0);
	for (unsigned int i = 0; i < sizeof(sTimePoints) / sizeof(sTimePoints[0]) - 1; i++)
	{
		QtyPriceMap[sTimePoints[i].first] = Zero;
	}
}