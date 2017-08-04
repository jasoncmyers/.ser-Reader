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
//private:
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
public:
	union DataMemberUnion
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

	struct DataTag
	{
		__int16 tagTypeID;
		float time;
		double positionX;
		double positionY;
		__int16 weirdFinalTags;	// still have no idea what these do
	};
	
	struct DataSet1D
	{
		double calOffset;
		double calDelta;
		__int32 calElement;
		__int16 dataType;
		__int32 arrayLength;
		DataTag dataTag;
		std::vector<DataMemberUnion> data;
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
		DataTag dataTag;
		std::vector<DataMemberUnion> data;
	};

	// publically accessible class to wrap the various data types
	class DataSet
	{
	private:
		DataTag infoTag;
		double calOffsetX;
		double CallOffsetY;
		__int32 calElementX;
		double calOffsetY;
		double calDeltaY;
		__int32 calElementY;
		__int16 dataType;
		__int32 arraySizeX;
		__int32 arraySizeY;
	};

private:
	std::ifstream* serFile;
	std::ofstream* outFile;
	SerHeader header;
	std::vector<__int64> dataOffsets, tagOffsets;

	// ReadHeaders and ReadOffsetArrays need to be called in order and with a new file.  They do not seek.
	// Other commands will seek, but require the offset arrays to be populated first.
	ErrorCode ReadHeaders();
	ErrorCode ReadOffsetArrays();
	ErrorCode ReadDataSet1D(DataSet1D &dataSet, int setNum);
	//int ReadAllDataSets1D(std::vector<DataSet1D> &dataSets);
	ErrorCode ReadDataSet2D(DataSet2D &dataSet, int setNum);
	//int ReadAllDataSets2D(std::vector<DataSet2D> &dataSets);
	int ReadAllTags(std::vector<DataTag> &dataTags);
	ErrorCode ReadDataTag(DataTag &tag, int setNum);

	ErrorCode WriteHeaders();
	ErrorCode WriteOffsetArrays();
	int WriteAllTags(std::vector<DataTag> &dataTags);
	ErrorCode WriteDataTag(DataTag tag, int setNum);
	int WriteAllDataAndTags2D(DataSet2D* &dataSets, DataTag* &dataTags);
	int OverwriteData2D(DataSet2D &dataSet, int setNum);

public:
	// the Set___File methods also read/write the header and data/tag offset information
	bool SetReadFile(std::ifstream* file);
	bool SetWriteFile(std::ofstream* file);

	// accessor functions for the file header
	int GetDataFileVersion();
	int GetNumberElements();
	int GetNumberValidElements();
	int GetNumberDataDimensions();
	// all dimension headers are zero indexed
	int GetDimensionSize(int dimensionIndex);
	double GetDimensionCalibrationOffset(int dimensionIndex);
	double GetDimensionCalibrationDelta(int dimensionIndex);
	int GetDimensionCalibrationElement(int dimensionIndex);
	std::string GetDimensionDescription(int dimensionIndex);
	std::string GetDimensionUnits(int dimensionIndex);

	
};	// end class SerReader

#endif