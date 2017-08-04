#include <iostream>
#include <fstream>
#include <stdexcept>
#include "serReader.hpp"

bool SerReader::SetReadFile(std::ifstream *file) {
	if(!file->is_open()) {
		serFile = NULL;
		return false;
	}

	serFile = file;
	ErrorCode errCode = ReadHeaders();
	if(errCode != 0)
		std::cout << "ReadHeaders() error: " << errCode << "\n";	// TODO: real error handling here.  Exceptions?

	errCode = ReadOffsetArrays();
	if(errCode != 0)
		std::cout << "ReadOffsetArrays() error: " << errCode << "\n";

	return true;
}

bool SerReader::SetWriteFile(std::ofstream *file) {
	if(!file->is_open()) {
		outFile = NULL;
		return false;
	}

	outFile = file;
	ErrorCode errCode = WriteHeaders();
	if(errCode != 0)
		std::cout << "WriteHeaders() error: " << errCode << "\n";	// TODO: real error handling here.  Exceptions?

	errCode = WriteOffsetArrays();
	if(errCode != 0)
		std::cout << "WriteOffsetArrays() error: " << errCode << "\n";
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
		BinOldOffsetHeader ooHead;
		serFile->read(reinterpret_cast<char*>(&ooHead), sizeof(ooHead));
		header.arrayOffset = ooHead.arrayOffset;
		header.numDimensions = ooHead.numDimensions;
	}
	else if (binHead.version == 0x220)
	{
		BinNewOffsetHeader noHead;
		serFile->read(reinterpret_cast<char*>(&noHead), sizeof(noHead));
		header.arrayOffset = noHead.arrayOffset;
		header.numDimensions = noHead.numDimensions;
	}
	else
	{
		return ERROR_UNKNOWN_FILE_VERSION;
	}

	header.dimHeaders.clear();
	header.dimHeaders.reserve(header.numDimensions);
	
	for(int i=0; i < header.numDimensions; i++)
	{
		SerReader::DimHeader tempDimHeader;
		int stringLength = 0;
		serFile->read(reinterpret_cast<char*>(&dimHead), sizeof(BinDimHeader));
		tempDimHeader.calDelta = dimHead.calDelta;
		tempDimHeader.calElement = dimHead.calElement;
		tempDimHeader.calOffset = dimHead.calOffset;
		tempDimHeader.dimSize = dimHead.dimSize;

		serFile->read(reinterpret_cast<char*>(&stringLength), 4);
		char *buffer = new char[stringLength+1];
		serFile->read(buffer, stringLength);
		buffer[stringLength] = '\0';
		tempDimHeader.description = buffer;
		delete[] buffer;
		
		serFile->read(reinterpret_cast<char*>(&stringLength), 4);
		buffer = new char[stringLength+1];
		serFile->read(buffer, stringLength);
		buffer[stringLength] = '\0';
		tempDimHeader.unitName = buffer;
		delete[] buffer;

		header.dimHeaders.push_back(tempDimHeader);
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
	dataOffsets.clear();
	tagOffsets.clear();
	dataOffsets.reserve(header.totNumElem);
	tagOffsets.reserve(header.totNumElem);

	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;
	
	int readSize = 0;
	if(header.version == 0x210)
		readSize = 4;
	else if(header.version == 0x220)
		readSize = 8;
	else
		return ERROR_UNKNOWN_FILE_VERSION;

	serFile->seekg((std::streamoff) header.arrayOffset);
	__int64 tempOffset;
	for(int i = 0; i < header.totNumElem; i++)
	{
		tempOffset = 0;
		serFile->read(reinterpret_cast<char*>(&tempOffset), readSize);
		dataOffsets.push_back(tempOffset);
	}
	for(int i = 0; i <header.totNumElem; i++)
	{
		tempOffset = 0;
		serFile->read(reinterpret_cast<char*>(&tempOffset), readSize);
		tagOffsets.push_back(tempOffset);
	}

	return ERROR_OK;
}


