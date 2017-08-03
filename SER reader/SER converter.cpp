// SER reader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "serReader.hpp"

using namespace std;

SerReader header;


int _tmain(int argc, char* argv[])
{
	fstream serFile;
	fstream outFile;

	SerReader reader;
	

	serFile.open("test.ser", ios::in | ios::binary);
	outFile.open("testout.ser", ios::out | ios::in | ios::binary);
	//serFile.open("test2.ser", ios::in | ios::binary);
	//serFile.open("test3.ser", ios::in | ios::binary);
	if(!outFile.is_open())
	{
		cout << "Could not open outfile\n";
	}
	
		
	if(serFile.is_open())
	{
		cout << "File opened\n\n";
		reader.SetFile(&serFile);

		SerReader::SerHeader header = reader.header;


	cout << "Header structure:" << endl;
	cout << "------------------------------------" << endl;
	cout << "Byte order: 0x" << hex << header.byteOrder << endl;
	cout << "SeriesID: 0x" << hex << header.seriesID << endl;
	cout << "Series version: 0x" << hex << header.version << endl;
	cout << "Data type ID: 0x" << hex << header.dataTypeID << endl;
	cout << "Tag type ID: 0x" << hex << header.tagTypeID << endl;
	cout << "Total # of elements: " << dec << header.totNumElem << endl;
	cout << "# of valid elements: " << dec << header.validNumElem << endl;
	cout << "Array absolute offset: 0x" << hex << header.arrayOffset << endl;
	cout << "Numer of dimensions of series: " << dec << header.numDimensions << endl << endl;

	if(header.numDimensions >= 1)
	{
		cout << "Dimension header follows: " << endl;
		cout << "------------------------------------" << endl;
		cout << "Dimension size : " << header.dimHeaders[0].dimSize << endl;
		cout << "Calibration offset: " << header.dimHeaders[0].calOffset << endl;
		cout << "Calibration delta: " << header.dimHeaders[0].calDelta << endl;
		cout << "Calibration element: " << header.dimHeaders[0].calElement << endl;
		cout << "Dimension description: " << header.dimHeaders[0].description << endl;
		cout << "Unit description: " << header.dimHeaders[0].unitName << endl << endl;
	}

	if(header.numDimensions >= 2)
	{
		cout << "Dimension header 2 follows: " << endl;
		cout << "------------------------------------" << endl;
		cout << "Dimension size : " << header.dimHeaders[1].dimSize << endl;
		cout << "Calibration offset: " << header.dimHeaders[1].calOffset << endl;
		cout << "Calibration delta: " << header.dimHeaders[1].calDelta << endl;
		cout << "Calibration element: " << header.dimHeaders[1].calElement << endl;
		cout << "Dimension description: " << header.dimHeaders[1].description << endl;
		cout << "Unit description: " << header.dimHeaders[1].unitName << endl;
	}

	serFile.close();
	outFile.close();
		
	}
	
	else
	{
		cout << "Error opening file!\n\n";
	}

	cout << "*** Complete***" << endl;
	cin.get();
	
	return 0;
}

