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
	outFile.open("testout.ser", ios::out | ios::binary);
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
		reader.ReadHeaders();

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
		cin.sync();
		cin.ignore();

	
		int test = reader.ReadOffsetArrays();
		if(test != 0)
			cout << "Error with ReadOffsetArrays: " << test << endl;

		for(int i = 0; i < header.totNumElem; i++) {
			cout << "DataOffset " << dec << i << " = " << hex << reader.dataOffsets[i] << "; TagOffset " << dec << i << " = " << hex << reader.tagOffsets[i] << endl;
		}
		
		cin.sync();
		cin.ignore();


		std::vector<SerReader::DataTag> dataTags;
		test = reader.ReadAllTags(dataTags);
		if(test != 0)
			cout << "Error with ReadAllTags: " << test << endl;

		for(int i =0; i < dataTags.size(); i++) {
			cout << "DataTag " << dec << i << ": \n";
			cout << "tagTypeID = 0x" << hex << dataTags[i].tagTypeID << dec << "\n";
			cout << "time = " << dataTags[i].time << "\n";
			cout << "position = (" << dataTags[i].positionX << ", " << dataTags[i].positionY << ")\n";
			cout << "weird end flags = " << dataTags[i].weirdFinalTags << "\n";
		}

		SerReader::DataSet2D dataSet;
		test = reader.ReadDataSet2D(dataSet, 0);
		if (test != 0)
			cout << "Error with Read2DDataSet: " << test << endl;
		
		
		serFile.close();
		outFile.close();
		
	}
	
	else
	{
		cout << "Error opening file!\n\n";
	}

	serFile.close();
	outFile.close();

	cout << "*** Complete***" << endl;
	cin.sync();
	cin.ignore();
	
	return 0;
}

