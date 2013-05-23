// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2011 Nicolas Schulz
//	D3D11 version 2012-2013 Attila Kocsis
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#include "../egRendererBase.h"
#include "../egRenderer.h"	//*todo: used for scratch - remove this when it is optimized
#include "../egModules.h"
#include "../egCom.h"

#include "utDebug.h"
#include <D3Dcompiler.h>
#include <D3DX11tex.h>
#include <float.h>

#define SAFE_RELEASE(obj) if (obj!=NULL) { obj->Release(); obj=NULL; }

namespace Horde3D {

static const char *defaultShaderVS =
	"matrix viewProjMat;\n"
	"matrix worldMat;\n"
	"struct VS_INPUT { float4 Position : POSITION; };\n"
	"struct VS_OUTPUT { float4 Position : SV_POSITION; };\n"
	"VS_OUTPUT main(VS_INPUT	In) {\n"
    "  VS_OUTPUT Out = (VS_OUTPUT)0;\n"
	"  Out.Position = mul(viewProjMat, mul(worldMat,In.Position));\n"
	"  return Out;\n"
	"}\n";

static const char *defaultShaderFS =
	" struct VS_OUTPUT { float4 Position : SV_POSITION; };\n"
	" float4 color;\n"
	" float4 main( VS_OUTPUT input) : SV_Target { \n"
	"	return color;\n"
	"}\n";

// =================================================================================================
// GPUTimer
// =================================================================================================

GPUTimer::GPUTimer() : _numQueries( 0 ),  _queryFrame( 0 ), _time( 0 ), _activeQuery( false )
{
	reset();
}


GPUTimer::~GPUTimer()
{
//*	if( !_queryPool.empty() )
//*		glDeleteQueries( (uint32)_queryPool.size(), &_queryPool[0] );
}


void GPUTimer::beginQuery( uint32 frameID )
{
	return;
//*	if( !glExt::ARB_timer_query ) return;
//*	ASSERT( !_activeQuery );
//*	
//*	if( _queryFrame != frameID )
//*	{
//*		if( !updateResults() ) return;
//*
//*		_queryFrame = frameID;
//*		_numQueries = 0;
//*	}
//*	
//*	// Create new query pair if necessary
//*	uint32 queryObjs[2];
//*	if( _numQueries++ * 2 == _queryPool.size() )
//*	{
//*		glGenQueries( 2, queryObjs );
//*		_queryPool.push_back( queryObjs[0] );
//*		_queryPool.push_back( queryObjs[1] );
//*	}
//*	else
//*	{
//*		queryObjs[0] = _queryPool[(_numQueries - 1) * 2];
//*	}
//*	
//*	_activeQuery = true;
//*	 glQueryCounter( queryObjs[0], GL_TIMESTAMP );
}


void GPUTimer::endQuery()
{
//*	if( _activeQuery )
//*	{	
//*		glQueryCounter( _queryPool[_numQueries * 2 - 1], GL_TIMESTAMP );
//*		_activeQuery = false;
//*	}
}


bool GPUTimer::updateResults()
{
	return false;
//*	if( !glExt::ARB_timer_query ) return false;
//*	
//*	if( _numQueries == 0 )
//*	{
//*		_time = 0;
//*		return true;
//*	}
//*	
//*	// Make sure that last query is available
//*	GLint available;
//*	glGetQueryObjectiv( _queryPool[_numQueries * 2 - 1], GL_QUERY_RESULT_AVAILABLE, &available );
//*	if( !available ) return false;
//*	
//*	//  Accumulate time
//*	GLuint64 timeStart = 0, timeEnd = 0, timeAccum = 0;
//*	for( uint32 i = 0; i < _numQueries; ++i )
//*	{
//*		glGetQueryObjectui64v( _queryPool[i * 2], GL_QUERY_RESULT, &timeStart );
//*		glGetQueryObjectui64v( _queryPool[i * 2 + 1], GL_QUERY_RESULT, &timeEnd );
//*		timeAccum += timeEnd - timeStart;
//*	}
//*	
//*	_time = (float)((double)timeAccum / 1000000.0);
//*	return true;
}


void GPUTimer::reset()
{
//*	_time = glExt::ARB_timer_query ? 0.f : -1.f;
}


// =================================================================================================
// RenderDevice
// =================================================================================================

RenderDevice::RenderDevice(void* device)
{
	_d3dDevice = (ID3D11Device*)device;
	_d3dDevice->GetImmediateContext(&_d3dContext);
	_numVertexLayouts = 0;
	_activeVertexAttribsMask = 0;
	
	_vpX = 0; _vpY = 0; _vpWidth = 320; _vpHeight = 240;
	_scX = 0; _scY = 0; _scWidth = 320; _scHeight = 240;
	_prevShaderId = _curShaderId = 0;
	_curRendBuf = 0; _outputBufferIndex = 0;
	_textureMem = 0; _bufferMem = 0;
	_curRasterState.hash = _newRasterState.hash = 0;
	_curBlendState.hash = _newBlendState.hash = 0;
	_curDepthStencilState.hash = _newDepthStencilState.hash = 0;
	_curVertLayout = _newVertLayout = 0;
	_curIndexBuf = _newIndexBuf = 0;
	for(uint32 i = 0; i<RDIRenderBuffer::MaxColorAttachmentCount; ++i)
		_defaultRenderTargetViews[i] = 0x0;
	_defaultDepthStencilView = 0x0;

	for(uint32 i = 0; i<RDIRasterState::numStates; ++i)
		_rasterizerStates[ i ] = 0x0;
	for(uint32 i = 0; i<RDIBlendState::numStates; ++i)
		_blendStates[ i ] = 0x0;
	for(uint32 i = 0; i<RDIDepthStencilState::numStates; ++i)
		_depthStencilStates[ i ] = 0x0;
	for(uint32 i = 0; i<RDISamplerNumStates; ++i)
		_samplerStates[ i ] = 0x0;

	_indexFormat = (uint32)IDXFMT_16;
	_pendingMask = 0;
}


RenderDevice::~RenderDevice()
{
	for(uint32 i = 0; i<RDIRasterState::numStates; ++i)
		SAFE_RELEASE( _rasterizerStates[ i ] );
	for(uint32 i = 0; i<RDIBlendState::numStates; ++i)
		SAFE_RELEASE( _blendStates[ i ] );
	for(uint32 i = 0; i<RDIDepthStencilState::numStates; ++i)
		SAFE_RELEASE( _depthStencilStates[ i ] );
	for(uint32 i = 0; i<RDISamplerNumStates; ++i)
		SAFE_RELEASE( _samplerStates[ i ] );
	_d3dContext->ClearState();
	_d3dContext->Flush();
}


void RenderDevice::initStates()
{
}


bool RenderDevice::init()
{
	bool failed = false;

	Modules::log().writeInfo( "Initializing D3D11 backend");
	
	// Check feature level (at least 9_2 needed now)
	if( _d3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_9_2 )
	{
		Modules::log().writeError( "D3D Feature Level 9.2 not available" );
		failed = true;
	}
	
//*	if( !glExt::EXT_texture_sRGB )
//*	{
//*		Modules::log().writeError( "Extension EXT_texture_sRGB not supported" );
//*		failed = true;
//*	}
	
	if( failed )
	{
		Modules::log().writeError( "Failed to init renderer backend" );
		return false;
	}
	
	// Get capabilities
	_caps.texDXT = true;
	_caps.texPVRTCI = false;
	_caps.texETC1 = false;

	_caps.texFloat = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_10_0;
	_caps.texDepth = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_10_0;
	_caps.texShadowCompare = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_10_0;

	_caps.tex3D = true; // always true but depends on format
	_caps.texNPOT = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_3;

	_caps.rtMultisampling = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_1;	//depends on format (B8G8R8A8_UNORM 9.1+, R8G8B8A8_UNORM 9.3+)
	_caps.rtMaxColBufs = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_3 ? 4 : 1;

	_caps.occQuery = _d3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_2; 
	_caps.timerQuery = false; // TODO

	if ( _d3dDevice->GetFeatureLevel() <= D3D_FEATURE_LEVEL_9_3 )
		_depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	//no shadow sampling, cannot be used as texture
	else if ( _d3dDevice->GetFeatureLevel() <= D3D_FEATURE_LEVEL_10_1 )
		_depthFormat = DXGI_FORMAT_R32_TYPELESS;  // used as d32_float, r32_float  sample_c
	else
		_depthFormat = DXGI_FORMAT_R24G8_TYPELESS; // used as R24_UNORM_X8_TYPELESS, D24_UNORM_S8_UINT , sample_c, gather

	initStates();
	resetStates();

	return true;
}


// =================================================================================================
// Vertex layouts
// =================================================================================================

uint32 RenderDevice::registerVertexLayout( uint32 numAttribs, VertexLayoutAttrib *attribs )
{
	if( _numVertexLayouts == MaxNumVertexLayouts )
		return 0;
	
	_vertexLayouts[_numVertexLayouts].numAttribs = numAttribs;

	for( uint32 i = 0; i < numAttribs; ++i )
		_vertexLayouts[_numVertexLayouts].attribs[i] = attribs[i];

	return ++_numVertexLayouts;
}


// =================================================================================================
// Buffers
// =================================================================================================

void RenderDevice::beginRendering()
{		
	//	Get the currently bound frame buffer object. 
	_d3dContext->OMGetRenderTargets(RDIRenderBuffer::MaxColorAttachmentCount, _defaultRenderTargetViews, &_defaultDepthStencilView);

	resetStates();
}

void RenderDevice::finishRendering()
{
		//set back original rendertarget (and release references)
	_d3dContext->OMSetRenderTargets(RDIRenderBuffer::MaxColorAttachmentCount, _defaultRenderTargetViews, _defaultDepthStencilView);
	for(uint32 i = 0; i<RDIRenderBuffer::MaxColorAttachmentCount; ++i)
		SAFE_RELEASE(_defaultRenderTargetViews[i]);
	SAFE_RELEASE(_defaultDepthStencilView);
}

uint32 RenderDevice::createVertexBuffer( uint32 size, const void *data, bool dynamic )
{
	return createBuffer( size, data, dynamic, D3D11_BIND_VERTEX_BUFFER );
}


uint32 RenderDevice::createIndexBuffer( uint32 size, const void *data, bool dynamic )
{
	return createBuffer( size, data, dynamic, D3D11_BIND_INDEX_BUFFER );
}

uint32 RenderDevice::createUniformBuffer( uint32 size, const void *data )
{
	return createBuffer( size, data, true, D3D11_BIND_CONSTANT_BUFFER );
}

//constant buffer trick:
// have in a shader header (hlsl and c++)
// slot by convention: b0: per frame, b1: per camera, ...

uint32 RenderDevice::createBuffer( uint32 size, const void *data, bool dynamic, uint32 bindFlags )
{
	D3D11_BUFFER_DESC desc;
	//buffer handling
	// forever: immutable - no update
	// long lived: default - updatesubresource (using this now)
	// temporary: dynamic - map/unmap (updatesubresource is slow - extra copy)
	// constant: dynamic - Map/discard to update - can have several of these

	desc.ByteWidth = size;
	desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = data;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	RDIBuffer buf;
	buf.type = bindFlags;
	buf.size = size;
	buf.d3dObj = 0;
	buf.dynamic = dynamic;
	if (size > 0)
	{	// d3d cannot create 0 sized buffer
		HRESULT hr = _d3dDevice->CreateBuffer( &desc, data!=NULL ? &initData : NULL, &buf.d3dObj );
		if (FAILED(hr))
			Modules::log().writeError( "createBuffer failed" );
	}
	
	_bufferMem += size;
	return _buffers.add( buf );
}


void RenderDevice::destroyBuffer( uint32 bufObj )
{
	if( bufObj == 0 ) return;
	
	RDIBuffer &buf = _buffers.getRef( bufObj );
	SAFE_RELEASE(buf.d3dObj);

	_bufferMem -= buf.size;
	_buffers.remove( bufObj );
}


void RenderDevice::updateBufferData( uint32 bufObj, uint32 offset, uint32 size, void *data )
{
	const RDIBuffer &buf = _buffers.getRef( bufObj );
	ASSERT( offset + size <= buf.size );

	if (buf.d3dObj == NULL) return;

	if (buf.dynamic)
	{	//todo: use D3D11_MAP_WRITE_NO_OVERWRITE flags!! - only useable for full update now
		D3D11_MAPPED_SUBRESOURCE mappedRes;
		HRESULT hr = _d3dContext->Map(buf.d3dObj, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);
		if (SUCCEEDED(hr))
		{
			if (offset+size<=mappedRes.RowPitch)
				memcpy((int8*)mappedRes.pData+offset, data, size);
			else
				Modules::log().writeError( "updateBufferData: invalid offset/size parameters" );
			_d3dContext->Unmap(buf.d3dObj, 0);
		}
	}
	else
	{
		D3D11_BOX box = { offset, 0,0, 
						 offset + size,1,1};
		_d3dContext->UpdateSubresource(buf.d3dObj, 0, &box, data, 0,0);
	}
}


// =================================================================================================
// Textures
// =================================================================================================

uint32 RenderDevice::calcTextureSize( TextureFormats::List format, int width, int height, int depth )
{
	switch( format )
	{
	case TextureFormats::RGBA8:
		return width * height * depth * 4;
	case TextureFormats::DXT1:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 8;
	case TextureFormats::DXT3:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 16;
	case TextureFormats::DXT5:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 16;
	case TextureFormats::RGBA16F:
		return width * height * depth * 8;
	case TextureFormats::RGBA32F:
		return width * height * depth * 16;
	default:
		return 0;
	}
}

static int getMipLevels(int width, int height, int depth)
{
	int mipcount = 1;
	while(width>1 || height>1 || depth>1)
	{
		width >>= 1;
		height >>= 1;
		depth >>= 1;
		mipcount++;
	}
	return mipcount;
}

uint32 RenderDevice::createTexture( TextureTypes::List type, int width, int height, int depth,
                                    TextureFormats::List format,
                                    bool hasMips, bool genMips, bool sRGB )
{
	ASSERT( depth > 0 );

	if( !_caps.texNPOT )
	{
		// Check if texture is NPOT
		if( (width & (width-1)) != 0 || (height & (height-1)) != 0 )
			Modules::log().writeWarning( "Texture has non-power-of-two dimensions although NPOT is not supported by GPU" );
	}
	
	RDITexture tex;
	tex.type = type;
	tex.format = format;
	tex.width = width;
	tex.height = height;
	tex.depth = depth;
	tex.sRGB = sRGB && Modules::config().sRGBLinearization;
	tex.genMips = genMips;
	tex.hasMips = hasMips;
	tex.d3dResourceView = 0x0;

	static DXGI_FORMAT formats[] = {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_BC1_UNORM, 
			DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, _depthFormat };
	static DXGI_FORMAT srgbFormats[] = {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_BC1_UNORM_SRGB, 
			DXGI_FORMAT_BC2_UNORM_SRGB, DXGI_FORMAT_BC3_UNORM_SRGB, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, _depthFormat };

	ASSERT( format < sizeof(formats)/sizeof(DXGI_FORMAT) );	//static assert??

	tex.d3dFmt = tex.sRGB ? srgbFormats[format] : formats[format];
	uint32 mipCount = tex.genMips ? 0 : (tex.hasMips ? getMipLevels(width,height,depth) : 1);

	if ( type == TextureTypes::Tex2D || type == TextureTypes::TexCube )
	{
		tex.d3dTexture2D = 0x0;
		D3D11_TEXTURE2D_DESC desc = { width, height, mipCount, type == TextureTypes::TexCube?6:1, tex.d3dFmt, {1,0}, D3D11_USAGE_DEFAULT, 
			D3D11_BIND_SHADER_RESOURCE, 0, type==TextureTypes::TexCube?D3D11_RESOURCE_MISC_TEXTURECUBE : 0};
		HRESULT hr = _d3dDevice->CreateTexture2D(&desc, NULL, &tex.d3dTexture2D);
		if (SUCCEEDED(hr))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
			if (format == TextureFormats::DEPTH && _depthFormat == DXGI_FORMAT_R32_TYPELESS)
				viewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			else if (format == TextureFormats::DEPTH && _depthFormat == DXGI_FORMAT_R24G8_TYPELESS)
				viewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				viewDesc.Format = tex.d3dFmt;
			if ( type == TextureTypes::Tex2D )
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MostDetailedMip = 0;
				viewDesc.Texture2D.MipLevels = -1;
			}
			else
			{
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				viewDesc.TextureCube.MostDetailedMip = 0;
				viewDesc.TextureCube.MipLevels = -1;
			}
			HRESULT hr = _d3dDevice->CreateShaderResourceView(tex.d3dTexture2D, &viewDesc, &tex.d3dResourceView);
			if (FAILED(hr))
			{
				tex.d3dTexture2D->Release();
				tex.d3dTexture2D = 0x0;
			}
		}  
	}
	else
	{
		tex.d3dTexture3D = 0x0;
///*todo create 3d texture
	}

	// Calculate memory requirements
	tex.memSize = calcTextureSize( format, width, height, depth );
	if( hasMips || genMips ) tex.memSize += ftoi_r( tex.memSize * 1.0f / 3.0f );
	if( type == TextureTypes::TexCube ) tex.memSize *= 6;
	_textureMem += tex.memSize;
	
	return _textures.add( tex );
}


