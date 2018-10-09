/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef DATA_RECORD_HPP
#define DATA_RECORD_HPP

#include "General.h"
#include <memory.h>

template <typename TSize, TSize N> class DataRecord
{
	//declared as a template to avoid doing a new and delete in the contructor
private:
	TSize	length;
	uint8	data[N];

protected:
	//lint -e{1401}  (Warning -- member 'DataRecord<unsigned short,16>::data' (line 10) not initialized by constructor)
	DataRecord(bool setLength) : length(setLength ? N : 0) {}
	virtual ~DataRecord() {}
	void SetData(uint8 const myData[], TSize myLength)
	{
		length = size_t(AcceptableLengthBytes(myLength));
		memcpy(DataNonConst(), myData, length);
	}
	
	void SetLength(TSize newLength) {length = TSize(AcceptableLengthBytes(newLength));}
	void IncreaseLength(TSize extra) {SetLength(AcceptableLengthBytes(Length() + extra));}

	uint16 AcceptableLengthBytes(unsigned l) const {return uint16((l > MaxLength()) ? MaxLength() : l);}

public:
	uint8 const* Data() const {return data;}
	uint8* DataNonConst() {return data;}
	TSize Length() const {return length;}

	void Initialise(uint8 newValue)
	{
		uint8* ptr = data;
		uint8* const end = ptr + length;

		for (; ptr < end; ptr++)
			*ptr = newValue;
	}

	uint8* FirstUnusedByte() {return data + length;}
	TSize MaxLength() const {return N;}

	uint8& operator[](TSize i) {return data[i];}
	uint8 const& operator[](TSize i) const {return data[i];}
};

template <typename TSize, TSize N> class DataRecordFL : public DataRecord<TSize, N>
{
	//lint --e{1942} (Note -- Unqualified name 'uint8' subject to misinterpretation owing to dependent base class)
public:
	DataRecordFL() :DataRecord<TSize, N>(true) {}
	DataRecordFL(uint8 const myData[]) : DataRecord<TSize, N>(true)
	{SetData(myData);}

	DataRecordFL(DataRecordFL<TSize, N> const& other) : DataRecord<TSize, N>(true) {*this = other;} 
	virtual ~DataRecordFL() {}

	void SetData(uint8 const myData[]) {DataRecord<TSize, N>::SetData(myData, DataRecord<TSize, N>::Length());}

	DataRecordFL& operator=(DataRecordFL<TSize,N> const& other) {SetData(other.Data()); return *this;}
	DataRecordFL& operator=(uint8 const newData[]) {SetData(newData); return *this;}
};

template <typename TSize, TSize N> class DataRecordVL : public DataRecord<TSize, N>
{
	//lint --e{1942} (Note -- Unqualified name 'uint8' subject to misinterpretation owing to dependent base class)
public:
	DataRecordVL() 
		: DataRecord<TSize, N>(false)
	{}

	DataRecordVL(uint8 const myData[], TSize myLength)
		: DataRecord<TSize, N>(false)
	{SetData(myData, myLength);}

	//lint -e{1738} (Info -- non copy constructor 'DataRecord<unsigned short,64>::DataRecord(bool)' used to initialize copy constructor)
	DataRecordVL(DataRecordVL const& other) 
		: DataRecord<TSize, N>(false)
	{SetData(other.Data(), other.Length());}

	virtual ~DataRecordVL() {}

	void SetData(uint8 const myData[], TSize myLength) {DataRecord<TSize, N>::SetData(myData, myLength);}
	void SetLength(TSize newLength) {DataRecord<TSize, N>::SetLength(newLength);}

	void AppendData(uint8 const myData[], TSize requestedIncrease) 
	{
		TSize oldLength = DataRecord<TSize, N>::Length();

		TSize increase;
		if (oldLength + requestedIncrease <= DataRecord<TSize, N>::MaxLength())
			increase = requestedIncrease;
		else
			increase = DataRecord<TSize, N>::MaxLength() - oldLength;

		memcpy(DataRecord<TSize, N>::FirstUnusedByte(), myData, increase);
		IncreaseLength(increase);
	}

	uint8* AppendData(DataRecord<TSize, N> const& other) {AppendData(other.Data(), other.Length()); return DataRecord<TSize, N>::FirstUnusedByte();}
	uint8* AppendByte(uint8 newByte) {AppendData(&newByte,1); return DataRecord<TSize, N>::FirstUnusedByte();}

	void IncreaseLength(TSize extra) {DataRecord<TSize, N>::IncreaseLength(extra);}
	void Clear() {DataRecord<TSize, N>::SetLength(0);}
};

#endif

