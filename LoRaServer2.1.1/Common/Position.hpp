/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef POSITION_HPP
#define POSITION_HPP

#include "General.h"
#include "ValueWithValidity.hpp"

#include <string>

struct Position
{
	static const uint	significantDecimalPlacesLatLong = 5;
	static const uint	significantDecimalPlacesAltitude = 1;

	ValidValueDouble	latitude;	//in degrees.  North is +ve
	ValidValueDouble	longitude;	//in degrees.  East is +ve
	ValidValueDouble	altitude;	//in metres.  Up is +ve
	ValidValueDouble	toleranceHorizonal;	//1 std dev in metres
	ValidValueDouble	toleranceVertical;	//1 std dev in metres

	Position() {}

	bool Valid() const {return latitude.Valid() && longitude.Valid();}
	bool ToleranceValid() const {return toleranceHorizonal.Valid() && toleranceVertical.Valid();}
	double Tolerance() const {return toleranceHorizonal.Value() > toleranceVertical.Value() ? toleranceVertical.Value() : toleranceVertical.Value();}
	void Reset() {latitude.SetInvalid(); longitude.SetInvalid(); altitude.SetInvalid(); toleranceHorizonal.SetInvalid(); toleranceVertical.SetInvalid();}

	std::string Text(bool printAltitude, bool printTolerance) const;
	operator std::string() const {return Text(true, false);}
	static uint TextWidth(bool includingTolerance = false) {return 30 + (includingTolerance ? 8 : 0);}
};


inline int operator==(Position const& l, Position const& r)
{
	if (l.latitude != r.latitude) return 0;
	if (l.longitude != r.longitude) return 0;
	if (l.altitude != r.altitude) return 0;

	return 1;
}

inline int operator!=(Position const& l, Position const& r) {return !(l == r);}

#endif

