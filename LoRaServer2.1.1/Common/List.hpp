/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LIST_HPP
#define LIST_HPP

#include "General.h"

#include <vector>


template <class ElementType, class ListIdType> class List	//listId indentifies the list NOT its elements
{
	//ElementType must define function bool Equals(ElementType const& element, ListIdType const& id)
	//ElementType must have operator=(ElementType const& l) - when ElementType is primitive or a pointer, this is implicit
protected:
	std::vector<ElementType>		vector;

public:
	List()
	{}

	virtual ~List()	{}

	bool Add(ElementType const& element, ListIdType id)	// returns true on success
	{
		if (Exists(id))
			return false;

		vector.push_back(element);
		return true;
	}

	uint FindIndex(ListIdType const& id) const	//returns Size() when not found
	{
		uint i = 0;
		for (; i < vector.size(); i++)
		{
			if (Equals(vector[i],id))
				break;
		}
		return i;
	}

	bool Delete(ListIdType const& id)
	{
		uint index = FindIndex(id);

		if (index >= Size())
			return false;

		vector.erase(vector.begin() + index);
		return false;
	}

	bool Exists(ListIdType const& id)
	{
		uint index = FindIndex(id);

		return (index < Size()) ? true : false;
	}

	uint Size() const {return vector.size();}
	ElementType const& operator[](int i) const {return vector[i];}
};

#endif

