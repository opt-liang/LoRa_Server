/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CONSOLIDATED_POSITION_HPP
#define CONSOLIDATED_POSITION_HPP

#include "Position.hpp"
#include "General.h"
#include "ValueWithValidity.hpp"
#include <math.h>


class MeanPosition
{
private:
	Position			sum;	// the tolerances are the sum of square values.  If one input tolerance value is invalid the entire tolerance value is invalid
	uint16				values;
	mutable Position	mean;
	mutable bool		meanIsCorrect;

public:
	MeanPosition()
		:values(0), meanIsCorrect(false)
	{}

	void Input(Position const& input)
	{
		if (!input.Valid())
			return;

		meanIsCorrect = false;

		if (sum.latitude.Valid())
			sum.latitude += input.latitude;
		else
			sum.latitude = input.latitude;

		if (sum.longitude.Valid())
			sum.longitude += input.longitude;
		else
			sum.longitude = input.longitude;

		if (sum.altitude.Valid())
			sum.altitude += input.altitude;
		else
			sum.altitude = input.altitude;

		if (input.toleranceHorizonal.Valid() && (values == 0 || sum.toleranceHorizonal.Valid()))	//only valid if input is valid AND either no previous input have been received OR all previous inputs are valid
			sum.toleranceHorizonal += input.toleranceHorizonal * input.toleranceHorizonal;
		else
			sum.toleranceHorizonal.SetInvalid();

		if (input.toleranceVertical.Valid() && (values == 0 || sum.toleranceVertical.Valid()))		//only valid if input is valid AND either no previous input have been received OR all previous inputs are valid
			sum.toleranceVertical += input.toleranceVertical * input.toleranceVertical;
		else
			sum.toleranceVertical.SetInvalid();

		values++;
	}

	Position const&	Value() const
	{
		if (!meanIsCorrect)
			UpdateMean();
		return mean;
	}
	uint16 Values() const {return values;}
	void Reset() {sum.Reset(); mean.Reset(); values = 0; meanIsCorrect = false;}

private:
	void UpdateMean() const
	{
		if (!sum.Valid() || values == 0)	//values == 0 is just to avoid a compiler warning
			return;

		mean.latitude = sum.latitude / values;
		mean.longitude = sum.longitude / values;
		mean.altitude = sum.altitude / values;

		if (sum.toleranceHorizonal.Valid())
			mean.toleranceHorizonal = sqrt(sum.toleranceHorizonal/values);
		else
			mean.toleranceHorizonal.SetInvalid();

		if (sum.toleranceVertical.Valid())
			mean.toleranceVertical = sqrt(sum.toleranceVertical/values);
		else
			mean.toleranceVertical.SetInvalid();

		meanIsCorrect = true;
	}
};


class ConsolidatedPosition
{
private:
	uint16			maxEntries;
	uint16			minEntries; //min entries to be valid
	MeanPosition	input;
	Position		output;
	bool			fullAverageTaken;	// input has once held maxEntries

public:
	ConsolidatedPosition(uint16 myMaxEntries, uint16 myMinEntries = 1)
		: maxEntries(myMaxEntries), minEntries(myMinEntries), fullAverageTaken(false)
	{}

	bool Input(Position const& newValue)	//returns true when output changes
	{
		input.Input(newValue);

		if (input.Values() >= maxEntries)
		{
			//input is completely full reset it and copy to output
			output = input.Value();
			input.Reset();
			fullAverageTaken = true;
			return true;
		}

		if (fullAverageTaken)
			return false;

		//output has never been full - update position if input hold enough entries
		if (input.Values() >= minEntries)
		{
			output = input.Value();
			return true;
		}
		return false;
	}

	bool Valid() const {return output.Valid();}
	Position const& Value() const {return output;}
	operator Position const&() const {return Value();}
};


#endif

