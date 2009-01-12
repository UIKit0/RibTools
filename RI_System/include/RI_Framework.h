/*
 *  RI_Framework.h
 *  RibTools
 *
 *  Created by Davide Pasca on 08/12/17.
 *  Copyright 2008 Davide Pasca. All rights reserved.
 *
 */

#ifndef RI_FRAMEWORK_H
#define RI_FRAMEWORK_H

#include "RI_Base.h"
#include "RI_Primitive.h"
#include "DContainers.h"

//==================================================================
namespace RI
{

class Options;
class Attributes;
class Transform;

//==================================================================
/// Framework
//==================================================================
class Framework
{
	DVec<Primitive *>	mpPrims;

public:
	Framework()
	{
	}
	
	void SetOutput( u_int width, u_int height );
	
	void WorldBegin();
	void Insert( Primitive			*pPrim,
				 const Options		&opt,
				 const Attributes	&attr,
				 const Transform	&xform,
				 const Matrix44		&mtxWorldCamera );
	void WorldEnd();
};

//==================================================================
}

#endif