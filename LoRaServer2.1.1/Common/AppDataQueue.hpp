/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APP_DATA_QUEUE_HPP
#define APP_DATA_QUEUE_HPP

#include "General.h"
#include "LoRa.hpp"
#include <queue>

class AppDataQueue
{
private:
	std::queue<LoRa::FrameApplicationData> queue;

public:
	void Add(LoRa::FrameApplicationData input) {queue.push(input);}

	bool Empty() const {return queue.empty();}
	bool Waiting() const {return !Empty();}
	LoRa::FrameApplicationData const& HeadOfQueue() {return queue.front();}
	void RemoveHead() {queue.pop();}
};


#endif