void RenderDevice::uploadTextureData( uint32 texObj, int slice, int mipLevel, const void *pixels )
{
	const RDITexture &tex = _textures.getRef( texObj );
	TextureFormats::List format = tex.format;

	// Calculate size of next mipmap using "floor" convention
	int mipLevels = (tex.hasMips||tex.genMips) ? getMipLevels(tex.width,tex.height,tex.depth) : 1;
	int width = std::max( tex.width >> mipLevel, 1 ), height = std::max( tex.height >> mipLevel, 1 );

	int srcRowPitch = calcTextureSize( format, width, height, 1 ) / height;
		//hack:
	if ( tex.format == TextureFormats::DXT1 || tex.format == TextureFormats::DXT3 ||tex.format == TextureFormats::DXT5 )
		srcRowPitch *= 4;

	if( tex.type == TextureTypes::Tex2D || tex.type == TextureTypes::TexCube )
	{
		_d3dContext->UpdateSubresource(tex.d3dTexture2D, D3D11CalcSubresource(mipLevel, slice, mipLevels), NULL, pixels,  srcRowPitch, 0 );
	}
	else if( tex.type == TextureTypes::Tex3D )
	{
		int depth = std::max( tex.depth >> mipLevel, 1 );
		
//*		if( compressed )
//*			glCompressedTexImage3D( GL_TEXTURE_3D, mipLevel, tex.glFmt, width, height, depth, 0,
//*			                        calcTextureSize( format, width, height, depth ), pixels );	
//*		else
//*			glTexImage3D( GL_TEXTURE_3D, mipLevel, tex.glFmt, width, height, depth, 0,
//*			              inputFormat, inputType, pixels );
	}

    if( tex.genMips && (tex.type != TextureTypes::TexCube || slice == 5) )
	{
		// Note: for cube maps mips are only generated when the side with the highest index is uploaded
		D3DX11FilterTexture( _d3dContext, tex.d3dTexture, 0, D3DX11_DEFAULT );
	}
}