SerReader::ErrorCode SerReader::ReadDataSet2D(DataSet2D &dataSet, int setNum)
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

	// read in all the header info for the dataset
	serFile->seekg((std::streamoff)dataOffsets[setNum]);
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
	SerReader::DataMemberUnion tempData;

	// read data elements
	dataSet.data.clear();
	dataSet.data.reserve(dataPts);
	switch(dataSet.dataType)
	{
		// unsigned 8-bit integers
		case 1:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),1);
				tempData.uIntData = temp;
				dataSet.data.push_back(tempData);
			}
			break;
		// unsigned 16-bit integers
		case 2:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),2);
				tempData.uIntData = temp;
				dataSet.data.push_back(tempData);
			}
			break;
		// unsigned 32-bit integers
		case 3:
			for(int j = 0; j < dataPts; j++)
			{
				temp = 0;
				serFile->read(reinterpret_cast<char*>(&temp),4);
				tempData.uIntData = temp;
				dataSet.data.push_back(tempData);
			}
			break;
		// signed 8-bit integers
		case 4:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),1);
				tempData.sIntData = temp2;
				dataSet.data.push_back(tempData);
			}
			break;
		// signed 16-bit integers
		case 5:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),2);
				tempData.sIntData = temp2;
				dataSet.data.push_back(tempData);
			}
			break;
		// signed 32-bit integers
		case 6:
			for(int j = 0; j < dataPts; j++)
			{
				temp2 = 0;
				serFile->read(reinterpret_cast<char*>(&temp2),4);
				tempData.sIntData = temp2;
				dataSet.data.push_back(tempData);
			}
			break;
		// 32-bit float
		case 7:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				tempData.floatData = temp3;
				dataSet.data.push_back(tempData);
			}
			break;
		// 64-bit float
		case 8:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				tempData.floatData = temp3;
				dataSet.data.push_back(tempData);
			}
			break;
		// 64-bit complex (32-bit real and imaginary floats)
		case 9:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				tempData.complexData.realPart = temp3;
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),4);
				tempData.complexData.imagPart = temp3;
				dataSet.data.push_back(tempData);
			}
			break;
		// 128-bit complex (64-bit real and imaginary floats)
		case 10:
			for(int j = 0; j < dataPts; j++)
			{
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				tempData.complexData.realPart = temp3;
				temp3 = 0;
				serFile->read(reinterpret_cast<char*>(&temp3),8);
				tempData.complexData.imagPart = temp3;
				dataSet.data.push_back(tempData);
			}
			break;
	} // end switch(dataType) statement

	ReadDataTag(dataSet.dataTag, setNum);	
	
	return ERROR_OK;	
}

