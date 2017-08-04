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
	ifstream serFile;
	ofstream outFile;

	SerReader reader;

	serFile.open("test.ser", ios::binary);
	outFile.open("testout.ser", ios::binary);
	//serFile.open("test2.ser", ios::in | ios::binary);
	//serFile.open("test3.ser", ios::in | ios::binary);
	if(!outFile.is_open())
	{
		cout << "Could not open outfile\n";
		
	}
	
		
	if(serFile.is_open() && outFile.is_open())
	{
		cout << "File opened\n\n";
		reader.SetReadFile(&serFile);
		reader.SetWriteFile(&outFile);

		cout << "Header info:" << endl;
		cout << "------------------------------------" << endl;
		cout << "Series version: 0x" << hex << reader.GetDataFileVersion() << dec << "\n";
		cout << "Numer of dimensions: " << dec << reader.GetNumberDataDimensions() << "\n";
		cout << "Total # of elements: " << reader.GetNumberElements() << "\n";
		cout << "# of valid elements: " << dec << reader.GetNumberValidElements() << "\n\n";
		
		cin.sync();
		cin.ignore();

	
		SerReader::DataSet2D dataSet;
		reader.Get2DDataSet(dataSet, 0);				
		
		cout <<"\n\nSample values for dataset 0:\n";
		cout << "calOffset X: " << dec << dataSet.calOffsetX << "\n";
		cout << "calDelta X: " << dec << dataSet.calDeltaX << "\n";
		cout << "calElem X: " << dataSet.calElementX << "\n";
		cout << "data type: " << dataSet.dataType << "\n";
		cout << "Array size: (" << dataSet.arraySizeX << ", " << dataSet.arraySizeY << ")\n";
		cout << "DataTag - tagTypeID = 0x" << hex << dataSet.dataTag.tagTypeID << dec << "\n";
		cout << "DataTag - time = " << dataSet.dataTag.time << "\n";
		cout << "DataTag - position = (" << dataSet.dataTag.positionX << ", " << dataSet.dataTag.positionY << ")\n";
		cout << "DataTag - undocumented final = " << dataSet.dataTag.weirdFinalTags << "\n\n";		
				
		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				cout << "Pixel (" << i << ", "<< j<<") intensity = " << dec << (__int16)dataSet.data[i+j*dataSet.arraySizeX].sIntData << "\n";
			}
		}
		cout << endl;

		cin.sync();
		cin.ignore();

		cout << "\n\n* * * Beginning modifying pixel intensities of dataset * * *\n";
		for(int i = 0; i < reader.GetNumberValidElements(); i++)
		{
			reader.Get2DDataSet(dataSet, i);
		
			// Test wherein we actually modify the data to remove the clipped pixel values
			int numPoints = dataSet.arraySizeX * dataSet.arraySizeY;
			int numClippedPixels = 0;
			for(int j = 0; j < numPoints; j++)
			{
				int tempVal = (__int16)dataSet.data[j].sIntData;
				if(tempVal < -1000) 
				{
					dataSet.data[j].sIntData = 25000;
					numClippedPixels++;
				}
			}
			if(numClippedPixels > 0) {
				cout << "DataSet " << i << " had " << numClippedPixels << " pixels reset to the max value.\n";
				reader.Write2DDataSet(dataSet, i);
			}				
		}

		serFile.close();
		outFile.close();

	}
	
	else
	{
		cout << "Error opening file!\n\n";
	}

	

	cout << "*** Complete***" << endl;
	cin.sync();
	cin.ignore();
	
	return 0;
}

