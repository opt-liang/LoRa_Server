/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef VALUE_WITH_VALIDITY_HPP
#define VALUE_WITH_VALIDITY_HPP
#include "General.h"

template <typename T> class ValueWithValidity
{
	//lint --e{1795} (Info -- defined but not instantiated)
private:
	bool		valid;
	T			value;	//NaN not used to show invalid because it is not universal

public:
	//lint --e{1401}  Warning -- member 'ValueWithValidity<float>::value' not initialized by constructor
	ValueWithValidity()
		:valid(false)
	{}

	ValueWithValidity(T const& myValue)
		:valid(true), value(myValue)
	{}

	ValueWithValidity(bool myValid, T const& myValue)
		:value(myValue), valid(myValid)
	{}

	void Set(T const& myValue, bool myValid = true) {valid = myValid; value = myValue;}
	void SetInvalid() {valid = false;}
	bool Valid() const {return valid;}
	T const& Value() const {return value;}
	T& Value() {return value;}

	//lint --e{1721}  Info -- operator =() for class 'ValueWithValidity<float>' is not assignment operator
	ValueWithValidity& operator=(T const& input) {valid = true; value = input; return *this;}
	ValueWithValidity& operator+=(T const& input) 	//+= sets value to valid
	{
		if (valid)
			value += input;
		else
		{
			value = input;
			valid = true;
		}
		return *this;
	}

	ValueWithValidity& operator+=(ValueWithValidity const& input) 
	{
		if (!input.Valid())
			return *this;

		if (Valid())
			value += input.Value();
		else
			value = input.Value();
		valid = true;

		return *this;
	}

	operator T const&() const {return Value();}
	operator T&() {return Value();}
};

template <typename T> int operator==(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r)
{
	if (!l.Valid() || !r.Valid())
		return 0;

	//lint --e{777}  Info -- sometimes testing floats for equality
	return l.Value() == r.Value();
}

template <typename T> int operator!=(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r)
{
	return !(l == r);
}

template <typename T> int operator<(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r)
{
	if (!l.Valid() || !r.Valid())
		return int(r.Valid());

	return l.Value() < r.Value();
}

template <typename T> int operator>(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r)
{
	if (!l.Valid() || !r.Valid())
		return int(l.Valid());

	return l.Value() > r.Value();
}

template <typename T> int operator>=(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r) {return !(l < r);}
template <typename T> int operator<=(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r) {return !(l > r);}


template <typename T> ValueWithValidity<T> operator+(ValueWithValidity<T> const& l, ValueWithValidity<T> const& r)
{
	ValueWithValidity<T> result;
	
	if (l.Valid() && r.Valid())
		result.Set(l.Value() + r.Value(), true);

	return result;
}

typedef ValueWithValidity<double>	ValidValueDouble;
typedef ValueWithValidity<float>	ValidValueFloat;
typedef ValueWithValidity<bool>		ValidValueBool;
typedef ValueWithValidity<uint64>	ValidValueUint64;
typedef ValueWithValidity<uint32>	ValidValueUint32;
typedef ValueWithValidity<uint16>	ValidValueUint16;
typedef ValueWithValidity<uint8>	ValidValueUint8;
typedef ValueWithValidity<sint16>	ValidValueSint16;

extern ValidValueDouble				invalidValueDouble;
extern ValidValueFloat				invalidValueFloat;
extern ValidValueBool				invalidValueBool;
extern ValidValueUint64				invalidValueUint64;
extern ValidValueUint32				invalidValueUint32;
extern ValidValueUint16				invalidValueUint16;
extern ValidValueSint16				invalidValueSint16;

#endif