SerReader::ErrorCode SerReader::ReadDataSet1D(DataSet1D &dataSet, int setNum)
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
		

		// read in all the header info for the dataset
		serFile->seekg((std::streamoff)dataOffsets[setNum]);
		serFile->read(reinterpret_cast<char*>(&dataSet.calOffset), 8);
		serFile->read(reinterpret_cast<char*>(&dataSet.calDelta), 8);
		serFile->read(reinterpret_cast<char*>(&dataSet.calElement), 4);
		serFile->read(reinterpret_cast<char*>(&dataSet.dataType), 2);
		serFile->read(reinterpret_cast<char*>(&dataSet.arrayLength), 4);

		ReadDataTag(dataSet.dataTag, setNum);
		
		unsigned __int32 temp = 0;
		signed __int32 temp2 = 0;
		double temp3 = 0;

		// read in the individual data elements
		SerReader::DataMemberUnion tempData;
		dataSet.data.reserve(dataSet.arrayLength);
		switch(dataSet.dataType)
		{
			// see ReadDataSet2D for documenation of dataType values
			case 1:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),1);
					tempData.uIntData = temp;
					dataSet.data.push_back(tempData);
				}
				break;
			case 2:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),2);
					tempData.uIntData = temp;
					dataSet.data.push_back(tempData);
				}
				break;
			case 3:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp = 0;
					serFile->read(reinterpret_cast<char*>(&temp),4);
					tempData.uIntData = temp;
					dataSet.data.push_back(tempData);
				}
				break;
			case 4:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),1);
					tempData.sIntData = temp2;
					dataSet.data.push_back(tempData);
				}
				break;
			case 5:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),2);
					tempData.sIntData = temp2;
					dataSet.data.push_back(tempData);
				}
				break;
			case 6:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp2 = 0;
					serFile->read(reinterpret_cast<char*>(&temp2),4);
					tempData.sIntData = temp2;
					dataSet.data.push_back(tempData);
				}
				break;
			case 7:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					tempData.floatData = temp3;
					dataSet.data.push_back(tempData);
				}
				break;
			case 8:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					tempData.floatData = temp3;
					dataSet.data.push_back(tempData);
				}
				break;
			case 9:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					tempData.complexData.realPart = temp3;
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),4);
					tempData.complexData.imagPart = temp3;
					dataSet.data.push_back(tempData);
				}
				break;
			case 10:
				for(int j = 0; j < dataSet.arrayLength; j++)
				{
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					tempData.complexData.realPart = temp3;
					temp3 = 0;
					serFile->read(reinterpret_cast<char*>(&temp3),8);
					tempData.complexData.imagPart = temp3;
					dataSet.data.push_back(tempData);
				}
				break;
		}

		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}



SerReader::ErrorCode SerReader::ReadDataTag(DataTag &tag, int setNum) 
{
	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;
	
	__int16 tagTypeID = 0;
	float timeStamp = 0;
	double posX = 0;
	double posY = 0;
	__int16 finalTags = 0;

	serFile->seekg((std::streamoff)tagOffsets[setNum]);
	serFile->read(reinterpret_cast<char*>(&tagTypeID), 2);
	if(tagTypeID != header.tagTypeID)
		return ERROR_DATA_TYPE_MISMATCH;
	serFile->read(reinterpret_cast<char*>(&timeStamp), 4);
	if(header.tagTypeID == 0x4142)
	{
		serFile->read(reinterpret_cast<char*>(&posX), 8);
		serFile->read(reinterpret_cast<char*>(&posY), 8);
	}
	else if(header.tagTypeID != 0x4152)
	{
		return ERROR_UNKNOWN_FILE_VERSION;
	}
	// TODO: are the final tags there in version 0x4152?
	serFile->read(reinterpret_cast<char*>(&finalTags), 2);

	tag.tagTypeID = tagTypeID;
	tag.time = timeStamp;
	tag.positionX = posX;
	tag.positionY = posY;
	tag.weirdFinalTags = finalTags;

	return ERROR_OK;

}

SerReader::ErrorCode SerReader::WriteHeaders() 
{
	if(outFile->is_open())
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

		outFile->write(reinterpret_cast<char*>(&binHead), sizeof(BinHeader));
		// determine which size of header to use based on file version number

		//TODO: The length of the headers (including dimension headers) should be calculated, 
		// then the array offset address should be set to the byte directly after all headers
		if(binHead.version == 0x210) 
		{
			ooHead.arrayOffset = (__int32)header.arrayOffset;
			ooHead.numDimensions = header.numDimensions;
			outFile->write(reinterpret_cast<char*>(&ooHead), sizeof(ooHead));
		}
		else if (binHead.version == 0x220)
		{
			noHead.arrayOffset = header.arrayOffset;
			noHead.numDimensions = header.numDimensions;
			outFile->write(reinterpret_cast<char*>(&noHead), sizeof(noHead));
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
			outFile->write(reinterpret_cast<char*>(&dimHead), sizeof(BinDimHeader));

			int stringLength = header.dimHeaders[i].description.length();
			outFile->write(reinterpret_cast<char*>(&stringLength), 4);
			outFile->write(header.dimHeaders[i].description.c_str(), stringLength);

			stringLength =header.dimHeaders[i].unitName.length();
			outFile->write(reinterpret_cast<char*>(&stringLength), 4);
			outFile->write(header.dimHeaders[i].unitName.c_str(), stringLength);
		}
		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}

