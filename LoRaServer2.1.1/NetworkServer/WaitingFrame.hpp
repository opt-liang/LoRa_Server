/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef WAITING_FRAME_HPP
#define WAITING_FRAME_HPP

#include "General.h"
#include "LoRa.hpp"

#include <list>

class Mote;
class WaitingFrame : public LoRa::Frame
{
private:
	struct TokenRecord
	{
		bool		app;	//true represents app data, false represents mac commands
		uint16		token;

		TokenRecord(bool myApp, uint16 myToken)
			:app(myApp), token(myToken)
		{}
	};

	class TokenList
	{
	private:
		std::list<TokenRecord>		list;
	public:
		bool IsEmpty() const {return list.empty();}

		void Add(bool app, uint16 token)
		{
			TokenRecord record(app, token);

			list.push_back(record);
		}

		void Clear()
		{
			while (!IsEmpty())
				GetNext();
		}

		TokenRecord GetNext() {TokenRecord result = list.front(); list.pop_front(); return result;}

		uint16 Size() const {return list.size();}
		TokenRecord const& Front() const {return list.front();}
	};


	static const uint8		defaultTransmissions = 5;

	Mote&					mote;
	TokenList				tokenList;
	bool					isJoinAcceptFrame;
	uint8					transmissionsRemaining;
	bool					first;

public:
	WaitingFrame(Mote& myMote) : mote(myMote), isJoinAcceptFrame(false), transmissionsRemaining(0), first(true)
	{}

	void Set(LoRa::Frame const& other, bool myIsJoinAcceptFrame, uint8 transmissions = 0)
	{
		ResetTransmissionCounter(transmissions);
		SetData(other.Data(), other.Length());
		SetToJoinAcceptFrame(myIsJoinAcceptFrame);
	}

	void SetToJoinAcceptFrame(bool myIsJoinAcceptFrame) {isJoinAcceptFrame = myIsJoinAcceptFrame;}

	void ResetTransmissionCounter(uint8 transmissions = 0)
	{
		if (transmissions == 0)
			transmissions = defaultTransmissions;
		transmissionsRemaining = transmissions;
		first = true;
	}

	void Transmitted() 	// called when transmitted
	{
		if (transmissionsRemaining > 0)
			transmissionsRemaining--;

		if (first)
		{
			ReportTransmission();
			first = false;
		}

		bool timeout = transmissionsRemaining == 0;
		bool waitingForAck = !timeout && IsDataConfirmedFrame();
		if (!waitingForAck)
			Clear();
	}

	bool Active() const {return Length() > 0;}
	bool First() const{return first;}
	bool IsJoinAcceptFrame() const {return isJoinAcceptFrame;}
	void AckReceived() {ReportAcknowledge(); Clear();}

	void AddToken(bool app, uint16 token) {tokenList.Add(app, token);}
	TokenList& GetTokenList() {return tokenList;}

private:
	void Clear()
	{
		transmissionsRemaining = 0;
		first = false;
		tokenList.Clear();
		LoRa::Frame::Clear();
	}

	void ReportTransmission();	//if token is valid
	void ReportAcknowledge();
};


#endif