void RenderDevice::destroyTexture( uint32 texObj )
{
	if( texObj == 0 ) return;
	
	RDITexture &tex = _textures.getRef( texObj );
	SAFE_RELEASE(tex.d3dTexture);
	SAFE_RELEASE(tex.d3dResourceView);

	_textureMem -= tex.memSize;
	_textures.remove( texObj );
}


void RenderDevice::updateTextureData( uint32 texObj, int slice, int mipLevel, const void *pixels )
{
	uploadTextureData( texObj, slice, mipLevel, pixels );
}


bool RenderDevice::getTextureData( uint32 texObj, int slice, int mipLevel, void *buffer )
{
	const RDITexture &tex = _textures.getRef( texObj );
	
//*	int target = tex.type == TextureTypes::TexCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
//*	if( target == GL_TEXTURE_CUBE_MAP ) target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice;
	
//*	int fmt, type, compressed = 0;
//*	glActiveTexture( GL_TEXTURE15 );
//*	glBindTexture( tex.type, tex.glObj );

	switch( tex.format )
	{
	case TextureFormats::RGBA8:
//*		fmt = GL_BGRA;
//*		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormats::DXT1:
	case TextureFormats::DXT3:
	case TextureFormats::DXT5:
//*		compressed = 1;
		break;
	case TextureFormats::RGBA16F:
	case TextureFormats::RGBA32F:
//*		fmt = GL_RGBA;
//*		type = GL_FLOAT;
		break;
	default:
		return false;
	};

//*	if( compressed )
//*		glGetCompressedTexImage( target, mipLevel, buffer );
//*	else
//*		glGetTexImage( target, mipLevel, fmt, type, buffer );

//*	glBindTexture( tex.type, 0 );
//*	if( _texSlots[15].texObj )
//*		glBindTexture( _textures.getRef( _texSlots[15].texObj ).type, _textures.getRef( _texSlots[15].texObj ).glObj );

	return true;
}


// =================================================================================================
// Shaders
// =================================================================================================

//#define VS_TARGET	"vs_4_0_level_9_3"
//#define PS_TARGET	"ps_4_0_level_9_3"
#define VS_TARGET	"vs_4_0"
#define PS_TARGET	"ps_4_0"

uint32 RenderDevice::createShader( const char *vertexShaderSrc, const char *fragmentShaderSrc )
{
	_shaderLog = "";

	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;

	// Vertex shader
	LPD3D10BLOB vsShaderBlob,errorBlob;
	HRESULT hr = D3DCompile(vertexShaderSrc, strlen(vertexShaderSrc), "", NULL, NULL, "main", VS_TARGET, D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsShaderBlob, &errorBlob); 

	if( FAILED(hr) || errorBlob!=NULL )
	{	
        if( errorBlob )
		{
			_shaderLog = _shaderLog + "[Vertex Shader]\n" + (char*)errorBlob->GetBufferPointer();
			errorBlob->Release();
		}
		return 0;
	}

    // Create the vertex shader
    hr = _d3dDevice->CreateVertexShader( vsShaderBlob->GetBufferPointer(), vsShaderBlob->GetBufferSize(), NULL, &vertexShader );
    if( FAILED( hr ) )
    {    
		_shaderLog = _shaderLog + "[Vertex Shader]\n Create Failed\n";
        vsShaderBlob->Release();
        return 0;
    }

		//pixel shader
	LPD3D10BLOB psShaderBlob;
	hr = D3DCompile(fragmentShaderSrc, strlen(fragmentShaderSrc), "", NULL, NULL, "main", PS_TARGET, D3D10_SHADER_ENABLE_STRICTNESS, 0, &psShaderBlob, &errorBlob); 

	if( FAILED(hr) || errorBlob!=NULL )
	{	
		vertexShader->Release();
        vsShaderBlob->Release();
        if( errorBlob )
		{
			_shaderLog = _shaderLog + "[Pixel Shader]\n" + (char*)errorBlob->GetBufferPointer();
			errorBlob->Release();
		}
		return 0;
	}

    // Create the pixel shader
    hr = _d3dDevice->CreatePixelShader( psShaderBlob->GetBufferPointer(), psShaderBlob->GetBufferSize(), NULL, &pixelShader );
    if( FAILED( hr ) )
    {    
		_shaderLog = _shaderLog + "[Pixel Shader]\n Create Failed\n";
		vertexShader->Release();
        vsShaderBlob->Release();
        psShaderBlob->Release();
        return 0;
    }

	uint32 shaderId = _shaders.add( RDIShader() );
	RDIShader &shader = _shaders.getRef( shaderId );
	shader.vertexShader = vertexShader;
	shader.pixelShader = pixelShader;
	for( uint32 i = 0; i < MaxNumVertexLayouts; ++i )
		shader.inputLayouts[i] = NULL;
	for( uint32 i = 0; i < RDIShaderType::Count; ++i)
	{
		shader.globalCBuffers[i].bufferData = NULL;
		shader.globalCBuffers[i].bufferId = 0;
		shader.globalCBuffers[i].bindPoint = -1;
	}

	parseShaderBlob( shaderId, vsShaderBlob, RDIShaderType::Vertex );
	vsShaderBlob->Release();

	parseShaderBlob( shaderId, psShaderBlob, RDIShaderType::Pixel );
	psShaderBlob->Release();

	return shaderId;
}

