/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef UDP_SOCKET_SET_HPP
#define UDP_SOCKET_SET_HPP

#include "IpSocket.hpp"
#include <vector>
#include "Mutex.hpp"

//lint -e{762} (Info -- Redundantly declared symbol 'Snooze(unsigned int))
void Snooze(uint snoozeTime_ms);	// thread suspends for snoozeTime_ms ms - must be defined by user
namespace IP
{
	class SocketSet
	{
		friend class Socket;
	private:
		static const unsigned		maxWaitToAddNewSocket_us = 250 * 1000;	//new socket will not be added to list of sockets on which select() is performed until select returns
		fd_set						fileDescriptors;
		uint						maxFd;	//only used in Berkley sockets
		bool						roundRobin;
		std::vector<IP::Socket*>	list;
		uint						previousTestedSocket;
		mutable Mutex				mutex;
		timeval						timeOut;

	public:
		SocketSet(bool myRoundRobin)	//if myRoundRobin is false, a call to Wait() tests sockets in order in which they were added (early means high priority)
			: maxFd(0), roundRobin(myRoundRobin), previousTestedSocket(0)
		{
			timeOut.tv_usec = maxWaitToAddNewSocket_us;
			timeOut.tv_sec = 0;

			FD_ZERO(&fileDescriptors);
		}

		~SocketSet()  {}

		Socket* Wait()
		{
			for (;;)
			{
				mutex.Lock();	//cannot use MutexHolder because mutex must be released during select()
				timeval currentTimeOut = timeOut;
				fd_set currentFdSet = fileDescriptors;	//input to indicate sockets on which to wait, output to indicate sockets with activity
				bool listEmpty = list.empty();
				mutex.Unlock();

				if (listEmpty)
				{
					Snooze(timeOut.tv_usec / 1000);	//Snooze time is in units of ms
				}
				else
				{
					try
					{
						Socket* socket = ReceiveData(currentFdSet, currentTimeOut);

						if (socket)
							return socket;
					}
					catch (IP::Error const& e)
					{
						if (e.Number() == IP::Error::notASocket || e.Number() == IP::Error::badFile)
						{
							//in Linux the select function returns with an error when a disconnected socket is found - the socket must be removed
							//One socket descriptor is invalid (probably disconnected) so validate them all
							
							for(;;)
							{
								Socket* failedSocket = FindFailedSocket();

								if (!failedSocket)
									break;

								failedSocket->Close();	//removes socket from this set
							}
						}
						else
							throw;
					}
				}
			}
		}

		//lint -e{1764} (Info -- Reference parameter 'currentTimeOut' could be declared const ref) - not true in Linux
		Socket* ReceiveData(fd_set const& currentFdSet, timeval& currentTimeOut)
		{
			fd_set localFdSet = currentFdSet;
			int selectResult = select(maxFd+1, &localFdSet, 0,0, &currentTimeOut);

			if (selectResult == 0)
				return 0;	//timeout

			else if (selectResult == SOCKET_ERROR)
				throw IP::Error("Invalid return from select");

			// hold mutex until end of function
			MutexHolder holder(mutex);
			uint size = list.size();

			if (size == 0)
				return 0;

			uint start;
			uint end;
			if (roundRobin)
			{
				start = previousTestedSocket + 1;
				if (start >= size)
					start = 0;

				if (start > 0)
					end = start - 1;
				else
					end = size - 1;
			}
			else
			{
				start = 0;
				end = size - 1;
			}

			//lint -e{850} (Info -- for loop index variable 'i' whose type category is 'integral' is modified in body of the for loop that began at 'line 111')
			for (uint i = start;; i++)
			{
				if (i >= size)
					i = 0;

				if (FD_ISSET(list[i]->SocketDescriptor(), &localFdSet))
				{
					//found
					previousTestedSocket = i;
					return list[i];
				}

				if (i == end)
					return 0;
			}
		}

	private:
		bool Add(Socket& newSocket)
		{
			//set socket to non-blocking
			if (!newSocket.SetToBlock(false))
				return false;

			MutexHolder holder(mutex);

			//lint -e{717} do ... while(0)  in FD_SET
			FD_SET(newSocket.SocketDescriptor(), &fileDescriptors);

			if (maxFd < newSocket.SocketDescriptor())
				maxFd = newSocket.SocketDescriptor();

			list.push_back(&newSocket);
			return true;
		}

		void Remove(Socket const& socket)
		{
			MutexHolder holder(mutex);
			//lint -e{717} do ... while(0)  in FD_CLR
			FD_CLR(socket.SocketDescriptor(), &fileDescriptors);

			maxFd = 0;
			std::size_t i = 0;
			while (i < list.size())
			{
				//lint -e{1702} (Info -- operator 'operator+' is both an ordinary function and a member function
				if (list[i]->SocketDescriptor() == socket.SocketDescriptor())
				{
					list.erase(list.begin() + i);
					continue;
				}

				if (list[i]->SocketDescriptor() > maxFd)
					maxFd = list[i]->SocketDescriptor();

				i++;
			}
		}

		Socket* FindFailedSocket() const;		//do not call when holding mutex
	};
}

#endif
