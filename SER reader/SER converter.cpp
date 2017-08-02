// SER reader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "serReader.hpp"

using namespace std;

SerHeader header;


int _tmain(int argc, char* argv[])
{
	fstream serFile;
	fstream outFile;
	

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
		ReadHeaders(&serFile, header);


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

	__int64* dataOffs = NULL;
	__int64* tagOffs = NULL;
	DataTag* tags = NULL;
	//D2DataSet dataSet;
	//D2DataSet* dataSets = NULL;
	D2DataSet dataSet;
	dataSet.data = NULL;
	D2DataSet* dataSets = NULL;
	
	int test;


	test = ReadOffsetArrays(&serFile, header, dataOffs, tagOffs);
	if(test != 0)
		cout << "Error with ReadOffsetArrays: " << test << endl;

	for(int i = 0; i < header.totNumElem; i++) {
		cout << "DataOffset " << dec << i << " = " << hex << dataOffs[i] << "; TagOffset " << dec << i << " = " << hex << tagOffs[i] << endl;
	}
	
	test = ReadAllTags(&serFile, header, tagOffs, tags);
	if(test != 0)
		cout << "Error with ReadAllTags: " << test << endl;

	test = Read2DDataSet(&serFile, header, dataOffs, dataSet, 0);
	if (test != 0)
		cout << "Error with Read2DDataSet: " << test << endl;

	/*
	cout <<"\n\nDataset values:\n";
	cout << "calOffset X: " << dec << dataSet.calOffsetX << endl;
	cout << "calDelta X: " << dec << dataSet.calDeltaX << endl;
	cout << "calElem X: " << dataSet.calElementX << endl;
	cout << "data type: " << dataSet.dataType << endl;
	cout << "Array size: (" << dataSet.arraySizeX << ", " << dataSet.arraySizeY << ")\n";
	*/
	
	/*
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			cout << "Pixel (" << i << ", "<< j<<") intensity = " << dec << (__int16)dataSet.data[i+j*dataSet.arraySizeX].sIntData << endl;
		}
	}
	*/

	
	/*
	cout << "\n\n* * * Beginning full data readout * * *\n";
	test = ReadAll2DDataSets(&serFile, header, dataOffs, dataSets);
	if (test != 0)
		cout << "Error with ReadAll2DDataSets: " << test << endl;
	cout << "\n\n* * * Full data readout complete * * *\n\n";
	

	for(int i = 0; i < header.totNumElem; i++)
	{
		cout << "Dataset " << dec << i << " has weird seperator tag 0x" << hex << tags[i].weirdFinalTags << endl;
	}
	*/

	

	for(int i = 0; i < header.validNumElem; i++)
	{
		Read2DDataSet(&serFile, header, dataOffs, dataSet, i);


	
	// Test wherein we actually modify the data to remove the clipped pixel values!!
	cout << "\n\n* * * Beginning modifying pixel intensities of dataset " << dec << i << " * * *\n";
	
		int numPoints = dataSet.arraySizeX * dataSet.arraySizeY;
		for(int j = 0; j < numPoints; j++)
		{
			int tempVal = (__int16)dataSet.data[j].sIntData;
			if(tempVal < -1000) 
			{
				//cout << "Member " << dec << j << " of dataset " << i << " is clipped.  Reseting intensity to max.\n";
				dataSet.data[j].sIntData = 25000;
			}
		}
	

		int eCode = Overwrite2DData(&outFile, header, dataOffs, dataSet, i);
		if(eCode == 0)
			cout << "Overwrite on dataset " << i << " complete\n";
		else
			cout << "Overwrite failed on dataset " << i << ": error code " << eCode << "\n";
	}


	// Test writing the data to a new file
	/*
	cout << "\n\n* * * Beginning output of test file * * *\n";
	outFile.open("testout.ser", ios::out | ios::binary);

	test = WriteHeaders(&outFile, header);
	if (test != 0)
		cout << "Error with WriteHeaders: " << test << endl;
	test = WriteOffsetArray(&outFile, header, dataOffs, tagOffs);
	if (test != 0)
		cout << "Error with WriteOffsetArray: " << test << endl;
	test = WriteAll2DDataAndTags(&outFile, header, dataSets, tags);
	if (test != 0)
		cout << "Error with WriteAll2DDataAndTags: " << test << endl;
	cout << "* * * Test file output complete * * *\n\n";
	*/

	serFile.close();
	outFile.close();



	delete[] header.dimHeaders;
	delete[] dataOffs;
	delete[] tagOffs;
	delete[] tags;
		
	}
	
	else
	{
		cout << "Error opening file!\n\n";
	}

	cout << "*** Complete***" << endl;
	cin.get();
	
	return 0;
}