void RenderDevice::parseShaderBlob(uint32 shaderId, LPD3D10BLOB blob, RDIShaderType::List shaderType)
{
	if( shaderId == 0 ) return;

	RDIShader &shader = _shaders.getRef( shaderId );

		// get vertex shader uniforms and inputlayout
	ID3D11ShaderReflection* reflector = NULL; 
	D3DReflect( blob->GetBufferPointer(), blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &reflector);
	D3D11_SHADER_DESC shaderDesc;
	reflector->GetDesc(&shaderDesc);

		//input layout - for vertex shader only
	if ( shaderType == RDIShaderType::Vertex )
	{	

		struct AttribRemap
		{
			const char* attrib;
			const char* semanticName;
			uint32		semanticIndex;
		};
		static AttribRemap attribRemaps[] = {
			{ "vertPos", "POSITION", 0},
			{ "texCoords0", "TEXCOORD", 0},
			{ "texCoords1", "TEXCOORD", 1},
			{ "normal", "NORMAL", 0},
			{ "tangent", "TANGENT", 0},
			{ "joints", "BLENDINDICES", 0},
			{ "weights", "BLENDWEIGHTS", 0},
			{ "parIdx", "BLENDINDICES", 0},
		};
		static DXGI_FORMAT formats[4] = {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, 
										DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT};

		for( uint32 i = 0; i < _numVertexLayouts; ++i )
		{
			RDIVertexLayout &vl = _vertexLayouts[i];
			D3D11_INPUT_ELEMENT_DESC layout[32];
			UINT numElements = 0;

			for( uint32 j = 0; j < vl.numAttribs; ++j )
			{
				for( uint32 k = 0; k < sizeof(attribRemaps) / sizeof(AttribRemap); ++k)
					if ( vl.attribs[j].semanticName.compare(attribRemaps[k].attrib) == 0 )
					{
						D3D11_INPUT_ELEMENT_DESC d = {attribRemaps[k].semanticName, attribRemaps[k].semanticIndex, formats[vl.attribs[j].size-1], vl.attribs[j].vbSlot, vl.attribs[j].offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
						layout[numElements++] = d;
						break;
					}
			}

				//check if all parameters is in vertexlayout - do we need this? createinput gives error if not found...
			bool allParametersFound = true;

			for( uint32 j = 0; j < shaderDesc.InputParameters; ++j )
			{
				bool parameterFound = false;
				D3D11_SIGNATURE_PARAMETER_DESC inputDesc;
				reflector->GetInputParameterDesc(j, &inputDesc);

				for( uint32 k = 0; k < numElements; ++k )
				{
					if( !strcmp(layout[k].SemanticName, inputDesc.SemanticName) && 
						layout[k].SemanticIndex == inputDesc.SemanticIndex)
					{
						parameterFound = true;
						break;
					}
				}

				if( !parameterFound )
				{
					allParametersFound = false;
					break;
				}
			}

			if ( allParametersFound )
			{
				_d3dDevice->CreateInputLayout( layout, numElements, blob->GetBufferPointer(),
											blob->GetBufferSize(), &shader.inputLayouts[i] );
			}
		}
	}

	///* todo: parse uniformbuffers - store $globals to uniforms others to buffers
	if (shaderDesc.ConstantBuffers > 0)
	{	//only using first constant buffer now
		ID3D11ShaderReflectionConstantBuffer* buffer = reflector->GetConstantBufferByName( "$Globals" );
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		HRESULT hr = buffer->GetDesc(&bufferDesc);
		if (SUCCEEDED(hr))
		{
			if (bufferDesc.Size > 0)
			{
				shader.globalCBuffers[shaderType].bufferId = createUniformBuffer( bufferDesc.Size, NULL );
				shader.globalCBuffers[shaderType].bufferData = new int8[ bufferDesc.Size ];
			}

			// create a memory version with size??
			for( uint32 i=0; i<bufferDesc.Variables; ++i)
			{
				ID3D11ShaderReflectionVariable* variable = buffer->GetVariableByIndex(i);
				D3D11_SHADER_VARIABLE_DESC variableDesc;
				hr = variable->GetDesc(&variableDesc);
				if (SUCCEEDED(hr) &&  (variableDesc.uFlags&D3D_SVF_USED)!=0)
				{
					int index = -1;
					for( uint32 j=0; j<shader.uniforms.size(); ++j)
					{
						if (shader.uniforms[j].name == variableDesc.Name )
						{
							index = (int)j;
							break;
						}
					}

					if (index==-1)
					{
						RDIUniform uniform;
						uniform.name = variableDesc.Name;
						index = (int)shader.uniforms.size();
						shader.uniforms.push_back(uniform);
					}

					shader.uniforms[index].desc[shaderType].offset = variableDesc.StartOffset;
					shader.uniforms[index].desc[shaderType].size = variableDesc.Size;

				}
			}
		}
		else
			_shaderLog = _shaderLog + "[Shader]\n Failed to get constant buffer desc\n";
	}

	for ( unsigned int i = 0; i < shaderDesc.BoundResources; ++i)
	{
		D3D11_SHADER_INPUT_BIND_DESC desc;
		reflector->GetResourceBindingDesc(i, &desc);

		if ( !strcmp(desc.Name,"$Globals") && desc.Type == D3D_SIT_CBUFFER )
			shader.globalCBuffers[shaderType].bindPoint = desc.BindPoint;
		else if ( desc.Type == D3D_SIT_TEXTURE ) 
		{
			if (strlen(desc.Name)>8 && strncmp(desc.Name,"texture_",8)==0)
			{
				int idx = -1;
				for( uint32 i = 0; i < shader.samplers.size(); ++i )
				{
					if (shader.samplers[i].name == (desc.Name + 8) )
					{
						idx = (int)i;
						break;
					}
				}

				if (idx==-1)
				{	//not found, add new
					RDISampler sampler;
					sampler.name = desc.Name + 8;
					sampler.slot = 0;
					idx = (int) shader.samplers.size();
					shader.samplers.push_back(sampler);
				}
				shader.samplers[idx].desc[shaderType].textureBindPoint = desc.BindPoint;
			}
			else
				_shaderLog = _shaderLog + "[Shader]\n Texture resource's name must have a texture_ prefix\n";
		}
		else if ( desc.Type == D3D_SIT_SAMPLER ) 
		{
			if (strlen(desc.Name)>8 && strncmp(desc.Name,"sampler_",8)==0)
			{
				int idx = -1;
				for( uint32 i = 0; i < shader.samplers.size(); ++i )
				{
					if (shader.samplers[i].name == (desc.Name + 8) )
					{
						idx = (int)i;
						break;
					}
				}

				if (idx==-1)
				{	//not found, add new
					RDISampler sampler;
					sampler.name = desc.Name + 8;
					sampler.slot = 0;
					idx = (int) shader.samplers.size();
					shader.samplers.push_back(sampler);
				}
				shader.samplers[idx].desc[shaderType].samplerBindPoint = desc.BindPoint;
			}
			else
				_shaderLog = _shaderLog + "[Shader]\n Texture resource's name must have a texture_ prefix\n";
		}
	}
}



void RenderDevice::destroyShader( uint32 shaderId )
{
	if( shaderId == 0 ) return;

	RDIShader &shader = _shaders.getRef( shaderId );
	SAFE_RELEASE( shader.vertexShader );
	for( uint32 i = 0; i < MaxNumVertexLayouts; ++i )
		SAFE_RELEASE( shader.inputLayouts[i] );
	SAFE_RELEASE( shader.pixelShader );
	for( uint32 i=0; i<RDIShaderType::Count; ++i)
	{
		destroyBuffer( shader.globalCBuffers[i].bufferId );
		delete[] shader.globalCBuffers[i].bufferData;
	}

	_shaders.remove( shaderId );
}


void RenderDevice::bindShader( uint32 shaderId )
{
	if( shaderId != 0 )
	{
		RDIShader &shader = _shaders.getRef( shaderId );
		_d3dContext->VSSetShader( shader.vertexShader, NULL, 0 );
		_d3dContext->PSSetShader( shader.pixelShader, NULL, 0 );
	}
	else
	{
		_d3dContext->VSSetShader( NULL, NULL, 0 );
		_d3dContext->PSSetShader( NULL, NULL, 0 );
	}
	
	_curShaderId = shaderId;
	_pendingMask |= PM_VERTLAYOUT;
} 


int RenderDevice::getShaderConstLoc( uint32 shaderId, const char *name )
{
	RDIShader &shader = _shaders.getRef( shaderId );

	for( uint32 i = 0, s = (uint32)shader.uniforms.size(); i < s; ++i )
		if ( shader.uniforms[i].name == name || shader.uniforms[i].name+"[0]" == name)
			return i;

	return -1;
}


int RenderDevice::getShaderSamplerLoc( uint32 shaderId, const char *name )
{
	RDIShader &shader = _shaders.getRef( shaderId );

	for( uint32 i = 0, s = (uint32)shader.samplers.size(); i < s; ++i )
		if ( shader.samplers[i].name == name )
			return i;

	return -1;
}


void RenderDevice::setShaderConst( int loc, RDIShaderConstType type, void *values, uint32 count )
{
	if ( _curShaderId == 0 )
		return;

	ASSERT(type<=CONST_FLOAT33);
	static const uint32 size[CONST_FLOAT33+1] = { sizeof(float), sizeof(float)*2, sizeof(float)*3, 
										sizeof(float)*4, sizeof(float)*16, sizeof(float)*11};
	uint32 sizeType = size[type];
	
	RDIShader &shader = _shaders.getRef( _curShaderId );
	RDIUniform &uniform = shader.uniforms[ loc ];

		// padding hack - todo a more general solution and optimize this
	if ( type == CONST_FLOAT33 )
	{
		float temp[11] = {((float*)values)[0], ((float*)values)[1], ((float*)values)[2], 0.0f,
						((float*)values)[3], ((float*)values)[4], ((float*)values)[5], 0.0f,
						((float*)values)[6], ((float*)values)[7], ((float*)values)[8]};
		values = temp;
	} else if ( type == CONST_FLOAT3 && count > 1)
	{
		unsigned char* mappedData = Modules::renderer().useScratchBuf( 4 * sizeof(float) * count );
		for(uint32 i = 0; i< count; ++i)
			memcpy( mappedData + i * 4 * sizeof(float), (char*)values + i * 3 * sizeof(float), 3 * sizeof(float) ); 
		values = mappedData;
		sizeType = sizeof(float) * 4;
	} else if ( type == CONST_FLOAT2 && count > 1)
	{
		unsigned char* mappedData = Modules::renderer().useScratchBuf( 4 * sizeof(float) * count );
		for(uint32 i = 0; i< count; ++i)
			memcpy( mappedData + i * 4 * sizeof(float), (char*)values + i * 2 * sizeof(float), 2 * sizeof(float) ); 
		values = mappedData;
		sizeType = sizeof(float) * 4;
	}

	for( unsigned int i=0; i<RDIShaderType::Count; ++i)
	{
		if ( uniform.desc[i].size > 0)
			memcpy( shader.globalCBuffers[i].bufferData + uniform.desc[i].offset, values, std::min(count*sizeType, uniform.desc[i].size) );
	}
}


void RenderDevice::setShaderSampler( int loc, uint32 texUnit )
{
	if ( _curShaderId == 0 || loc < 0)
		return;

	RDIShader &shader = _shaders.getRef( _curShaderId );
	shader.samplers[loc].slot = texUnit;
}


const char *RenderDevice::getDefaultVSCode()
{
	return defaultShaderVS;
}


const char *RenderDevice::getDefaultFSCode()
{
	return defaultShaderFS;
}


// =================================================================================================
// Renderbuffers
// =================================================================================================

uint32 RenderDevice::createRenderBuffer( uint32 width, uint32 height, TextureFormats::List format,
                                         bool depth, uint32 numColBufs, uint32 samples )
{
	if( (format == TextureFormats::RGBA16F || format == TextureFormats::RGBA32F) && !_caps.texFloat )
	{
		return 0;
	}

	if( numColBufs > RDIRenderBuffer::MaxColorAttachmentCount ) return 0;

	uint32 maxSamples = 0;
	if( _caps.rtMultisampling )
	{
//*		GLint value;
//*		glGetIntegerv( GL_MAX_SAMPLES_EXT, &value );
//*		maxSamples = (uint32)value;
	}
	if( samples > maxSamples )
	{
		samples = maxSamples;
		Modules::log().writeWarning( "GPU does not support desired multisampling quality for render target" );
	}

	RDIRenderBuffer rb;
	rb.width = width;
	rb.height = height;
	rb.samples = samples;

	// Create framebuffers
//*	glGenFramebuffersEXT( 1, &rb.fbo );
//*	if( samples > 0 ) glGenFramebuffersEXT( 1, &rb.fboMS );
//*
//*	if( numColBufs > 0 )
//*	{
//*		// Attach color buffers
//*		for( uint32 j = 0; j < numColBufs; ++j )
//*		{
//*			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*			// Create a color texture
//*			uint32 texObj = createTexture( TextureTypes::Tex2D, rb.width, rb.height, 1, format, false, false, false, false );
//*			ASSERT( texObj != 0 );
//*			uploadTextureData( texObj, 0, 0, 0x0 );
//*			rb.colTexs[j] = texObj;
//*			RDITexture &tex = _textures.getRef( texObj );
//*			// Attach the texture
//*			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j, GL_TEXTURE_2D, tex.glObj, 0 );
//*
//*			if( samples > 0 )
//*			{
//*				glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
//*				// Create a multisampled renderbuffer
//*				glGenRenderbuffersEXT( 1, &rb.colBufs[j] );
//*				glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, rb.colBufs[j] );
//*				glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, rb.samples, tex.glFmt, rb.width, rb.height );
//*				// Attach the renderbuffer
//*				glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j,
//*				                              GL_RENDERBUFFER_EXT, rb.colBufs[j] );
//*			}
//*		}
//*
//*		uint32 buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT,
//*		                     GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT };
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*		glDrawBuffers( numColBufs, buffers );
//*		
//*		if( samples > 0 )
//*		{
//*			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
//*			glDrawBuffers( numColBufs, buffers );
//*		}
//*	}
//*	else
//*	{	
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*		glDrawBuffer( GL_NONE );
//*		glReadBuffer( GL_NONE );
//*		
//*		if( samples > 0 )
//*		{
//*			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
//*			glDrawBuffer( GL_NONE );
//*			glReadBuffer( GL_NONE );
//*		}
//*	}

	// Attach depth buffer
//*	if( depth )
//*	{
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*		// Create a depth texture
//*		uint32 texObj = createTexture( TextureTypes::Tex2D, rb.width, rb.height, 1, TextureFormats::DEPTH, false, false, false, false );
//*		ASSERT( texObj != 0 );
//*		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
//*		uploadTextureData( texObj, 0, 0, 0x0 );
//*		rb.depthTex = texObj;
//*		RDITexture &tex = _textures.getRef( texObj );
//*		// Attach the texture
//*		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, tex.glObj, 0 );
//*
//*		if( samples > 0 )
//*		{
//*			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
//*			// Create a multisampled renderbuffer
//*			glGenRenderbuffersEXT( 1, &rb.depthBuf );
//*			glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, rb.depthBuf );
//*			glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, rb.samples, _depthFormat, rb.width, rb.height );
//*			// Attach the renderbuffer
//*			glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
//*			                              GL_RENDERBUFFER_EXT, rb.depthBuf );
//*		}
//*	}

	uint32 rbObj = _rendBufs.add( rb );
	
	// Check if FBO is complete
	bool valid = true;
//*	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*	uint32 status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
//*	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _defaultFBO );
//*	if( status != GL_FRAMEBUFFER_COMPLETE_EXT ) valid = false;
	
//*	if( samples > 0 )
//*	{
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
//*		status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _defaultFBO );
//*		if( status != GL_FRAMEBUFFER_COMPLETE_EXT ) valid = false;
//*	}

	if( !valid )
	{
		destroyRenderBuffer( rbObj );
		return 0;
	}
	
	return rbObj;
}


void RenderDevice::destroyRenderBuffer( uint32 rbObj )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
//*	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _defaultFBO );
	
	if( rb.depthTex != 0 ) destroyTexture( rb.depthTex );
//*	if( rb.depthBuf != 0 ) glDeleteRenderbuffersEXT( 1, &rb.depthBuf );
	rb.depthTex = rb.depthBuf = 0;
		
	for( uint32 i = 0; i < RDIRenderBuffer::MaxColorAttachmentCount; ++i )
	{
		if( rb.colTexs[i] != 0 ) destroyTexture( rb.colTexs[i] );
//*		if( rb.colBufs[i] != 0 ) glDeleteRenderbuffersEXT( 1, &rb.colBufs[i] );
		rb.colTexs[i] = rb.colBufs[i] = 0;
	}

//*	if( rb.fbo != 0 ) glDeleteFramebuffersEXT( 1, &rb.fbo );
//*	if( rb.fboMS != 0 ) glDeleteFramebuffersEXT( 1, &rb.fboMS );
	rb.fbo = rb.fboMS = 0;

	_rendBufs.remove( rbObj );
}


