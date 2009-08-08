//==================================================================
/// RibRenderLib_Net.cpp
///
/// Created by Davide Pasca - 2009/8/6
/// See the file "license.txt" that comes with this project for
/// copyright info. 
//==================================================================

#include "RibRenderLib_Net.h"
#include "DSystem/include/DNetwork_Connecter.h"
#include "DSystem/include/DUtils_MemFile.h"

//==================================================================
namespace RRL
{
//==================================================================
namespace NET
{

//===============================================================
void ConnectToServers( DVec<Server> &srvList, U32 timeoutMS )
{
	DVec<DNET::Connecter *>	pConnecters;
	
	pConnecters.resize( srvList.size() );

	// fill up the connection objects
	for (size_t i=0; i < srvList.size(); ++i)
	{
		DNET::Connecter *pConn;

		try {
			pConn =	DNEW DNET::Connecter(
								srvList[i].mAddressName.c_str(),
								srvList[i].mPortToCall );

			srvList[i].mIsValid = true;
			pConnecters[i] = pConn;
		}
		catch ( ... )
		{
			srvList[i].mIsValid = false;
			pConnecters[i] = NULL;
		}
	}

	DUT::TimeOut	timeOut( timeoutMS );

	// loop until there is some connection waiting
	bool	isSomeConnectionWaiting = false;
	do
	{
		for (size_t i=0; i < pConnecters.size(); ++i)
		{
			DNET::Connecter *pConn = pConnecters[i];

			if ( pConn->IsConnected() )
				continue;

			switch ( pConn->TryConnect() )
			{
			case DNET::Connecter::RETVAL_WAITING:
				isSomeConnectionWaiting = true;
				break;

			case DNET::Connecter::RETVAL_CONNECTED:
				DASSERT( srvList[i].mpPakMan == NULL );
				srvList[i].Init( 
							DNEW DNET::PacketManager( pConn->GetSocket() ) );
				break;

			case DNET::Connecter::RETVAL_ERROR:
				srvList[i].mIsValid = false;
				break;

			default:
				DASSERT( 0 );
				break;
			}
		}

		if ( isSomeConnectionWaiting )
		{
		#if defined WIN32
			Sleep( 1 );	// avoid overloading the CPU !
		#endif
		}

		if ( timeOut.IsExpired() )
			break;

	} while ( isSomeConnectionWaiting );

	// delete the connection objects
	for (size_t i=0; i < pConnecters.size(); ++i)
		DSAFE_DELETE( pConnecters[i] );
}


}

//==================================================================
}