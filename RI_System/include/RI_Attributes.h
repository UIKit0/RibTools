/*
 *  RI_Attributes.h
 *  ribparser
 *
 *  Created by Davide Pasca on 08/12/17.
 *  Copyright 2008 Davide Pasca. All rights reserved.
 *
 */

#ifndef RI_ATTRIBUTES_H
#define RI_ATTRIBUTES_H

#include "RI_Base.h"
#include "RI_Param.h"
#include "RI_Tokens.h"
#include "RI_Symbol.h"
#include "RI_SlShader.h"
#include "RI_LightSource.h"

//==================================================================
namespace RI
{

//==================================================================
class State;

//==================================================================
/// Attributes
//==================================================================
class Attributes
{
public:
	State			*mpState;

private:
	SymbolList		*mpStatics;
	ResourceManager *mpResManager;

public:
	RevisionTracker	*mpRevision;

public:
	//==================================================================
	SymbolList			mSymbols;
					
	Bound				mBound;				// DoBound()
	Bound				mDetail;			// Detail()
				
	float				mMinVisible;		// DetailRange()
	float				mLowerTransition;	//
	float				mUpperTransition;	//
	float				mMaxVisible;		//
					
	CPSymVoid			mpyTypeApproximation;	// GeometricApproximation()
	float				mValueApproximation;
					
	bool				mOrientationFlipped;// Orientation()
	int					mSides;				// Sides()

	CPSymBasis			mpyUBasis;			// Basis()
	CPSymBasis			mpyVBasis;
	const RtBasis		*mpCustomUBasis;
	const RtBasis		*mpCustomVBasis;
	
	int					mUSteps;
	int					mVSteps;

	Color				mColor;
	Color				mOpacity;
	SlShaderInstance	mShaderInstance;

	DVec<U16>			mActiveLights;

public:
	Attributes();
	Attributes( const Attributes &attributes );

	Attributes& operator=(const Attributes& rhs);

	void Init(
			State			*pState,
			SymbolList		*pStatics,
			ResourceManager	*pResManager,
			RevisionTracker	*pRevision );

	~Attributes();

private:
	void copyFrom(const Attributes& rhs);

public:
	void cmdBound( const Bound &bound );
	void cmdDetail( const Bound &detail );
	void cmdDetailRange(float	minVisible,
						float	lowerTransition,
						float	upperTransition,
						float	maxVisible );
	void cmdGeometricApproximation(RtToken typeApproximation,
								   float valueApproximation );
	
	void cmdOrientation( RtToken orientation );
	void cmdSides( int sides );
	void cmdBasis(RtToken ubasis, const float *pCustomUBasis, int ustep,
				  RtToken vbasis, const float *pCustomVBasis, int vstep );
				  
	const RtBasis &GetUBasis() const { return mpCustomUBasis ? *mpCustomUBasis : mpyUBasis->value; }
	const RtBasis &GetVBasis() const { return mpCustomVBasis ? *mpCustomVBasis : mpyVBasis->value; }

	void cmdColor( const Color &color );
	void cmdOpacity( const Color &color );
	bool cmdLightSource( ParamList &params, const Transform &xform, const Matrix44 &mtxWorldCam );
	void cmdSurface( ParamList &params );

private:
	SlShader *loadShader( const char *pBasePath, const char *pSName );
};

//==================================================================
}


#endif