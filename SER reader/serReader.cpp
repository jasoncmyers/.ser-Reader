#include <iostream>
#include <fstream>
#include "serReader.hpp"

// TODO: get rid of this
using namespace std;

// TODO: get rid of this, just for test compilation
void main(void)
{

}

SerReader::SerReader() : serFile(NULL) {};

SerReader::~SerReader() {
	CloseFile();
}

void SerReader::CloseFile() {
	delete[] header.dimHeaders;
	delete[] dataOffsets;
	delete[] tagOffsets;
	serFile = NULL;
	dataOffsets = NULL;
	tagOffsets = NULL;
}


bool SerReader::SetFile(std::fstream *file) {
	CloseFile();
	if(!file->is_open()) {
		return false;
	}

	serFile = file;
	ErrorCode errCode = ReadHeaders();
	return true;
}


SerReader::ErrorCode SerReader::ReadHeaders() 
{
	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;

	
	BinHeader binHead;
	BinDimHeader dimHead;

	serFile->read(reinterpret_cast<char*>(&binHead), sizeof(BinHeader));
	// determine which size of header to use based on file version number
	if(binHead.version == 0x210) 
	{
		cout << "Using old version of header\n";
		BinOldOffsetHeader ooHead;
		serFile->read(reinterpret_cast<char*>(&ooHead), sizeof(ooHead));
		header.arrayOffset = ooHead.arrayOffset;
		header.numDimensions = ooHead.numDimensions;
	}
	else if (binHead.version == 0x220)
	{
		cout << "Using new version of header\n";
		BinNewOffsetHeader noHead;
		serFile->read(reinterpret_cast<char*>(&noHead), sizeof(noHead));
		header.arrayOffset = noHead.arrayOffset;
		header.numDimensions = noHead.numDimensions;
	}
	else
	{
		cout << "Error: unknown file version\n\n";
		return ERROR_UNKNOWN_FILE_VERSION;
	}

	header.dimHeaders = new DimHeader[header.numDimensions];
	for(int i=0; i < header.numDimensions; i++)
	{
		int stringLength = 0;
		serFile->read(reinterpret_cast<char*>(&dimHead), sizeof(BinDimHeader));
		header.dimHeaders[i].calDelta = dimHead.calDelta;
		header.dimHeaders[i].calElement = dimHead.calElement;
		header.dimHeaders[i].calOffset = dimHead.calOffset;
		header.dimHeaders[i].dimSize = dimHead.dimSize;

		serFile->read(reinterpret_cast<char*>(&stringLength), 4);
		char *buffer = new char[stringLength+1];
		serFile->read(buffer, stringLength);
		buffer[stringLength] = '\0';
		header.dimHeaders[i].description = buffer;
		delete[] buffer;
		
		serFile->read(reinterpret_cast<char*>(&stringLength), 4);
		buffer = new char[stringLength+1];
		serFile->read(buffer, stringLength);
		buffer[stringLength] = '\0';
		header.dimHeaders[i].unitName = buffer;
		delete[] buffer;
	}

	header.byteOrder = binHead.byteOrder;
	header.dataTypeID = binHead.dataTypeID;
	header.seriesID = binHead.seriesID;
	header.tagTypeID = binHead.tagTypeID;
	header.totNumElem = binHead.totNumElem;
	header.validNumElem = binHead.validNumElem;
	header.version = binHead.version;

	return ERROR_OK;
	
	
}

SerReader::ErrorCode SerReader::ReadOffsetArrays()
{
	// initialize the offset arrays
	delete[] dataOffsets;
	delete[] tagOffsets;
	dataOffsets = new __int64[header.totNumElem];
	tagOffsets = new __int64[header.totNumElem];
	for(int i = 0; i < header.totNumElem; i++)
	{
		dataOffsets[i] = 0;
		tagOffsets[i] = 0;
	}

	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;
	
	int readSize = 0;
	if(header.version == 0x210)
		readSize = 4;
	else if(header.version == 0x220)
		readSize = 8;
	else
		return ERROR_UNKNOWN_FILE_VERSION;

	serFile->seekg((streamoff)header.arrayOffset);
	
	for(int i = 0; i < header.totNumElem; i++)
	{
		serFile->read(reinterpret_cast<char*>(&dataOffsets[i]), readSize);	
	}
	for(int i = 0; i <header.totNumElem; i++)
	{
		serFile->read(reinterpret_cast<char*>(&tagOffsets[i]), readSize);
	}

	return ERROR_OK;
}