uint32 RenderDevice::getRenderBufferTex( uint32 rbObj, uint32 bufIndex )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
	if( bufIndex < RDIRenderBuffer::MaxColorAttachmentCount ) return rb.colTexs[bufIndex];
	else if( bufIndex == 32 ) return rb.depthTex;
	else return 0;
}


void RenderDevice::resolveRenderBuffer( uint32 rbObj )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
	if( rb.fboMS == 0 ) return;
	
//*	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, rb.fboMS );
//*	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, rb.fbo );

	bool depthResolved = false;
	for( uint32 i = 0; i < RDIRenderBuffer::MaxColorAttachmentCount; ++i )
	{
		if( rb.colBufs[i] != 0 )
		{
//*			glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
//*			glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
			
//*			int mask = GL_COLOR_BUFFER_BIT;
			if( !depthResolved && rb.depthBuf != 0 )
			{
//*				mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
				depthResolved = true;
			}
//*			glBlitFramebufferEXT( 0, 0, rb.width, rb.height, 0, 0, rb.width, rb.height, mask, GL_NEAREST );
		}
	}

	if( !depthResolved && rb.depthBuf != 0 )
	{
//*		glReadBuffer( GL_NONE );
//*		glDrawBuffer( GL_NONE );
//*		glBlitFramebufferEXT( 0, 0, rb.width, rb.height, 0, 0, rb.width, rb.height,
//*							  GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST );
	}

