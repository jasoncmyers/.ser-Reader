#ifndef SER_FILES_H
#define SER_FILES_H

#include <iostream>
#include <fstream>

using namespace std;

#pragma pack(push, 2)

// binary header structures
struct BinHeader
{
	__int16 byteOrder;
	__int16 seriesID;
	__int16 version;
	__int32 dataTypeID;
	__int32 tagTypeID;
	__int32 totNumElem;
	__int32 validNumElem;
};

struct BinOldOffsetHeader
{
	__int32 arrayOffset;
	__int32 numDimensions;
};

struct BinNewOffsetHeader
{
	__int64 arrayOffset;
	__int32 numDimensions;
};

struct BinDimHeader
{
	__int32 dimSize;
	double calOffset;
	double calDelta;
	__int32 calElement;
};
#pragma pack(pop)

// Logical headers for storing information
struct DimHeader
{
	__int32 dimSize;
	double calOffset;
	double calDelta;
	__int32 calElement;
	char* description;
	char* unitName;
};

struct SerHeader
{
	__int16 byteOrder;
	__int16 seriesID;
	__int16 version;
	__int32 dataTypeID;
	__int32 tagTypeID;
	__int32 totNumElem;
	__int32 validNumElem;

	__int64 arrayOffset;
	__int32 numDimensions;

	DimHeader*	dimHeaders;
};

union DataMember
{
	unsigned __int32 uIntData;
	signed __int32 sIntData;
	double floatData;
	struct ComplexValue16byte
	{
		double realPart;
		double imagPart;
	} complexData;
};

struct D1DataSet
{
	double calOffset;
	double calDelta;
	__int32 calElement;
	__int16 dataType;
	__int32 arrayLength;
	DataMember* data;
};

struct D2DataSet
{
	double calOffsetX;
	double calDeltaX;
	__int32 calElementX;
	double calOffsetY;
	double calDeltaY;
	__int32 calElementY;
	__int16 dataType;
	__int32 arraySizeX;
	__int32 arraySizeY;
	DataMember* data;
	

};

struct DataTag
{
	__int16 tagTypeID;
	float time;
	double positionX;
	double positionY;
	__int16 weirdFinalTags;
};


class SerFile 
{
	SerHeader header;
};



int ReadHeaders(fstream* file, SerHeader &header);
int ReadOffsetArrays(fstream* file, const SerHeader &header, __int64* &dataOffsets, __int64* &tagOffsets);
int Read1DDataSet(fstream* file, const SerHeader &header, const __int64* dataOffsets, D1DataSet &dataSet, int setNum);
int ReadAll1DDataSets(fstream* file, const SerHeader &header, const __int64* dataOffsets, D1DataSet* &dataSets);
int Read2DDataSet(fstream* file, const SerHeader &header, const __int64* dataOffsets, D2DataSet &dataSet, int setNum);
int ReadAll2DDataSets(fstream* file, const SerHeader &header, const __int64* dataOffsets, D2DataSet* &dataSets);
int ReadAllTags(fstream* file, const SerHeader &header, const __int64* tagOffsets, DataTag* &dataTags);

int WriteHeaders(fstream* file, SerHeader &header);
int WriteOffsetArray(fstream* file, const SerHeader &header, __int64* &dataOffsets, __int64* &tagOffsets);
int WriteAll2DDataAndTags(fstream* file, const SerHeader &header, D2DataSet* &dataSets, DataTag* &dataTags);
int Overwrite2DData(fstream* file, const SerHeader &header, const __int64* dataOffsets, D2DataSet &dataSet, int setNum);

#endif