SerReader::ErrorCode SerReader::WriteOffsetArrays()
{
	if(outFile->is_open())
	{
		int addrSize = 0;
		if(header.version == 0x210)
			addrSize = 4;
		else if(header.version == 0x220)
			addrSize = 8;
		else
			return ERROR_UNKNOWN_FILE_VERSION;

		// Not sure if this is needed.  The file *should* be at the correct place, if the headers were properly written
		//file->seekp((streampos)header.arrayOffset);
		
		for(int i = 0; i < header.totNumElem; i++)
		{
			outFile->write(reinterpret_cast<char*>(&dataOffsets[i]), addrSize);	
		}
		for(int i = 0; i <header.totNumElem; i++)
		{
			outFile->write(reinterpret_cast<char*>(&tagOffsets[i]), addrSize);
		}

		return ERROR_OK;
	}
	else
		return ERROR_FILE_NOT_OPEN;
}


SerReader::ErrorCode SerReader::WriteDataTag(SerReader::DataTag tag, int setNum) 
{
	if(!outFile->is_open())
		return ERROR_FILE_NOT_OPEN;

	//std::cout << "Writing Tag #" << setNum << " at 0x" << std::hex << tagOffsets[setNum] << std::dec << "\n";
	
	outFile->seekp((std::streamoff)tagOffsets[setNum]);
	outFile->write(reinterpret_cast<char*>(&tag.tagTypeID), 2);
	outFile->write(reinterpret_cast<char*>(&tag.time), 4);
	if(header.tagTypeID == 0x4142)
	{
		outFile->write(reinterpret_cast<char*>(&tag.positionX), 8);
		outFile->write(reinterpret_cast<char*>(&tag.positionY), 8);
	}
	else if(header.tagTypeID != 0x4152)
	{
		return ERROR_UNKNOWN_FILE_VERSION;
	}
	// TODO: are the final tags there in version 0x4152?
	outFile->write(reinterpret_cast<char*>(&tag.weirdFinalTags), 2);

	return ERROR_OK;
}


SerReader::ErrorCode SerReader::OverwriteData2D(const DataSet2D &dataSet, int setNum)
{
	if(header.dataTypeID != 0x4122)
		return ERROR_DATA_TYPE_MISMATCH;

	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;

	outFile->seekp((std::streamoff)dataOffsets[setNum]);
	
	outFile->write(reinterpret_cast<const char*>(&dataSet.calOffsetX), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calDeltaX), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calElementX), 4);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calOffsetY), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calDeltaY), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calElementY), 4);
	outFile->write(reinterpret_cast<const char*>(&dataSet.dataType), 2);
	outFile->write(reinterpret_cast<const char*>(&dataSet.arraySizeX), 4);
	outFile->write(reinterpret_cast<const char*>(&dataSet.arraySizeY), 4);

	long dataPts = dataSet.arraySizeX * dataSet.arraySizeY;
		
	switch(dataSet.dataType)
	{
	
	case 1:
		for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),1);
			}
			break;
	case 2:
		for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),2);
			}
			break;
	case 3:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),4);
			}
			break;
	case 4:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),1);
			}
			break;
	case 5:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),2);
			}
			break;
	case 6:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),4);
			}
			break;
	case 7:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].floatData),4);
			}
			break;
	case 8:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].floatData),8);
			}	
			break;
	case 9:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.realPart),4);
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.imagPart),4);
			}
			break;
	case 10:
			for(int j = 0; j < dataPts; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.realPart),8);
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.imagPart),8);
			}
			break;
	}

	WriteDataTag(dataSet.dataTag, setNum);

	return ERROR_OK;
}

