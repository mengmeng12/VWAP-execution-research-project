
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

int iExtractTimeIntervalQtyPrice(FILE *f, map< string, pair<long, double> > & QtyPriceMap, int & count);
string sConvertToKey(char *pTime);

int main(int argc, char* argv[])
{
	auto start = time(NULL);
	pair<long, double> Zero(0, 0.0);
	std::map< string, pair<long, double> > TimeIntervalQtyPrice;

	for (unsigned int i = 0; i < sizeof(sTimePoints)/sizeof(sTimePoints[1])-1; i++)
	{
		TimeIntervalQtyPrice[sTimePoints[i].first] = Zero;
	}

	std::map< string, pair<long, double> >::iterator mapIt = TimeIntervalQtyPrice.begin();

	FILE * inputFile = fopen(FILE_NAME.c_str(), "r");

	ofstream fout;
	fout.open ("outputs.txt");

	int numDays = 0;

	if ( inputFile == NULL )
	{
		cout << "Open file failed" << endl;
		return -1;
	}

	int iRC = iExtractTimeIntervalQtyPrice(inputFile, TimeIntervalQtyPrice, numDays);
	fclose(inputFile);

	if (iRC != 0)
	{
		cerr << "Fail in extract data from the input file" << endl;
		return -1;
	}
	cout << "num of days for qty calculation = " << numDays << endl;

	for (; mapIt != TimeIntervalQtyPrice.end(); mapIt++)
		mapIt->second.first = (long)(mapIt->second.first/numDays);

	for (unsigned int i = 0; i < sizeof(sTimePoints)/sizeof(sTimePoints[1])-1; i++)
	{
		cout << sTimePoints[i].first 
			 << setiosflags(ios::fixed) << setprecision(2)
			 << setw(15) << TimeIntervalQtyPrice[sTimePoints[i].first].first << "	"
			 << TimeIntervalQtyPrice[sTimePoints[i].first].second << endl;
		fout << sTimePoints[i].first 
			 << setiosflags(ios::fixed) << setprecision(2)
			 << setw(15) << TimeIntervalQtyPrice[sTimePoints[i].first].first << "	"
			 << TimeIntervalQtyPrice[sTimePoints[i].first].second << endl;
	}
	cout << endl;
	fout.close();

	auto end = time(NULL);
	cout << "Time taken to fetch all the data: " << (end - start) << " seconds" << endl;

	return 0;
}

int iExtractTimeIntervalQtyPrice(FILE *f, map< string, pair<long, double> >& QtyPriceMap, int &count)
{
	char buffer[BUF_SIZE];
	char date1[DATE_SIZE+1], date2[DATE_SIZE+1];
	char time[TIME_SIZE+1];
	double price = 0;
	int qty = 0;

	char *p = NULL, *p1 = NULL, *p2 = NULL, *p3 = NULL;
	memset(date1, '\0', DATE_SIZE+1);
    // date2 is not needed
	memset(date2, '\0', DATE_SIZE+1);
	memset(time, '\0', TIME_SIZE+1);
	while ( fgets(buffer, BUF_SIZE, f) )
	{
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
			*p3 = '\0';

			if ( string(time).compare(TIME_MKT_START) < 0 || string(time).compare(TIME_MKT_END) >= 0 )
				continue;

			if (string(date1).compare(TRADING_DATE) < 0)
				QtyPriceMap[sConvertToKey(time)].first += qty;
			else if (string(date1).compare(TRADING_DATE) == 0)
			{
				if ( (int)QtyPriceMap[sConvertToKey(time)].second == 0 )
					QtyPriceMap[sConvertToKey(time)].second = price;
			}
			else
				break;

            // data2 is not needed.
			if ( strncmp(date1, date2, DATE_SIZE) != 0 )
			{
				count++;
				strncpy(date2, date1, DATE_SIZE);
				date2[DATE_SIZE] = '\0';
			}
		}
	}

	if (QtyPriceMap.empty()) {
		cerr << "QtyPriceMap is empty" << endl;
		return -1;
	}

	// As count also includes the 5/21/2012 which is for trading price, not trading qty,
	// reduce the count by one day
	count--;

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
