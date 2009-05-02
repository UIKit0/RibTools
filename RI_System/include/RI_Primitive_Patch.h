/*
 *  RI_Primitive_Patch.h
 *  RibTools
 *
 *  Created by Davide Pasca on 09/01/18.
 *  Copyright 2009 Davide Pasca. All rights reserved.
 *
 */

#ifndef RI_PRIMITIVE_PATCH_H
#define RI_PRIMITIVE_PATCH_H

#include "RI_Primitive_Base.h"

//==================================================================
namespace RI
{

//==================================================================
/// PatchBilinear
//==================================================================
class PatchBilinear : public SimplePrimitiveBase
{
private:
	ParamList		mParams;
	Vec3			mHullPos[4];

public:
	PatchBilinear( ParamList &params, const SymbolList &staticSymbols );
	PatchBilinear( ParamList &params, const Vec3 hull[4] );

		PatchBilinear *Clone() const {	return new PatchBilinear( *this ); }

		bool MakeBound( Bound &out_bound ) const;

		void Eval_dPdu_dPdv(
					float u,
					float v,
					Point3 &out_pt,
					Vec3 *out_dPdu,
					Vec3 *out_dPdv ) const;
};

//==================================================================
/// SplinePatchCalc
//==================================================================
class SplinePatchCalc
{
	Vec3 v0;
	Vec3 v1;
	Vec3 v2;
	Vec3 v3;

public:
	void Setup( const RtBasis &b,
				const Vec3 &p0,
				const Vec3 &p1,
				const Vec3 &p2,
				const Vec3 &p3 )
	{
		v0 = b.u.m44[0][0]*p0 + b.u.m44[0][1]*p1 + b.u.m44[0][2]*p2 + b.u.m44[0][3]*p3;
		v1 = b.u.m44[1][0]*p0 + b.u.m44[1][1]*p1 + b.u.m44[1][2]*p2 + b.u.m44[1][3]*p3;
		v2 = b.u.m44[2][0]*p0 + b.u.m44[2][1]*p1 + b.u.m44[2][2]*p2 + b.u.m44[2][3]*p3;
		v3 = b.u.m44[3][0]*p0 + b.u.m44[3][1]*p1 + b.u.m44[3][2]*p2 + b.u.m44[3][3]*p3;
	}

	inline Vec3	Eval( float t ) const
	{
		return	v0 *t*t*t +
				v1 *t*t +
				v2 *t +
				v3 ;
	}

	inline Vec3	EvalDeriv( float t ) const
	{
		return	v0 *3*t*t +
				v1 *2*t +
				v2 ;
	}
};

//==================================================================
/// PatchBicubic
//==================================================================
class PatchBicubic : public SimplePrimitiveBase
{
private:
	ParamList		mParams;
	const RtBasis	*mpUBasis;
	const RtBasis	*mpVBasis;
	Vec3			mHullPos[16];
	SplinePatchCalc	mCalcU[4];
	SplinePatchCalc	mCalcV[4];

public:
	PatchBicubic( ParamList &params, const Attributes &attr, const SymbolList &staticSymbols );
	PatchBicubic( ParamList &params,
							const Vec3 hull[16],
						    const Attributes &attr,
							const SymbolList &staticSymbols );

		PatchBicubic *Clone() const {	return new PatchBicubic( *this ); }

		bool MakeBound( Bound &out_bound ) const;

		void Eval_dPdu_dPdv(
					float u,
					float v,
					Point3 &out_pt,
					Vec3 *out_dPdu,
					Vec3 *out_dPdv ) const;

private:
	void setupEvalCalc();
};

//==================================================================
/// PatchMesh
//==================================================================
class PatchMesh : public ComplexPrimitiveBase
{
public:
	CPSymVoid		mpyPatchType;
	ParamList		mParams;

public:
	PatchMesh(RtToken type,
			  ParamList &params,
			  const SymbolList &staticSymbols );

		void Simplify( HiderREYES &hider );
};

	
//==================================================================
}

#endif