SerReader::ErrorCode SerReader::OverwriteData1D(const DataSet1D &dataSet, int setNum)
{
	if(header.dataTypeID != 0x4120)
		return ERROR_DATA_TYPE_MISMATCH;

	if(!serFile->is_open())
		return ERROR_FILE_NOT_OPEN;

	outFile->seekp((std::streamoff)dataOffsets[setNum]);
	
	outFile->write(reinterpret_cast<const char*>(&dataSet.calOffset), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calDelta), 8);
	outFile->write(reinterpret_cast<const char*>(&dataSet.calElement), 4);
	outFile->write(reinterpret_cast<const char*>(&dataSet.dataType), 2);
	outFile->write(reinterpret_cast<const char*>(&dataSet.arrayLength), 4);

	switch(dataSet.dataType)
	{
	
	case 1:
		for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),1);
			}
			break;
	case 2:
		for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),2);
			}
			break;
	case 3:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].uIntData),4);
			}
			break;
	case 4:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),1);
			}
			break;
	case 5:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),2);
			}
			break;
	case 6:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].sIntData),4);
			}
			break;
	case 7:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].floatData),4);
			}
			break;
	case 8:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].floatData),8);
			}	
			break;
	case 9:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.realPart),4);
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.imagPart),4);
			}
			break;
	case 10:
			for(int j = 0; j < dataSet.arrayLength; j++)
			{
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.realPart),8);
				outFile->write(reinterpret_cast<const char*>(&dataSet.data[j].complexData.imagPart),8);
			}
			break;
	}

	WriteDataTag(dataSet.dataTag, setNum);

	return ERROR_OK;
}


void SerReader::Get2DDataSet(SerReader::DataSet2D &dataSet, int setIndex)
{
	SerReader::ErrorCode errCode = ReadDataSet2D(dataSet, setIndex);
	if(errCode != 0) {
		throw std::runtime_error("Get2DDataSet failed with error = " + errCode);
	}
}

void SerReader::Get1DDataSet(SerReader::DataSet1D &dataSet, int setIndex)
{
	SerReader::ErrorCode errCode = ReadDataSet1D(dataSet, setIndex);
	if(errCode != 0) {
		throw std::runtime_error("Get1DDataSet failed with error = " + errCode);
	}
}
bool SerReader::Write2DDataSet(const SerReader::DataSet2D &dataSet, int setIndex)
{
	SerReader::ErrorCode errCode = OverwriteData2D(dataSet, setIndex);
	if(errCode != 0) {
		throw std::runtime_error("Write2DDataSet failed with error = " + errCode);
	}
}
bool SerReader::Write1DDataSet(const SerReader::DataSet1D &dataSet, int setIndex)
{
	SerReader::ErrorCode errCode = OverwriteData1D(dataSet, setIndex);
	if(errCode != 0) {
		throw std::runtime_error("Write1DDataSet failed with error = " + errCode);
	}
}




// Header accessors
int SerReader::GetDataFileVersion()
{ return header.version; }
int SerReader::GetNumberElements()
{ return header.totNumElem; }
int SerReader::GetNumberValidElements()
{ return header.validNumElem; }
int SerReader::GetNumberDataDimensions()
{ return header.numDimensions; }
int SerReader::GetDimensionSize(int dimensionIndex)
{ return header.dimHeaders[dimensionIndex].dimSize; }
double SerReader::GetDimensionCalibrationOffset(int dimensionIndex)
{ return header.dimHeaders[dimensionIndex].calOffset; }
double SerReader::GetDimensionCalibrationDelta(int dimensionIndex)
{ return header.dimHeaders[dimensionIndex].calDelta; }
int SerReader::GetDimensionCalibrationElement(int dimensionIndex)
{ return header.dimHeaders[dimensionIndex].calElement; }
std::string SerReader::GetDimensionDescription(int dimensionIndex)
{ return header.dimHeaders[dimensionIndex].description; }
std::string SerReader::GetDimensionUnits(int dimensionIndex) 
{ return header.dimHeaders[dimensionIndex].unitName; }