//*	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, _defaultFBO );
//*	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, _defaultFBO );
}


void RenderDevice::setRenderBuffer( uint32 rbObj )
{
	// Resolve render buffer if necessary
	if( _curRendBuf != 0 ) resolveRenderBuffer( _curRendBuf );
	
	// Set new render buffer
	_curRendBuf = rbObj;
	
	if( rbObj == 0 )
	{
		_d3dContext->OMSetRenderTargets(RDIRenderBuffer::MaxColorAttachmentCount, 
										_defaultRenderTargetViews, _defaultDepthStencilView);
		_fbWidth = _vpWidth + _vpX;
		_fbHeight = _vpHeight + _vpY;
	}
	else
	{
		// Unbind all textures to make sure that no FBO attachment is bound any more
		for( uint32 i = 0; i < 16; ++i ) setTexture( i, 0, 0 );
		commitStates( PM_TEXTURES );
		
		RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );

		//* TODO: hack now
		_d3dContext->OMSetRenderTargets(0,NULL,NULL);

//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS != 0 ? rb.fboMS : rb.fbo );
//		ASSERT( glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT ) == GL_FRAMEBUFFER_COMPLETE_EXT );
		_fbWidth = rb.width;
		_fbHeight = rb.height;

//*		if( rb.fboMS != 0 ) glEnable( GL_MULTISAMPLE );
//*		else glDisable( GL_MULTISAMPLE );
	}
}

void RenderDevice::getRenderBufferSize( uint32 rbObj, int *width, int *height )
{
	if( rbObj == 0 )
	{
		if( width != 0x0 ) *width = _vpWidth;
		if( height != 0x0 ) *height = _vpHeight;
	}
	else
	{
		RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
		
		if( width != 0x0 ) *width = rb.width;
		if( height != 0x0 ) *height = rb.height;
	}
}

bool RenderDevice::getRenderBufferData( uint32 rbObj, int bufIndex, int *width, int *height,
                                        int *compCount, void *dataBuffer, int bufferSize )
{
	int x, y, w, h;
//*	int format = GL_RGBA;
//*	int type = GL_FLOAT;
	beginRendering();
//*	glPixelStorei( GL_PACK_ALIGNMENT, 4 );
	
	if( rbObj == 0 )
	{
		if( bufIndex != 32 && bufIndex != 0 ) return false;
		if( width != 0x0 ) *width = _vpWidth;
		if( height != 0x0 ) *height = _vpHeight;
		
		x = _vpX; y = _vpY; w = _vpWidth; h = _vpHeight;

//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _defaultFBO );
//*		if( bufIndex != 32 ) glReadBuffer( GL_BACK_LEFT );
		//format = GL_BGRA;
		//type = GL_UNSIGNED_BYTE;
	}
	else
	{
		resolveRenderBuffer( rbObj );
		RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
		
		if( bufIndex == 32 && rb.depthTex == 0 ) return false;
		if( bufIndex != 32 )
		{
			if( (unsigned)bufIndex >= RDIRenderBuffer::MaxColorAttachmentCount || rb.colTexs[bufIndex] == 0 )
				return false;
		}
		if( width != 0x0 ) *width = rb.width;
		if( height != 0x0 ) *height = rb.height;

		x = 0; y = 0; w = rb.width; h = rb.height;
		
//*		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
//*		if( bufIndex != 32 ) glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + bufIndex );
	}

	if( bufIndex == 32 )
	{	
//*		format = GL_DEPTH_COMPONENT;
//*		type = GL_FLOAT;
	}
	
	int comps = (bufIndex == 32 ? 1 : 4);
	if( compCount != 0x0 ) *compCount = comps;
	
	bool retVal = false;
//*	if( dataBuffer != 0x0 &&
//*	    bufferSize >= w * h * comps * (type == GL_FLOAT ? 4 : 1) ) 
//*	{
//*		glFinish();
//*		glReadPixels( x, y, w, h, format, type, dataBuffer );
//*		retVal = true;
//*	}
//*	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _defaultFBO );

	return retVal;
}


// =================================================================================================
// Queries
// =================================================================================================

uint32 RenderDevice::createOcclusionQuery()
{
	uint32 queryObj = 0;
//*	glGenQueries( 1, &queryObj );
	return queryObj;
}


void RenderDevice::destroyQuery( uint32 queryObj )
{
	if( queryObj == 0 ) return;
	
//*	glDeleteQueries( 1, &queryObj );
}


