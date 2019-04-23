
#define __RANDOM_C

#include "TCPIPConfig.h"

#if defined(STACK_USE_SSL_SERVER) || defined(STACK_USE_SSL_CLIENT)

#include "TCPIPStack/TCPIP.h"

static HASH_SUM randHash;
static BYTE output[20];
static BYTE bCount;

void RandomInit(void)
{
	unsigned char i;
	unsigned long dw;

	SHA1Initialize(&randHash);

	// Add some starting entropy to the pool.  This is slow.
	for(i = 0; i < 5; i++)
	{
		dw = GenerateRandomDWORD();
		RandomAdd(((BYTE*)&dw)[0]);
		RandomAdd(((BYTE*)&dw)[1]);
		RandomAdd(((BYTE*)&dw)[2]);
		RandomAdd(((BYTE*)&dw)[3]);
	}

	bCount = 20;
}

BYTE RandomGet(void)
{
	if(bCount >= 20u)
	{//we need to get new random bytes
		SHA1Calculate(&randHash, output);
		RandomAdd(output[0]);
		bCount = 0;
	}

	//return the random byte
	return output[bCount++];
}


void RandomAdd(BYTE data)
{
	DWORD dTemp;

	SHA1AddData(&randHash, &data, 1);
	dTemp = TickGet();
	SHA1AddData(&randHash, (BYTE*)&dTemp, 1);

	bCount = 20;
}

#endif	//ifdefined(STACK_USE_SSL_SERVER) || defined(STACK_USE_SSL_CLIENT)

