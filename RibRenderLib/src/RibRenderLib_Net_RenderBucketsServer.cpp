//==================================================================
/// RibRenderLib_Net_RenderBucketsServer.cpp
///
/// Created by Davide Pasca - 2009/8/6
/// See the file "license.txt" that comes with this project for
/// copyright info. 
//==================================================================

#include "RibRenderLib_Net_RenderBucketsServer.h"

//==================================================================
namespace RRL
{
//==================================================================
namespace NET
{

//==================================================================
/// RenderBucketsServer
//==================================================================
RenderBucketsServer::RenderBucketsServer( DNET::PacketManager &pakMan ) :
	mpPakMan(&pakMan)
{
}

//==================================================================
void RenderBucketsServer::Render( RI::HiderREYES &hider )
{
	DNET::Packet	*pPacket;

	while ( true )
	{
		pPacket = mpPakMan->WaitNextPacket( false );

		DASSTHROW(
			pPacket->mDataBuff.size() >= sizeof(U32),
				("Unexpected packet in RenderBucketsServer") );

		MsgID	msgID = GetMsgID( pPacket );

		if ( msgID == MSGID_RENDBUCKETS )
		{
			const MsgRendBuckes	&msg = *((const MsgRendBuckes *)&pPacket->mDataBuff[0]);

			rendBucketsRange( hider, msg.BucketStart, msg.BucketEnd );

			sendBucketsData( hider, msg.BucketStart, msg.BucketEnd );

			mpPakMan->RemoveAndDeletePacket( pPacket );
		}
		else
		if ( msgID == MSGID_RENDDONE )
		{
			mpPakMan->RemoveAndDeletePacket( pPacket );
			break;
		}
/*
		else
		{
			DASSTHROW(
				pPacket->mDataBuff.size() >= sizeof(U32),
					("Unexpected packet in RenderBucketsServer") );
		}
*/
	}
}

//==================================================================
void RenderBucketsServer::rendBucketsRange( RI::HiderREYES &hider, int buckRangeX1, int buckRangeX2 )
{
	const DVec<RI::Bucket *> &buckets = hider.GetBuckets();

	DASSERT(
		buckRangeX1 >= 0 &&
		buckRangeX1 <= buckRangeX2 &&
		buckRangeX2 < (int)buckets.size() );

	#pragma omp parallel for
	for (int bi=buckRangeX1; bi < buckRangeX2; ++bi)
		RI::FrameworkREYES::RenderBucket_s( hider, *buckets[ bi ] );
}

//==================================================================
void RenderBucketsServer::sendBucketsData( RI::HiderREYES &hider, int buckRangeX1, int buckRangeX2 )
{
	const DVec<RI::Bucket *> &buckets = hider.GetBuckets();

	for (int bi=buckRangeX1; bi < buckRangeX2; ++bi)
	{
		size_t	buckMemSize = hider.GetOutputBucketMemSize( (size_t)bi );

		U8 *pSendData = mpPakMan->SendBegin( sizeof(MsgBucketData) + buckMemSize );

			MsgBucketData	&msg = *(MsgBucketData *)pSendData; pSendData += sizeof(MsgBucketData);
			msg.BucketIdx = bi;

			hider.CopyOutputBucket( (size_t)bi, (float *)pSendData, buckMemSize );

		mpPakMan->SendEnd();
	}
}

//==================================================================
}
//==================================================================
}