void RenderDevice::beginQuery( uint32 queryObj )
{
//*	glBeginQuery( GL_SAMPLES_PASSED, queryObj );
}


void RenderDevice::endQuery( uint32 /*queryObj*/ )
{
//*	glEndQuery( GL_SAMPLES_PASSED );
}


uint32 RenderDevice::getQueryResult( uint32 queryObj )
{
	uint32 samples = 0;
//*	glGetQueryObjectuiv( queryObj, GL_QUERY_RESULT, &samples );
	return samples;
}


// =================================================================================================
// Internal state management
// =================================================================================================

bool RenderDevice::applyVertexLayout()
{
	uint32 newVertexAttribMask = 0;
	
	if( _newVertLayout != 0 )
	{
		if( _curShaderId == 0 ) return false;
		
		RDIVertexLayout &vl = _vertexLayouts[_newVertLayout - 1];
		RDIShader &shader = _shaders.getRef( _curShaderId );
		ID3D11InputLayout* inputLayout = shader.inputLayouts[_newVertLayout - 1];
		
		if( inputLayout == 0x0)
			return false;

		_d3dContext->IASetInputLayout( inputLayout );
		
		// Set vertex attrib pointers
		for( uint32 i = 0; i < vl.numAttribs; ++i )
		{
			int buffer = vl.attribs[i].vbSlot;
			if ( (newVertexAttribMask & (1 << buffer)) == 0 )
			{
				const RDIVertBufSlot &vbSlot = _vertBufSlots[buffer];
				_d3dContext->IASetVertexBuffers(buffer, 1, &_buffers.getRef( vbSlot.vbObj ).d3dObj, &vbSlot.stride, &vbSlot.offset);

				newVertexAttribMask |= (1 << buffer);
			}
		}
	}

	uint32 disableMask = ~newVertexAttribMask & _activeVertexAttribsMask;
	if ( disableMask != 0 )
	{
		ID3D11Buffer* buffer = NULL;
		uint32 p = 0;
		for( uint32 i = 0; i < 16; ++i )
		{
			if( disableMask & (1 << i) )
				_d3dContext->IASetVertexBuffers( i, 1, &buffer, &p, &p );
		}
	}
	_activeVertexAttribsMask = newVertexAttribMask;

	return true;
}


ID3D11SamplerState* RenderDevice::getSamplerState( uint32 state )
{
	state = state & RDISamplerStateMask;
	if ( _samplerStates[ state ] != NULL )
		return _samplerStates[ state ];

	const D3D11_FILTER  filters[] = { D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_POINT };
	const D3D11_FILTER  filtersComp[] = {D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT};
	const uint32 maxAniso[] = { 1, 2, 4, 8, 16, 1, 1, 1 };
	const D3D11_TEXTURE_ADDRESS_MODE addressModes[] = { D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER };

	D3D11_SAMPLER_DESC desc;
	if ( !(state & SS_COMP_LEQUAL) )
	{
		if ( (state & SS_ANISO_MASK) != SS_ANISO1 )
			desc.Filter  = D3D11_FILTER_ANISOTROPIC;
		else
			desc.Filter = filters[(state & SS_FILTER_MASK) >> SS_FILTER_START];
	}
	else
	{
		if ( (state & SS_ANISO_MASK) != SS_ANISO1 )
			desc.Filter  = D3D11_FILTER_COMPARISON_ANISOTROPIC;
		else
			desc.Filter = filtersComp[(state & SS_FILTER_MASK) >> SS_FILTER_START];
	}
	desc.AddressU = addressModes[(state & SS_ADDRU_MASK) >> SS_ADDRU_START];
	desc.AddressV = addressModes[(state & SS_ADDRV_MASK) >> SS_ADDRV_START];
	desc.AddressW = addressModes[(state & SS_ADDRW_MASK) >> SS_ADDRW_START];
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = maxAniso[(state & SS_ANISO_MASK) >> SS_ANISO_START];
	if( !(state & SS_COMP_LEQUAL) )
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	else
		desc.ComparisonFunc = (state & SS_COMP_LEQUAL)!=0 ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_NEVER;
	for(int i=0;i<4;++i) 
		desc.BorderColor[i] = 1.0f;
	desc.MinLOD = -FLT_MAX;
	desc.MaxLOD = FLT_MAX;

	HRESULT hres = _d3dDevice->CreateSamplerState(&desc, &_samplerStates[ state ]);
	if (FAILED(hres))
		Modules::log().writeError( "CreateSamplerState failed with flag:%d", state );

	return _samplerStates[ state ];
}


void RenderDevice::applyRenderStates()
{
	// Rasterizer state
	if( _newRasterState.hash != _curRasterState.hash )
	{
		int hash = _newRasterState.hash & RDIRasterState::stateMask;
		if ( _rasterizerStates[ hash ] == NULL )
		{
			D3D11_RASTERIZER_DESC desc = {D3D11_FILL_SOLID, D3D11_CULL_NONE , TRUE, 0, 0.0f, 0.0f, TRUE, FALSE, FALSE, FALSE};
 
			if ( _newRasterState.fillMode == RS_FILL_WIREFRAME ) desc.FillMode = D3D11_FILL_WIREFRAME;

			if ( _newRasterState.cullMode == RS_CULL_BACK ) 
				desc.CullMode = D3D11_CULL_BACK;
			else if ( _newRasterState.cullMode == RS_CULL_FRONT ) 
				desc.CullMode = D3D11_CULL_FRONT;

			if( _newRasterState.scissorEnable ) desc.ScissorEnable = TRUE;

			_d3dDevice->CreateRasterizerState( &desc,  &_rasterizerStates[ hash ] );
		}

		_d3dContext->RSSetState( _rasterizerStates[ hash ] );

		_curRasterState.hash = _newRasterState.hash;
	}

	// Blend state
	if( _newBlendState.hash != _curBlendState.hash )
	{
		int hash = _newBlendState.hash & RDIBlendState::stateMask;
		if ( _blendStates[ hash ] == NULL )
		{
			D3D11_BLEND_DESC desc = { FALSE, FALSE, FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
									D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL};

			if( _newBlendState.alphaToCoverageEnable ) desc.AlphaToCoverageEnable = TRUE;
			
			if( _newBlendState.blendEnable )
			{
				desc.RenderTarget[0].BlendEnable = TRUE;

				D3D11_BLEND d3dBlendFuncs[8] = { D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, 
					D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO };

				desc.RenderTarget[0].SrcBlend = d3dBlendFuncs[_newBlendState.srcBlendFunc];
				desc.RenderTarget[0].SrcBlendAlpha = d3dBlendFuncs[_newBlendState.srcBlendFunc];
				desc.RenderTarget[0].DestBlend = d3dBlendFuncs[_newBlendState.destBlendFunc];
				desc.RenderTarget[0].DestBlendAlpha = d3dBlendFuncs[_newBlendState.destBlendFunc];
			}

			if( !_newBlendState.renderTargetWriteMask ) desc.RenderTarget[0].RenderTargetWriteMask = 0;

			_d3dDevice->CreateBlendState( &desc,  &_blendStates[ hash ] );
		}
		_d3dContext->OMSetBlendState( _blendStates[ hash ], NULL, 0xffffffff );
		
		_curBlendState.hash = _newBlendState.hash;
	}

	// Depth-stencil state
	if( _newDepthStencilState.hash != _curDepthStencilState.hash )
	{
		int hash = _newDepthStencilState.hash & RDIDepthStencilState::stateMask;
		if ( _depthStencilStates[ hash ] == NULL )
		{
			D3D11_DEPTH_STENCIL_DESC desc = { FALSE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, 
						FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
						D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS,
						D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};

			if( !_newDepthStencilState.depthWriteMask ) desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

			if( _newDepthStencilState.depthEnable )
			{
				D3D11_COMPARISON_FUNC d3dDepthFuncs[8] = { D3D11_COMPARISON_LESS_EQUAL, D3D11_COMPARISON_LESS, D3D11_COMPARISON_EQUAL, 
					D3D11_COMPARISON_GREATER, D3D11_COMPARISON_GREATER_EQUAL, D3D11_COMPARISON_ALWAYS, D3D11_COMPARISON_ALWAYS, D3D11_COMPARISON_ALWAYS };
			
				desc.DepthEnable = TRUE;
				desc.DepthFunc = d3dDepthFuncs[ _newDepthStencilState.depthFunc ];
			}

			_d3dDevice->CreateDepthStencilState( &desc,  &_depthStencilStates[hash] );
		}

		_d3dContext->OMSetDepthStencilState( _depthStencilStates[hash], 0 );
		
		_curDepthStencilState.hash = _newDepthStencilState.hash;
	}
}