/* Wrapper class: possible future feature 
// The DataSet utility subclass:
// simple accessors
double SerReader::DataSet::CalibrationOffsetX()
{ return calOffsetX; }
double SerReader::DataSet::CalibrationDeltaX()
{ return calDeltaX; }
int SerReader::DataSet::CalibrationElementX()
{ return calElementX; }
double SerReader::DataSet::CalibrationOffsetY()
{ return calOffsetY; }
double SerReader::DataSet::CalibrationDeltaY()
{ return calDeltaY; }
int SerReader::DataSet::CalibrationElementY()
{ return calElementY; }
int SerReader::DataSet::ArraySizeX()
{ return arraySizeX; }
int SerReader::DataSet::ArraySizeY()
{ return arraySizeY; }
float SerReader::DataSet::TimeStamp()
{ return dataTag.time; }
double SerReader::DataSet::PositionX()
{ return dataTag.positionX; }
double SerReader::DataSet::PositionY()
{ return dataTag.positionY; }

SerReader::DataTypeEnum SerReader::DataSet::DataType() 
{ 
	switch (dataTag.tagTypeID) 
	{
	case 1:
	case 2:
	case 3:
		return SerReader::UNSIGNED_INT;
	case 4:
	case 5:
	case 6:
		return SerReader::SIGNED_INT;
	case 7:
	case 8:
		return SerReader::FLOAT;
	case 9:
	case 10:
		return SerReader::COMPLEX;
	default:
		return SerReader::INVALID_DATA_TYPE;
	}
}

// data retrieval methods
std::vector<unsigned int> SerReader::DataSet::GetUnsignedIntData()
{
	if(dataTag.tagTypeID < 1 || dataTag.tagTypeID > 3)
		throw std::logic_error("GetUnsignedIntData() called for non unsigned int data set");



}


std::vector<unsigned int> SerReader::DataSet::GetUnsignedIntDataAs2D()
{
	if(dataTag.tagTypeID < 1 || dataTag.tagTypeID > 3)
		throw std::logic_error("GetUnsignedIntData() called for non unsigned int data set");

}


std::vector<int> SerReader::DataSet::GetSignedIntData()
{
	if(dataTag.tagTypeID < 4 || dataTag.tagTypeID > 6)
		throw std::logic_error("GetSignedIntData() called for non signed int data set");

}


std::vector<int> SerReader::DataSet::GetSignedIntDataAs2D()
{
	if(dataTag.tagTypeID < 4 || dataTag.tagTypeID > 6)
		throw std::logic_error("GetSignedIntData() called for non signed int data set");

}


std::vector<double> SerReader::DataSet::GetFloatData()
{
	if(dataTag.tagTypeID < 7 || dataTag.tagTypeID > 8)
		throw std::logic_error("GetFloatData() called for non float data set");

}


std::vector<double> SerReader::DataSet::GetFloatDataAs2D()
{
	if(dataTag.tagTypeID < 7 || dataTag.tagTypeID > 8)
		throw std::logic_error("GetFloatData() called for non float data set");

}


std::vector<double> SerReader::DataSet::GetRealPartData()
{
	if(dataTag.tagTypeID < 9 || dataTag.tagTypeID > 10)
		throw std::logic_error("GetRealPartData() called for non complex data set");

}


std::vector<double> SerReader::DataSet::GetRealPartDataAs2D()
{
	if(dataTag.tagTypeID < 9 || dataTag.tagTypeID > 10)
		throw std::logic_error("GetRealPartData() called for non complex data set");

}


std::vector<double> SerReader::DataSet::GetImagPartData()
{
	if(dataTag.tagTypeID < 9 || dataTag.tagTypeID > 10)
		throw std::logic_error("GetImagPartData() called for non complex data set");

}


std::vector<double> SerReader::DataSet::GetImagPartDataAs2D()
{
	if(dataTag.tagTypeID < 9 || dataTag.tagTypeID > 10)
		throw std::logic_error("GetImagPartData() called for non complex data set");

}
*/