SerReader::ErrorCode SerReader::Read2DDataSet(DataSet2D &dataSet, int setNum)
{
	if(header.dataTypeID != 0x4122)
		return ERROR_DATA_TYPE_MISMATCH;
	if(setNum < 0 || setNum > (header.totNumElem-1))
		return ERROR_INDEX_OUT_OF_BOUNDS;

	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;
	
	//initialize data set
	dataSet.arraySizeX = 0;
	dataSet.arraySizeY = 0;
	dataSet.calDeltaX = 0;
	dataSet.calDeltaY = 0;
	dataSet.calElementX = 0;
	dataSet.calElementY = 0;
	dataSet.calOffsetX = 0;
	dataSet.calOffsetY = 0;
	dataSet.dataType = 0;
	delete[] dataSet.data;
	dataSet.data = NULL;


	// read in all the header info for the dataset
	serFile->seekg((streamoff)dataOffsets[setNum]);
	serFile->read(reinterpret_cast<char*>(&dataSet.calOffsetX), 8);
	serFile->read(reinterpret_cast<char*>(&dataSet.calDeltaX), 8);
	serFile->read(reinterpret_cast<char*>(&dataSet.calElementX), 4);
	serFile->read(reinterpret_cast<char*>(&dataSet.calOffsetY), 8);
	serFile->read(reinterpret_cast<char*>(&dataSet.calDeltaY), 8);
	serFile->read(reinterpret_cast<char*>(&dataSet.calElementY), 4);
	serFile->read(reinterpret_cast<char*>(&dataSet.dataType), 2);
	serFile->read(reinterpret_cast<char*>(&dataSet.arraySizeX), 4);
	serFile->read(reinterpret_cast<char*>(&dataSet.arraySizeY), 4);

	long dataPts = dataSet.arraySizeX * dataSet.arraySizeY;
	unsigned __int32 temp = 0;
	signed __int32 temp2 = 0;
	double temp3 = 0;

	dataSet.data = new DataMember[dataPts];
	switch(dataSet.dataType)
	{
	
		case 1:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),1);
				dataSet.data[j].uIntData = temp;
			}
			break;
		case 2:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),2);
				dataSet.data[j].uIntData = temp;
			}
			break;
		case 3:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),4);
				dataSet.data[j].uIntData = temp;
			}
			break;
		case 4:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),1);
				dataSet.data[j].sIntData = temp2;
			}
			break;
		case 5:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),2);
				dataSet.data[j].sIntData = temp2;
			}
			break;
		case 6:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),4);
				dataSet.data[j].sIntData = temp2;
			}
			break;
		case 7:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				dataSet.data[j].floatData = temp3;
			}
			break;
		case 8:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				dataSet.data[j].floatData = temp3;
			}
			break;
		case 9:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				dataSet.data[j].complexData.realPart = temp3;
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				dataSet.data[j].complexData.imagPart = temp3;
			}
			break;
		case 10:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				dataSet.data[j].complexData.realPart = temp3;
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				dataSet.data[j].complexData.imagPart = temp3;
			}
			break;
	} // end switch(dataType) statement
	
	return ERROR_OK;	
}

int SerReader::ReadAll2DDataSets(DataSet2D* &dataSets)
{
if(header.dataTypeID != 0x4122)
	return ERROR_DATA_TYPE_MISMATCH;

if(serFile->is_open())
{
	delete[] dataSets;
	dataSets = new DataSet2D[header.totNumElem];
	
	for(int i = 0; i < header.totNumElem; i++)
	{
		Read2DDataSet(dataSets[i], i);
	}

	return ERROR_OK;
}
	else
		return ERROR_FILE_NOT_OPEN;
}