bool RenderDevice::commitStates( uint32 filter )
{
	if( _pendingMask & filter )
	{
		uint32 mask = _pendingMask & filter;
	
		// Set viewport
		if( mask & PM_VIEWPORT )
		{
			D3D11_VIEWPORT vp = { (float)_vpX, (float)_vpY, (float)_vpWidth, (float)_vpHeight, 0.0f, 1.0f };
			_d3dContext->RSSetViewports(1, &vp);
			_pendingMask &= ~PM_VIEWPORT;
		}

		if( mask & PM_RENDERSTATES )
		{
			applyRenderStates();
			_pendingMask &= ~PM_RENDERSTATES;
		}

		// Set scissor rect
		if( mask & PM_SCISSOR )
		{
			D3D11_RECT rect = { _scX, _scY, _scX + _scWidth, _scY + _scHeight };
			_d3dContext->RSSetScissorRects( 1, &rect );
			_pendingMask &= ~PM_SCISSOR;
		}
		
		// Bind index buffer
		if( mask & PM_INDEXBUF )
		{
			if( _newIndexBuf != _curIndexBuf )
			{
				if( _newIndexBuf != 0 )
					_d3dContext->IASetIndexBuffer( _buffers.getRef( _newIndexBuf ).d3dObj, (DXGI_FORMAT)_indexFormat, 0);
				else
					_d3dContext->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0);

				_curIndexBuf = _newIndexBuf;
				_pendingMask &= ~PM_INDEXBUF;
			}
		}

		// Bind textures and set sampler state
		if( mask & PM_TEXTURES )
		{
			if ( _curShaderId > 0 )
			{
				RDIShader &shader = _shaders.getRef( _curShaderId );

					//set shader textures/samplers
				for ( uint32 i = 0; i < shader.samplers.size(); ++i )
				{
					RDISampler &sampler = shader.samplers[i];
					if ( sampler.slot >= 0 && _texSlots[sampler.slot].texObj )
					{
						RDITexture &tex = _textures.getRef( _texSlots[sampler.slot].texObj );
						ID3D11SamplerState* s = getSamplerState( _texSlots[sampler.slot].samplerState );

						///todo: set vertex shader textures/samplers
						RDISampler::Desc& desc = sampler.desc[RDIShaderType::Pixel];
						_d3dContext->PSSetShaderResources(desc.textureBindPoint,1, &tex.d3dResourceView);
						_d3dContext->PSSetSamplers(desc.samplerBindPoint,1, &s);
					}
				}

				///*TODO: unset not used samplers/shaderresources - maybe only in finishRendering
			}
			
			_pendingMask &= ~PM_TEXTURES;
		}

		// Bind vertex buffers
		if( mask & PM_VERTLAYOUT )
		{
			//if( _newVertLayout != _curVertLayout || _curShader != _prevShader )
			{
				if( !applyVertexLayout() )
					return false;
				_curVertLayout = _newVertLayout;
				_prevShaderId = _curShaderId;
				_pendingMask &= ~PM_VERTLAYOUT;
			}
		}

		// upload shader constants
		if ( _curShaderId > 0 )
		{
			RDIShader &shader = _shaders.getRef( _curShaderId );

				//TODO: do this in loop (put SET functions into an array??)
			RDIShader::GlobalCBuffer& vertexCBuffer = shader.globalCBuffers[RDIShaderType::Vertex];
			if ( vertexCBuffer.bindPoint >= 0 )
			{
				RDIBuffer& buffer = _buffers.getRef( vertexCBuffer.bufferId );
				updateBufferData(vertexCBuffer.bufferId, 0, buffer.size, vertexCBuffer.bufferData);
				_d3dContext->VSSetConstantBuffers( vertexCBuffer.bindPoint, 1, &buffer.d3dObj ); 
			}

			RDIShader::GlobalCBuffer& pixelCBuffer = shader.globalCBuffers[RDIShaderType::Pixel];
			if ( pixelCBuffer.bindPoint >= 0 )
			{
				RDIBuffer& buffer = _buffers.getRef( pixelCBuffer.bufferId );
				updateBufferData(pixelCBuffer.bufferId, 0, buffer.size, pixelCBuffer.bufferData);
				_d3dContext->PSSetConstantBuffers( pixelCBuffer.bindPoint, 1, &buffer.d3dObj ); 
			}
		}
	}

	return true;
}


void RenderDevice::resetStates()
{
	_curIndexBuf = 1; _newIndexBuf = 0;
	_curVertLayout = 1; _newVertLayout = 0;
	_curRasterState.hash = 0xFFFFFFFF; _newRasterState.hash = 0;
	_curBlendState.hash = 0xFFFFFFFF; _newBlendState.hash = 0;
	_curDepthStencilState.hash = 0xFFFFFFFF; _newDepthStencilState.hash = 0;

	for( uint32 i = 0; i < 16; ++i )
		setTexture( i, 0, 0 );

	setColorWriteMask( true );
	_pendingMask = 0xFFFFFFFF;
	commitStates();

//*	glBindBuffer( GL_ARRAY_BUFFER, 0 );

}


// =================================================================================================
// Draw calls and clears
// =================================================================================================

void RenderDevice::clear( uint32 flags, float *colorRGBA, float depth )
{
	ID3D11RenderTargetView*	renderTargetViews[4];
	ID3D11DepthStencilView*	depthStencilView;

	_d3dContext->OMGetRenderTargets(4, renderTargetViews, &depthStencilView);

	if( (flags & CLR_DEPTH)!=0 && depthStencilView != 0x0 )
		_d3dContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, depth, 0);

	float blackColor[4] = {0,0,0,0};
	float* clearColor = colorRGBA ? colorRGBA : blackColor; 

	if( (flags & CLR_COLOR_RT0) && renderTargetViews[0] != 0x0 )
		_d3dContext->ClearRenderTargetView (renderTargetViews[0], clearColor);
	if( (flags & CLR_COLOR_RT1) && renderTargetViews[1] != 0x0 )
		_d3dContext->ClearRenderTargetView (renderTargetViews[1], clearColor);
	if( (flags & CLR_COLOR_RT2) && renderTargetViews[2] != 0x0 )
		_d3dContext->ClearRenderTargetView (renderTargetViews[2], clearColor);
	if( (flags & CLR_COLOR_RT3) && renderTargetViews[3] != 0x0 )
		_d3dContext->ClearRenderTargetView (renderTargetViews[3], clearColor);

	for(uint32 i=0; i<4; ++i)
		SAFE_RELEASE(renderTargetViews[i]);
	SAFE_RELEASE(depthStencilView);

	commitStates( PM_VIEWPORT | PM_SCISSOR | PM_RENDERSTATES );
}


void RenderDevice::draw( RDIPrimType primType, uint32 firstVert, uint32 numVerts )
{
	if( commitStates() )
	{
		_d3dContext->IASetPrimitiveTopology( (D3D11_PRIMITIVE_TOPOLOGY) primType );
		_d3dContext->Draw( numVerts, firstVert );
	}
}


void RenderDevice::drawIndexed( RDIPrimType primType, uint32 firstIndex, uint32 numIndices,
                                uint32 firstVert, uint32 numVerts )
{
	if( commitStates() )
	{
		_d3dContext->IASetPrimitiveTopology( (D3D11_PRIMITIVE_TOPOLOGY) primType );
		_d3dContext->DrawIndexed( numIndices, firstIndex, 0 );
	}
}

}  // namespace
