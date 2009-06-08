//==================================================================
/// RSLC_Functions.h
///
/// Created by Davide Pasca - 2009/5/30
/// See the file "license.txt" that comes with this project for
/// copyright info. 
//==================================================================

#ifndef RSLC_FUNCTIONS_H
#define RSLC_FUNCTIONS_H

#include "RSLC_Token.h"

//==================================================================
namespace RSLC
{

class TokNode;

//==================================================================
class Function
{
public:
	TokNode			*mpCodeBlkNode;
	Token			*mpRetTypeTok;
	Token			*mpNameTok;

	Function() :
		mpCodeBlkNode(NULL),
		mpRetTypeTok(NULL),
		mpNameTok(NULL)
	{
	}

	~Function()
	{
	}
};

//==================================================================
void DiscoverFunctions( TokNode *pRoot );
void WriteFunctions( FILE *pFile, TokNode *pNode );

//==================================================================
}

#endif