SerReader::ErrorCode SerReader::Read1DDataSet(DataSet1D &dataSet, int setNum)
{
	if(header.dataTypeID != 0x4120)
		return ERROR_DATA_TYPE_MISMATCH;
	if(setNum < 0 || setNum > (header.totNumElem-1))
		return ERROR_INDEX_OUT_OF_BOUNDS;
	
	if(serFile->is_open())
	{
		//initialize data set
		dataSet.arrayLength = 0;
		dataSet.calDelta = 0;
		dataSet.calElement = 0;
		dataSet.calOffset = 0;
		dataSet.dataType = 0;
		dataSet.data = NULL;


		// read in all the header info for the dataset
		serFile->seekg((streamoff)dataOffsets[setNum]);
		serFile->read(reinterpret_cast<char*>(&dataSet.calOffset), 8);
		serFile->read(reinterpret_cast<char*>(&dataSet.calDelta), 8);
		serFile->read(reinterpret_cast<char*>(&dataSet.calElement), 4);
		serFile->read(reinterpret_cast<char*>(&dataSet.dataType), 2);
		serFile->read(reinterpret_cast<char*>(&dataSet.arrayLength), 4);

		unsigned __int32 temp = 0;
		signed __int32 temp2 = 0;
		double temp3 = 0;

		dataSet.data = new DataMember[dataSet.arrayLength];
		switch(dataSet.dataType)
		{
		
			case 1:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),1);
					dataSet.data[j].uIntData = temp;
				}
				break;
			case 2:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),2);
					dataSet.data[j].uIntData = temp;
				}
				break;
			case 3:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),4);
					dataSet.data[j].uIntData = temp;
				}
				break;
			case 4:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),1);
					dataSet.data[j].sIntData = temp2;
				}
				break;
			case 5:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),2);
					dataSet.data[j].sIntData = temp2;
				}
				break;
			case 6:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),4);
					dataSet.data[j].sIntData = temp2;
				}
				break;
			case 7:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					dataSet.data[j].floatData = temp3;
				}
				break;
			case 8:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					dataSet.data[j].floatData = temp3;
				}
				break;
			case 9:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					dataSet.data[j].complexData.realPart = temp3;
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					dataSet.data[j].complexData.imagPart = temp3;
				}
				break;
			case 10:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					dataSet.data[j].complexData.realPart = temp3;
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					dataSet.data[j].complexData.imagPart = temp3;
				}
				break;
		}

		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}

