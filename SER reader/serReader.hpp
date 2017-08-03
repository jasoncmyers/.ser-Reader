#ifndef SER_FILE_READER_H
#define SER_FILE_READER_H

#include <iostream>
#include <fstream>
#include <vector>

class SerReader {

private:
	enum ErrorCode
{
	ERROR_OK = 0,
	ERROR_FILE_NOT_OPEN = 1,
	ERROR_UNKNOWN_FILE_VERSION = 2,
	ERROR_DATA_TYPE_MISMATCH = 3,
	ERROR_INDEX_OUT_OF_BOUNDS = 4
};
	
	// binary header structures (need to interact with binary data, so fix the packing at the 16 bit level)
	#pragma pack(push, 2)
// TODO: remove this, just here for testing
public:
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
		std::string description;
		std::string unitName;
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

		std::vector<DimHeader>	dimHeaders;
	};

	// Data members can belong to 10 different types, including different sizes of (un)signed int, float, and complex float values
	// This union is memory inefficient but simplifies the data handling code considerably
	// TODO: when exporting to vectors, this should be handled by type.
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

	struct DataSet1D
	{
		double calOffset;
		double calDelta;
		__int32 calElement;
		__int16 dataType;
		__int32 arrayLength;
		std::vector<DataMember> data;
	};

	struct DataSet2D
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
		std::vector<DataMember> data;
	};

	struct DataTag
	{
		__int16 tagTypeID;
		float time;
		double positionX;
		double positionY;
		__int16 weirdFinalTags;	// still have no idea what these do
	};

	std::ifstream* serFile;
	std::ofstream* outFile;
	SerHeader header;
	std::vector<__int64> dataOffsets, tagOffsets;

	ErrorCode ReadHeaders();
	ErrorCode ReadOffsetArrays();
	ErrorCode ReadDataSet1D(DataSet1D &dataSet, int setNum);
	int ReadAllDataSets1D(std::vector<DataSet1D> &dataSets);
	ErrorCode ReadDataSet2D(DataSet2D &dataSet, int setNum);
	int ReadAllDataSets2D(std::vector<DataSet2D> &dataSets);
	int ReadAllTags(std::vector<DataTag> &dataTags);

	int WriteHeaders();
	int WriteOffsetArray();
	int WriteAllTags(std::vector<DataTag> &dataTags);
	int WriteAllDataAndTags2D(DataSet2D* &dataSets, DataTag* &dataTags);
	int OverwriteData2D(DataSet2D &dataSet, int setNum);

public:
	bool SetReadFile(std::ifstream* file);
	bool SetWriteFile(std::ofstream* file);
	std::vector<std::vector<int>> GetImage();
	
};	// end class SerReader

#endif