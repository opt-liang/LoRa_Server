/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "IpSocketSet.hpp"

IP::Socket* IP::SocketSet::FindFailedSocket() const	//tests all members and removes those that are not valid
{
	MutexHolder holder(mutex);

	for (int i = 0; i < int(list.size()); i++)
	{
		Socket* socket = list[i];

		if (socket && !socket->Validate()) //if false - the socket will remove itself from the list
			return socket;
	}
	return 0;
}