int SerReader::ReadAll1DDataSets(DataSet1D* &dataSets)
{
	if(header.dataTypeID != 0x4120)
		return ERROR_DATA_TYPE_MISMATCH;
	
	if(serFile->is_open())
	{
		delete[] dataSets;
		dataSets = new DataSet1D[header.totNumElem];
		
		for(int i = 0; i < header.totNumElem; i++)
		{
			Read1DDataSet(dataSets[i], i);
		}

		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}


int SerReader::ReadAllTags(DataTag* &dataTags)
{
	if(serFile->is_open())
	{
		delete[] dataTags;
		dataTags = new DataTag[header.totNumElem];

		__int16 tagTypeID = 0;
		float timeStamp = 0;
		double posX = 0;
		double posY = 0;

		for(int i = 0; i < header.totNumElem; i++)
		{
			serFile->seekg((streamoff)tagOffsets[i]);
			serFile->read(reinterpret_cast<char*>(&tagTypeID), 2);
			if(tagTypeID != header.tagTypeID)
				return ERROR_DATA_TYPE_MISMATCH;
			serFile->read(reinterpret_cast<char*>(&timeStamp), 4);
			if(header.tagTypeID == 0x4142)
			{
				serFile->read(reinterpret_cast<char*>(&posX), 8);
				serFile->read(reinterpret_cast<char*>(&posY), 8);
			}
			serFile->read(reinterpret_cast<char*>(&dataTags[i].weirdFinalTags), 2);

			dataTags[i].tagTypeID = tagTypeID;
			dataTags[i].time = timeStamp;
			dataTags[i].positionX = posX;
			dataTags[i].positionY = posY;
		}
		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}

int SerReader::WriteHeaders() 
{
	if(serFile->is_open())
	{
		BinHeader binHead;
		BinOldOffsetHeader ooHead;
		BinNewOffsetHeader noHead;
		BinDimHeader dimHead;

		binHead.byteOrder = header.byteOrder;
		binHead.dataTypeID = header.dataTypeID;
		binHead.seriesID = header.seriesID;
		binHead.tagTypeID = header.tagTypeID;
		binHead.totNumElem = header.totNumElem;
		binHead.validNumElem = header.validNumElem;
		binHead.version = header.version;

		serFile->write(reinterpret_cast<char*>(&binHead), sizeof(BinHeader));
		// determine which size of header to use based on file version number

		//TODO: The length of the headers (including dimension headers) should be calculated, 
		// then the array offset address should be set to the byte directly after all headers
		if(binHead.version == 0x210) 
		{
			ooHead.arrayOffset = (__int32)header.arrayOffset;
			ooHead.numDimensions = header.numDimensions;
			serFile->write(reinterpret_cast<char*>(&ooHead), sizeof(ooHead));
		}
		else if (binHead.version == 0x220)
		{
			noHead.arrayOffset = header.arrayOffset;
			noHead.numDimensions = header.numDimensions;
			serFile->write(reinterpret_cast<char*>(&noHead), sizeof(noHead));
		}
		else
		{
			return ERROR_UNKNOWN_FILE_VERSION;
		}

		for(int i=0; i < header.numDimensions; i++)
		{
			dimHead.calDelta = header.dimHeaders[i].calDelta;
			dimHead.calElement = header.dimHeaders[i].calElement;
			dimHead.calOffset = header.dimHeaders[i].calOffset;
			dimHead.dimSize = header.dimHeaders[i].dimSize;
			serFile->write(reinterpret_cast<char*>(&dimHead), sizeof(BinDimHeader));

			int stringLength = header.dimHeaders[i].description.length();
			serFile->write(reinterpret_cast<char*>(&stringLength), 4);
			serFile->write(header.dimHeaders[i].description.c_str(), stringLength);

			stringLength =header.dimHeaders[i].unitName.length();
			serFile->write(reinterpret_cast<char*>(&stringLength), 4);
			serFile->write(header.dimHeaders[i].unitName.c_str(), stringLength);
		}
		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}

int SerReader::WriteOffsetArray()
{
	if(serFile->is_open())
	{
		int addrSize = 0;
		if(header.version == 0x210)
			addrSize = 4;
		else if(header.version == 0x220)
			addrSize = 8;
		else
			return ERROR_UNKNOWN_FILE_VERSION;

		// Not sure if this is needed.  The file *should* be at the correct place, if the headers were properly written
		//file->seekp((streamoff)header.arrayOffset);
		
		for(int i = 0; i < header.totNumElem; i++)
		{
			serFile->write(reinterpret_cast<char*>(&dataOffsets[i]), addrSize);	
		}
		for(int i = 0; i <header.totNumElem; i++)
		{
			serFile->write(reinterpret_cast<char*>(&tagOffsets[i]), addrSize);
		}

		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}

int SerReader::WriteAll2DDataAndTags(DataSet2D* &dataSet, DataTag* &dataTags)
{
	if(header.dataTypeID != 0x4122)
		return ERROR_DATA_TYPE_MISMATCH;
	
	if(serFile->is_open())
	{
		for(int i = 0; i < header.totNumElem; i++)
		{
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calOffsetX), 8);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calDeltaX), 8);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calElementX), 4);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calOffsetY), 8);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calDeltaY), 8);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].calElementY), 4);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].dataType), 2);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].arraySizeX), 4);
			serFile->write(reinterpret_cast<char*>(&dataSet[i].arraySizeY), 4);

			long dataPts = dataSet[i].arraySizeX * dataSet[i].arraySizeY;
			
			switch(dataSet[i].dataType)
			{
		
				case 1:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].uIntData),1);
					}
					break;
				case 2:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].uIntData),2);
					}
					break;
				case 3:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].uIntData),4);
					}
					break;
				case 4:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].sIntData),1);
					}
					break;
				case 5:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].sIntData),2);
					}
					break;
				case 6:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].sIntData),4);
					}
					break;
				case 7:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].floatData),4);
					}
					break;
				case 8:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].floatData),8);
					}	
					break;
				case 9:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].complexData.realPart),4);
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].complexData.imagPart),4);
					}
					break;
				case 10:
					for(int j = 0; j < dataPts; j++)
					{
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].complexData.realPart),8);
						serFile->write(reinterpret_cast<char*>(&dataSet[i].data[j].complexData.imagPart),8);
					}
					break;
			}
			
			// Write tag data immediately after the main data set
			serFile->write(reinterpret_cast<char*>(&dataTags[i].tagTypeID), 2);
			serFile->write(reinterpret_cast<char*>(&dataTags[i].time), 4);
			if(header.tagTypeID == 0x4142)
			{
				serFile->write(reinterpret_cast<char*>(&dataTags[i].positionX), 8);
				serFile->write(reinterpret_cast<char*>(&dataTags[i].positionY), 8);
			}

			// Insert the spacer found between each Tag and the start of the next data point
			// It starts at A7 3E, then the value decements (but not necessarily by one) after each row length
			// not really sure how it works yet, but just filling in A7 3E seems to work ok.
			//__int16 temp = 0x3EA7;
			// try something new
			//__int16 temp = 0x3EA7 - (int)(i/15);
			//cout << "Temp spacer = 0x" << hex << temp << dec << endl;
			

			serFile->write(reinterpret_cast<char*>(&dataTags[i].weirdFinalTags), 2);
		}  // end of for loop
	return ERROR_OK;
	}
	else	// if file is not open
		return ERROR_FILE_NOT_OPEN;
}

