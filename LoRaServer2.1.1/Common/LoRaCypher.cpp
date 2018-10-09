/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRa.hpp"
#include "aes.h"
#include "cmac.h"

namespace LoRa
{
	void BlockExOr(uint8 const l[], uint8 const r[], uint8 out[], uint16 bytes);	//out location may equal either l or r
}


void LoRa::GenerateDataFrameIntegrityCode(CypherKey const& key, uint8 const input[], uint16 dataLength, uint32 address, bool up, uint32 sequenceNumber, uint8 output[micBytes])
{
	/*
	Generate artificial B[0] block
	Encrypt B[0] to give X[1]

	for n = 1 to number of blocks 
		exclusive OR B[n] with X[n] to give Y[n]
		encrypt Yi using key to give X[n+1]
	*/
	uint8 b0[LoRa::authenticationBlockBytes];
	memset(b0, 0 , LoRa::authenticationBlockBytes);

	b0[ 0] = 0x49; //authentication flags

	b0[ 5] = up ? 0 : 1;
	Write4ByteValue(&b0[6], address);
	Write4ByteValue(&b0[10], sequenceNumber);

	b0[15] = uint8(dataLength);

	AES_CMAC_CTX cmacctx;
	AES_CMAC_Init(&cmacctx);
	AES_CMAC_SetKey(&cmacctx, key.Data());

	AES_CMAC_Update(&cmacctx, b0, LoRa::authenticationBlockBytes);
	AES_CMAC_Update(&cmacctx, input, dataLength);

	uint8 temp[LoRa::authenticationBlockBytes];
	AES_CMAC_Final(temp, &cmacctx);

	memcpy(output, temp, micBytes);
}


void LoRa::GenerateJoinFrameIntegrityCode(CypherKey const& key, uint8 const input[], uint16 dataLength, uint8 output[micBytes])
{
	AES_CMAC_CTX cmacctx;
	AES_CMAC_Init(&cmacctx);
	AES_CMAC_SetKey(&cmacctx, key.Data());

	AES_CMAC_Update(&cmacctx, input, dataLength);

	uint8 temp[LoRa::authenticationBlockBytes];
	AES_CMAC_Final(temp, &cmacctx);

	memcpy(output, temp, micBytes);
}


void LoRa::EncryptPayload(CypherKey const& key, uint8 const in[], uint16 inputDataLength, uint32 address, bool up, uint32 sequenceNumber, uint8 out[])
{
	if (inputDataLength == 0)
		return;

	uint8 A[LoRa::encryptionBlockBytes];
	uint16 const overHangBytes = inputDataLength & (LoRa::encryptionBlockBytes - 1);

	memset(A, 0, LoRa::encryptionBlockBytes);

	A[ 0] = 0x01; //encryption flags
	A[ 5] = up ? 0 : 1;

	Write4ByteValue(&A[6], address);
	Write4ByteValue(&A[10], sequenceNumber);

	unsigned blocks = CountBlocks(inputDataLength, LoRa::encryptionBlockBytes);

	uint8 const* blockInput = in;
	uint8* blockOutput = out;
	for (unsigned i = 1; i <= blocks; i++, blockInput += LoRa::encryptionBlockBytes, blockOutput += LoRa::encryptionBlockBytes)
	{
		A[15] = uint8(i);

		aes_context aesContext;
		aes_set_key(key.Data(), LoRa::cypherKeyBytes, &aesContext);

		uint8 S[LoRa::cypherKeyBytes];
		aes_encrypt(A, S, &aesContext);

		uint16 bytesToExOr;
		if ((i < blocks) || (overHangBytes == 0))
			bytesToExOr = LoRa::cypherKeyBytes;
		else
			bytesToExOr = overHangBytes;

		BlockExOr(S, blockInput, blockOutput, bytesToExOr);
	}
}

void LoRa::CryptJoinServer(CypherKey const& key, uint8 const input[], uint16 length, uint8 output[])
{
	aes_context aesContext;
	aes_set_key(key.Data(), length_type(length), &aesContext);

	aes_decrypt(input, output, &aesContext);
}

void LoRa::CryptJoinMote(CypherKey const& key, uint8 const input[], uint16 length, uint8 output[])
{
	aes_context aesContext;
	aes_set_key(key.Data(), length_type(length), &aesContext);

	aes_encrypt(input, output, &aesContext);
}



LoRa::CypherKey LoRa::GenerateSessionKey(bool generateNetworkKey, CypherKey const& applicationKey, uint32 networkId, uint32 applicationNonce, uint16 deviceNonce)
{
	uint8 input[LoRa::encryptionBlockBytes];

	input[0] = generateNetworkKey ? 0x01 : 0x02;
	uint8* ptr = &input[1];

	ptr = LoRa::Write3ByteValue(ptr, applicationNonce);
	ptr = LoRa::Write3ByteValue(ptr, networkId);
	ptr = LoRa::Write2ByteValue(ptr, deviceNonce);
	memset(ptr, 0, LoRa::encryptionBlockBytes - (ptr - input));

	aes_context aesContext;
	aes_set_key(applicationKey.Data(), LoRa::cypherKeyBytes, &aesContext);

	uint8 output[LoRa::cypherKeyBytes];
	aes_encrypt(input, output, &aesContext);

	CypherKey result = output;

	return result;
}


void LoRa::BlockExOr(uint8 const l[], uint8 const r[], uint8 out[], uint16 bytes)
{
	uint8 const* lptr = l;
	uint8 const* rptr = r;
	uint8* optr = out;
	uint8 const* const end = out + bytes;
	
	for (;optr < end; lptr++, rptr++, optr++)
		*optr = *lptr ^ *rptr;
}

