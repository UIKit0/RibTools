/*
 *  RI_SlShader.cpp
 *  RibTools
 *
 *  Created by Davide Pasca on 09/02/19.
 *  Copyright 2009 Davide Pasca. All rights reserved.
 *
 */

#include "RI_SlShader.h"
#include "RI_SlRunContext.h"
#include "RI_SlShaderAsmParser.h"
#include "DUtils.h"

//==================================================================
namespace RI
{

//==================================================================
/// SlShader
//==================================================================
SlShader::SlShader( const char *pShaderName, const char *pShaderSource ) :
	ResourceBase(pShaderName)
{
	DUT::MemFile	file((const void *)pShaderSource,
						  strlen(pShaderSource) );

	ShaderAsmParser	parser( file, this );
}

//==================================================================
/// SlShaderInstance
//==================================================================
SlShaderInstance::SlShaderInstance() :
	mpShader(NULL)
{
}

//==================================================================
void SlShaderInstance::SetParameter(
			const char		*pParamName,
			SlSymbol::Type	type,
			bool			isVarying,
			void			*pValue )
{
	
}

//==================================================================
SlValue	*SlShaderInstance::Bind( MicroPolygonGrid &g )
{
	size_t	symbolsN = mpShader->mSymbols.size();

	SlValue	*pDataSegment = new SlValue [ symbolsN ];
	
	for (size_t i=0; i < symbolsN; ++i)
	{
		SlSymbol	&symbol = mpShader->mSymbols[i];

		switch ( symbol.mStorage )
		{
		case SlSymbol::CONSTANT:
			pDataSegment[i].Data.pVoidValue =
								symbol.mpDefaultVal;
			break;

		case SlSymbol::PARAMETER:
			{
				SlSymbol	*pFoundSymbol = NULL;

				pFoundSymbol =
					g.mSymbols.LookupVariable(
									symbol.mName.c_str(),
									symbol.mType,
									symbol.mIsVarying );

				if NOT( pFoundSymbol )
				{
					pFoundSymbol =
						mCallingParams.LookupVariable(
											symbol.mName.c_str(),
											symbol.mType,
											symbol.mIsVarying );
				}

				if NOT( pFoundSymbol )
				{
					if NOT( symbol.mpDefaultVal )
						DASSTHROW( 0, ("Could not bind symbol %s", symbol.mName.c_str()) );

					pDataSegment[i].Data.pVoidValue = symbol.mpDefaultVal;
				}
				else
					pDataSegment[i].Data.pVoidValue = pFoundSymbol->mpDefaultVal;

			}
			break;

		case SlSymbol::TEMPORARY:			
			pDataSegment[i].Data.pVoidValue = symbol.AllocClone( g.mPointsN );
			break;

		case SlSymbol::GLOBAL:
			break;
		}
	}

	return pDataSegment;
}

//==================================================================
void SlShaderInstance::Unbind( SlValue * &pDataSegment )
{
	size_t	symbolsN = mpShader->mSymbols.size();

	for (size_t i=0; i < symbolsN; ++i)
	{
		SlSymbol	&symbol = mpShader->mSymbols[i];

		switch ( symbol.mStorage )
		{
		case SlSymbol::CONSTANT:
			pDataSegment[i].Data.pVoidValue = NULL;
			break;

		case SlSymbol::PARAMETER:
			pDataSegment[i].Data.pVoidValue = NULL;
			break;

		case SlSymbol::TEMPORARY:			
			symbol.FreeClone( pDataSegment[i].Data.pVoidValue );
			break;

		case SlSymbol::GLOBAL:
			break;
		}
	}
	
	delete [] pDataSegment;
	pDataSegment = NULL;
}


//==================================================================
typedef void (*ShaderInstruction)( SlRunContext &ctx );

//==================================================================
template <class TA, class TB>
void Inst_Copy( SlRunContext &ctx )
{
		  TA*	lhs	= (		 TA*)ctx.GetVoid( 1 );
	const TB*	op1	= (const TB*)ctx.GetVoid( 2 );
	
	bool	lhs_varying = ctx.IsSymbolVarying( 1 );
	
	if ( lhs_varying )
	{
		bool	op1_varying = ctx.IsSymbolVarying( 2 );
		int		op1_offset = 0;
		
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
		{
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = op1[op1_offset];

			if ( op1_varying )	++op1_offset;
		}
	}
	else
	{
		DASSERT( !ctx.IsSymbolVarying( 2 ) );
				 
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = op1[0];
	}

	ctx.NextInstruction();
}

//==================================================================
template <class TA, class TB, const char OPNAME>
void Inst_AlOp( SlRunContext &ctx )
{
		  TA*	lhs	= (		 TA*)ctx.GetFloat( 1 );
	const TA*	op1	= (const TA*)ctx.GetFloat( 2 );
	const TB*	op2	= (const TB*)ctx.GetFloat( 3 );
	
	bool	lhs_varying = ctx.IsSymbolVarying( 1 );
	
	if ( lhs_varying )
	{
		bool	op1_varying = ctx.IsSymbolVarying( 2 );
		bool	op2_varying = ctx.IsSymbolVarying( 3 );
		int		op1_offset = 0;
		int		op2_offset = 0;
		
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
		{
			if ( ctx.IsProcessorActive( i ) )
			{
				if ( OPNAME == '+' ) lhs[i] = op1[op1_offset] + op2[op2_offset]; else
				if ( OPNAME == '-' ) lhs[i] = op1[op1_offset] - op2[op2_offset]; else
				if ( OPNAME == '*' ) lhs[i] = op1[op1_offset] * op2[op2_offset]; else
				if ( OPNAME == '/' ) lhs[i] = op1[op1_offset] / op2[op2_offset];
			}
			
			if ( op1_varying )	++op1_offset;
			if ( op2_varying )	++op2_offset;
		}
	}
	else
	{
		DASSERT( !ctx.IsSymbolVarying( 2 ) &&
				 !ctx.IsSymbolVarying( 3 ) );

		TA	tmp;
		if ( OPNAME == '+' ) tmp = op1[0] + op2[0]; else
		if ( OPNAME == '-' ) tmp = op1[0] - op2[0]; else
		if ( OPNAME == '*' ) tmp = op1[0] * op2[0]; else
		if ( OPNAME == '/' ) tmp = op1[0] / op2[0];

		for (u_int i=0; i < ctx.mSIMDCount; ++i)
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = tmp;
	}

	ctx.NextInstruction();
}

//==================================================================
static void Inst_Normalize( SlRunContext &ctx )
{
		  Vector3*	lhs	= (		 Vector3*)ctx.GetVoid( 1 );
	const Vector3*	op1	= (const Vector3*)ctx.GetVoid( 2 );
	
	bool	lhs_varying = ctx.IsSymbolVarying( 1 );
	
	if ( lhs_varying )
	{
		bool	op1_varying = ctx.IsSymbolVarying( 2 );
		int		op1_offset = 0;
		
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
		{
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = op1[op1_offset].GetNormalized();

			if ( op1_varying )	++op1_offset;
		}
	}
	else
	{
		DASSERT( !ctx.IsSymbolVarying( 2 ) );

		Vector3	tmp = op1[0].GetNormalized();

		for (u_int i=0; i < ctx.mSIMDCount; ++i)
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = tmp;
	}

	ctx.NextInstruction();
}

//==================================================================
inline float sign( float val )
{
	if ( val < 0 ) return -1; else
	if ( val > 0 ) return 1; else
				   return 0;
}

//==================================================================
static void Inst_Faceforward( SlRunContext &ctx )
{
		  Vector3*	lhs	= (		 Vector3*)ctx.GetVoid( 1 );
	const Vector3*	pN	= (const Vector3*)ctx.GetVoid( 2 );
	const Vector3*	pI	= (const Vector3*)ctx.GetVoid( 3 );
	
	const SlSymbol*	pNgSymbol = ctx.mpSymbols->LookupVariable( "Ng", SlSymbol::NORMAL );
	const Vector3*	pNg = (const Vector3 *)pNgSymbol->GetData();
	
	bool	lhs_varying = ctx.IsSymbolVarying( 1 );
	bool	N_varying	= ctx.IsSymbolVarying( 2 );
	bool	I_varying	= ctx.IsSymbolVarying( 3 );
	bool	Ng_varying	= pNgSymbol->mIsVarying;
	
	if ( lhs_varying )
	{
		int		N_offset	= 0;
		int		I_offset	= 0;
		int		Ng_offset	= 0;
		
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
		{
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = pN[N_offset] * sign( pI[N_offset].GetDot( pNg[Ng_offset] ) );

			if ( N_varying )	++N_offset;
			if ( I_varying )	++I_offset;
			if ( Ng_varying )	++Ng_varying;
		}
	}
	else
	{
		DASSERT( !N_varying		);
		DASSERT( !I_varying		);
		DASSERT( !Ng_varying	);

		Vector3	tmp = pN[0] * sign( pI[0].GetDot( pNg[0] ) );

		for (u_int i=0; i < ctx.mSIMDCount; ++i)
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = tmp;
	}

	ctx.NextInstruction();
}

//==================================================================
// this is a simplified version.. until lights become available 8)
static void Inst_Diffuse( SlRunContext &ctx )
{
		  Color*	lhs	= (		 Vector3*)ctx.GetVoid( 1 );
	const Color*	op1	= (const Vector3*)ctx.GetVoid( 2 );

	bool	lhs_varying = ctx.IsSymbolVarying( 1 );
	
	if ( lhs_varying )
	{
		bool	op1_varying = ctx.IsSymbolVarying( 2 );
		int		op1_offset = 0;
		
		for (u_int i=0; i < ctx.mSIMDCount; ++i)
		{
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = op1[op1_offset].GetNormalized().GetDot( Vector3(0, 0, 1) );

			if ( op1_varying )	++op1_offset;
		}
	}
	else
	{
		DASSERT( !ctx.IsSymbolVarying( 2 ) );

		Vector3	tmp = op1[0].GetNormalized().GetDot( Vector3(0, 0, 1) );

		for (u_int i=0; i < ctx.mSIMDCount; ++i)
			if ( ctx.IsProcessorActive( i ) )
				lhs[i] = tmp;
	}

	ctx.NextInstruction();
}

//==================================================================
// this is a simplified version.. until lights become available 8)
static void Inst_Ambient( SlRunContext &ctx )
{
	Color*	lhs	= (		 Color*)ctx.GetVoid( 1 );

	for (u_int i=0; i < ctx.mSIMDCount; ++i)
	{
		if ( ctx.IsProcessorActive( i ) )
			lhs[i] = Vector3( 0.2f, 0.2f, 0.2f );
	}

	ctx.NextInstruction();
}

//==================================================================
#define SINGLE	float
#define VECTOR	Vector3
#define MATRIX	Matrix44

//==================================================================
static ShaderInstruction	sInstructionTable[OP_N] =
{
Inst_Copy<SINGLE,SINGLE>,
Inst_Copy<VECTOR,SINGLE>,
Inst_Copy<VECTOR,VECTOR>,

Inst_AlOp<SINGLE,SINGLE,'+'>,
Inst_AlOp<VECTOR,SINGLE,'+'>,
Inst_AlOp<VECTOR,VECTOR,'+'>,

Inst_AlOp<SINGLE,SINGLE,'-'>,
Inst_AlOp<VECTOR,SINGLE,'-'>,
Inst_AlOp<VECTOR,VECTOR,'-'>,

Inst_AlOp<SINGLE,SINGLE,'*'>,
Inst_AlOp<VECTOR,SINGLE,'*'>,
Inst_AlOp<VECTOR,VECTOR,'*'>,

Inst_AlOp<SINGLE,SINGLE,'/'>,
Inst_AlOp<VECTOR,SINGLE,'/'>,
Inst_AlOp<VECTOR,VECTOR,'/'>,

Inst_Normalize,
Inst_Faceforward,
Inst_Diffuse,
Inst_Ambient,
};

//==================================================================
void SlShaderInstance::Run( MicroPolygonGrid &g )
{
	SlRunContext	ctx;
	
	ctx.mProgramCounter = 0;
	ctx.mpDataSegment	= Bind( g );
	ctx.InitializeSIMD( g );
	ctx.mpShaderInst	= this;

	while ( ctx.mProgramCounter < mpShader->mCode.size() )
	{
		const SlCPUWord	*pWord = ctx.GetOp( 0 );
		
		// [pWord->mOpCode.mDestOpType]
		sInstructionTable[pWord->mOpCode.mTableOffset]( ctx );
	}
	
	Unbind( ctx.mpDataSegment );
}

//==================================================================
}