int SerReader::Overwrite2DData(DataSet2D &dataSet, int setNum)
{
	if(header.dataTypeID != 0x4122)
		return ERROR_DATA_TYPE_MISMATCH;
	
	if(serFile->is_open())
	{
		serFile->seekp((streamoff)dataOffsets[setNum]);
		
		serFile->write(reinterpret_cast<char*>(&dataSet.calOffsetX), 8);
		serFile->write(reinterpret_cast<char*>(&dataSet.calDeltaX), 8);
		serFile->write(reinterpret_cast<char*>(&dataSet.calElementX), 4);
		serFile->write(reinterpret_cast<char*>(&dataSet.calOffsetY), 8);
		serFile->write(reinterpret_cast<char*>(&dataSet.calDeltaY), 8);
		serFile->write(reinterpret_cast<char*>(&dataSet.calElementY), 4);
		serFile->write(reinterpret_cast<char*>(&dataSet.dataType), 2);
		serFile->write(reinterpret_cast<char*>(&dataSet.arraySizeX), 4);
		serFile->write(reinterpret_cast<char*>(&dataSet.arraySizeY), 4);

		long dataPts = dataSet.arraySizeX * dataSet.arraySizeY;
			
		switch(dataSet.dataType)
		{
		
		case 1:
			for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].uIntData),1);
				}
				break;
		case 2:
			for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].uIntData),2);
				}
				break;
		case 3:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].uIntData),4);
				}
				break;
		case 4:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].sIntData),1);
				}
				break;
		case 5:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].sIntData),2);
				}
				break;
		case 6:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].sIntData),4);
				}
				break;
		case 7:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].floatData),4);
				}
				break;
		case 8:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].floatData),8);
				}	
				break;
		case 9:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].complexData.realPart),4);
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].complexData.imagPart),4);
				}
				break;
		case 10:
				for(int j = 0; j < dataPts; j++)
				{
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].complexData.realPart),8);
					serFile->write(reinterpret_cast<char*>(&dataSet.data[j].complexData.imagPart),8);
				}
				break;
		}	// end switch statement
	return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}