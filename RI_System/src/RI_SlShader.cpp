//==================================================================
/// RI_SlShader.cpp
///
/// Created by Davide Pasca - 2009/2/19
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include "stdafx.h"
#include "RI_SlShader.h"
#include "RI_SlRunContext.h"
#include "RI_SlShaderAsmParser.h"
#include "RI_Attributes.h"
#include "RI_State.h"
#include "DUtils.h"
#include "RI_SlShader_OpCodeFuncs.h"
#include "RSLCompilerLib/include/RSLCompiler.h"
#include "DSystem/include/DUtils_Files.h"

//==================================================================
//#define FORCE_MEM_CORRUPTION_CHECK

//==================================================================
namespace RI
{

//==================================================================
static void compileSLToAsm(
						DUT::MemFile &slSource,
						const char *pSrcFPathName,
						const char *pAppResDir,
						const char *pAsmOutName )
{
	// compile
	std::string	basInclude( pAppResDir );
	basInclude += "/RSLC_Builtins.sl";

	try {
		RSLCompiler::Params	params;
		params.mDbgOutputTree = false;

		// try compile
		RSLCompiler	compiler(
						(const char *)slSource.GetData(),
						slSource.GetDataSize(),
						basInclude.c_str(),
						params
					);

		// save autogen rrasm file
		compiler.SaveASM( pAsmOutName, pSrcFPathName );
	}
	catch ( RSLC::Exception &e )
	{
		printf(
			"ERROR: while compiling '%s'..\n%s\n",
			pSrcFPathName,
			e.GetMessage().c_str() );
	}
	catch ( ... )
	{
		printf( "ERROR while compiling '%s'\n", pSrcFPathName );
	}
}

//==================================================================
static void compileFromMemFile(
				DUT::MemFile &file,
				SlShader *pShader,
				const char *pFileName,
				const char *pShaderName,
				const char *pAppResDir,
				FileManagerBase &fileManager )
{
	bool	isSL =
		(0 == strcasecmp( DUT::GetFileNameExt( pFileName ), "sl" ) );

	if NOT( isSL )
	{
		// compile/parse the rrasm file and return right away
		ShaderAsmParser	parser( file, pShader, pShaderName );
		return;
	}

	// make the autogen name
	char	asmOutName[1024];
	strcpy_s( asmOutName, pFileName );
	DUT::GetFileNameExt( asmOutName )[0] = 0;	// cut the extension
	strcat_s( asmOutName, "autogen.rrasm" );	// make the .autogen etc name

	DUT::MemFile	autogenAsmFile;

	// does the autogen file already exist ?
	if ( fileManager.FileExists( asmOutName ) )
	{
		// just get it from the file manager
		fileManager.GrabFile( asmOutName, autogenAsmFile );
	}
	else
	{
		// ..otherwise.. compile the sl into an autogen.rrasm
		compileSLToAsm( file, pFileName, pAppResDir, asmOutName );
		// ..and also read in the file..
		autogenAsmFile.Init( asmOutName );
	}

	// now parse/compile the rrasm file
	ShaderAsmParser	parser( autogenAsmFile, pShader, pShaderName );
}

//==================================================================
/// SlShader
//==================================================================
SlShader::SlShader( const CtorParams &params, FileManagerBase &fileManager ) :
	ResourceBase(params.pName, ResourceBase::TYPE_SHADER),
	mType(TYPE_UNKNOWN),
	mStartPC(INVALID_PC),
	mHasDirPosInstructions(false)
{
	DUT::MemFile	file;

	if ( params.pSource )
	{
		file.Init( (const void *)params.pSource, strlen(params.pSource) );

		compileFromMemFile( file, this, params.pSourceFileName, params.pName, params.pAppResDir, fileManager );
	}
	else
	if ( params.pSourceFileName )
	{
		fileManager.GrabFile( params.pSourceFileName, file );

		compileFromMemFile( file, this, params.pSourceFileName, params.pName, params.pAppResDir, fileManager );
	}
	else
	{
		DASSTHROW( 0, ("Missing parameters !") );
	}
}

//==================================================================
/// SlShaderInst
//==================================================================
SlShaderInst::SlShaderInst( SlShader *pShader, size_t maxPointsN ) :
	moShader(pShader),
	mMaxPointsN(maxPointsN)
{
}

//==================================================================
SlShaderInst::~SlShaderInst()
{
}

//==================================================================
void SlShaderInst::runFrom( SlRunContext &ctx, u_int startPC ) const
{
	ctx.mProgramCounterIdx = 0;
	ctx.mProgramCounter[ ctx.mProgramCounterIdx ] = startPC;
	ctx.mIlluminanceCtx.Reset();
	ctx.mIsInSolar = false;

	const SlCPUWord	*pWord = NULL;

	try {
		while ( true )
		{
			if ( ctx.mProgramCounter[ctx.mProgramCounterIdx] >= moShader->mCode.size() )
				return;

			pWord = ctx.GetOp( 0 );

			if ( pWord->mOpCode.mTableOffset == OP_RET )
			{
				// what ? Subroutines ?
				if ( ctx.mProgramCounterIdx == 0 )
					return;

				ctx.mProgramCounterIdx -= 1;
			}

			// [pWord->mOpCode.mDestOpType]
			_gSlOpCodeFuncs[pWord->mOpCode.mTableOffset]( ctx );

#if defined(FORCE_MEM_CORRUPTION_CHECK)
			const char *pDude = DNEW char;
			delete pDude;
#endif
		}
	}
	catch ( ... )
	{
		printf( "SHADER ERROR: %s failed at line %i !!\n",
					moShader->mShaderName.c_str(),
						pWord->mOpCode.mDbgLineNum );
	}
}

//==================================================================
void SlShaderInst::Run( SlRunContext &ctx ) const
{
	// reset the program counter
	ctx.mProgramCounterIdx = 0;
	ctx.mProgramCounter[ 0 ] = INVALID_PC;

	// initialize the SIMD state
	ctx.InitializeSIMD( mMaxPointsN );

	// initialize the non uniform/constant values with eventual default data
	for (size_t i=0; i < ctx.mpShaderInst->moShader->mpShaSyms.size(); ++i)
	{
		SlValue	&slValue = ctx.mpDataSegment[i];

		if ( slValue.Flags.mOwnData &&
			 slValue.mpSrcSymbol->mpConstVal != NULL )
		{
			DASSERT(
				slValue.Data.pVoidValue != NULL &&
				slValue.mpSrcSymbol->IsConstant() );

			slValue.mpSrcSymbol->CopyConstValue( slValue.Data.pVoidValue );
		}
	}

	// run the default params fill subroutines (the instance
	// determines which need to be called)
	for (size_t i=0; i < ctx.mDefParamValsStartPCs.size(); ++i)
	{
		runFrom( ctx, ctx.mDefParamValsStartPCs[i] );
	}

	// run the main
	runFrom( ctx, moShader->mStartPC );
}

//==================================================================
}
