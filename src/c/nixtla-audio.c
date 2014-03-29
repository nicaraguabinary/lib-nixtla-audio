//
//  nixtla.c
//  NixtlaAudioLib
//
//  Created by Marcos Ortega on 11/02/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//
//  This entire notice must be retained in this source code.
//  This source code is under LGLP v2.1 Licence.
//
//  This software is provided "as is", with absolutely no warranty expressed
//  or implied. Any use is at your own risk.
//
//  Latest fixes enhancements and documentation at https://github.com/nicaraguabinary/nixtla-audio
//

#define NIX_ASSERTS_ACTIVATED

#include <stdio.h>			//NULL
#include <string.h>			//memcpy, memset
#ifdef NIX_ASSERTS_ACTIVATED
	#include <assert.h>			//assert
#endif
#include "nixtla-audio.h"

//-------------------------------
//-- IDENTIFY OS
//-------------------------------
#if defined(_WIN32) || defined(WIN32) //Windows
	#pragma message("COMPILING FOR WINDOWS (OpenAL)")
	#include <AL/al.h>
	#include <AL/alc.h>
	#define NIX_OPENAL
#elif defined(__ANDROID__) //Android
	#pragma message("COMPILING FOR ANDROID (OpenSL)")
	#include <SLES/OpenSLES.h>
	#include <SLES/OpenSLES_Android.h>
	#define NIX_OPENSL
#elif defined(__linux__) || defined(linux) //Linux
	#pragma message("COMPILING FOR LINUX (OpenAL)")
	#include <AL/al.h>	//remember to install "libopenal-dev" package
	#include <AL/alc.h> //remember to install "libopenal-dev" package
	#define NIX_OPENAL
#elif defined(__MAC_OS_X_VERSION_MAX_ALLOWED) //OSX
	#pragma message("COMPILING FOR MacOSX (OpenAL)")
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	#define NIX_OPENAL
#else	//iOS?
	#pragma message("COMPILING FOR iOS? (OpenAL)")
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	#define NIX_OPENAL
#endif

//TEMPORAL (forzar OpenSL)
//#undef NIX_OPENAL
//#define NIX_OPENSL

//TEMPORAL (forzar OpenAL)
//#define NIX_OPENAL
//#undef NIX_OPENSL

//
// You can custom memory management by defining this MACROS
// and CONSTANTS before this file get included or compiled.
//
// This are the default memory management MACROS and CONSTANTS:
#if !defined(NIX_MALLOC) || !defined(NIX_FREE)
	#include <stdlib.h>		//malloc, free
	#ifndef NIX_MALLOC
		#define NIX_MALLOC(POINTER_DEST, POINTER_TYPE, SIZE_BYTES, STR_HINT) POINTER_DEST = (POINTER_TYPE*)malloc(SIZE_BYTES);
	#endif
	#ifndef NIX_FREE
		#define NIX_FREE(POINTER) free(POINTER);
	#endif
#endif
#ifndef NIX_MSWAIT_BEFORE_DELETING_BUFFERS
	#ifdef NIX_OPENAL
		#define NIX_MSWAIT_BEFORE_DELETING_BUFFERS	250 //For some reason OpenAL fails when stopping, unqueuing and deleting buffers in the same tick
	#endif
#endif
#ifndef NIX_SOURCES_MAX
	#define NIX_SOURCES_MAX		0xFFFF
#endif
#ifndef NIX_SOURCES_GROWTH
	#define NIX_SOURCES_GROWTH	1
#endif
#ifndef NIX_BUFFERS_MAX
	#define NIX_BUFFERS_MAX		0xFFFF
#endif
#ifndef NIX_BUFFERS_GROWTH
	#define NIX_BUFFERS_GROWTH	1
#endif
#ifndef NIX_AUDIO_GROUPS_SIZE
	#define NIX_AUDIO_GROUPS_SIZE 8
#endif


//++++++++++++++++++++
//++++++++++++++++++++
//++++++++++++++++++++
//++ PRIVATE HEADER ++
//++++++++++++++++++++
//++++++++++++++++++++
//++++++++++++++++++++

#if defined(__ANDROID__)
	//#pragma message("COMPILANDO PARA ANDROID")
	#include <android/log.h>
	#define PRINTF_INFO(STR_FMT, ...)		__android_log_print(ANDROID_LOG_INFO, "Nixtla", STR_FMT, ##__VA_ARGS__) //((void)0)
	#define PRINTF_ERROR(STR_FMT, ...)		__android_log_print(ANDROID_LOG_ERROR, "Nixtla", "ERROR, "STR_FMT, ##__VA_ARGS__)
	#define PRINTF_WARNING(STR_FMT, ...)	__android_log_print(ANDROID_LOG_WARN, "Nixtla", "WARNING, "STR_FMT, ##__VA_ARGS__)
#elif defined(__APPLE__) || (defined(__APPLE__) && defined(__MACH__))
	//#pragma message("COMPILANDO PARA iOS/Mac")
	#define PRINTF_INFO(STR_FMT, ...)		printf("Nix, " STR_FMT, ##__VA_ARGS__) //((void)0)
	#define PRINTF_ERROR(STR_FMT, ...)		printf("Nix ERROR, " STR_FMT, ##__VA_ARGS__)
	#define PRINTF_WARNING(STR_FMT, ...)	printf("Nix WARNING, " STR_FMT, ##__VA_ARGS__)
#else
	//#pragma message("(SE ASUME) COMPILANDO PARA BLACKBERRY")
	#define PRINTF_INFO(STR_FMT, ...)		fprintf(stdout, "Nix, " STR_FMT, ##__VA_ARGS__); fflush(stdout) //((void)0)
	#define PRINTF_ERROR(STR_FMT, ...)		fprintf(stderr, "Nix ERROR, " STR_FMT, ##__VA_ARGS__); fflush(stderr)
	#define PRINTF_WARNING(STR_FMT, ...)	fprintf(stdout, "Nix WARNING, " STR_FMT, ##__VA_ARGS__); fflush(stdout)
#endif

//-------------------------------
//-- BASIC DEFINITIONS
//-------------------------------

#ifdef NIX_ASSERTS_ACTIVATED
	#define NIX_ASSERT(EVAL)			{ if(!(EVAL)){ PRINTF_ERROR("ASSERT, cond '"#EVAL"'.\n"); PRINTF_ERROR("ASSERT, file '%s'\n", __FILE__); PRINTF_ERROR("ASSERT, line %d.\n", __LINE__); assert(0); }}
#else
	#define NIX_ASSERT(EVAL)			((void)0);
#endif

#define STR_ERROR_AL(idError) alGetString(idError) //(idError==AL_INVALID_NAME?"AL_INVALID_NAME":idError==AL_INVALID_ENUM?"AL_INVALID_ENUM":idError==AL_INVALID_VALUE?"AL_INVALID_VALUE":idError==AL_INVALID_OPERATION?"AL_INVALID_OPERATION":idError==AL_OUT_OF_MEMORY?"AL_OUT_OF_MEMORY":idError==AL_NO_ERROR?"AL_NO_ERROR":"AL_ERROR_DESCONOCIDO")
#define VERIFICA_ERROR_AL(nomFunc) 	{ ALenum idErrorAL=alGetError(); if(idErrorAL!=AL_NO_ERROR){ PRINTF_ERROR("'%s' (#%d) en %s\n", STR_ERROR_AL(idErrorAL), idErrorAL, nomFunc);} NIX_ASSERT(idErrorAL==AL_NO_ERROR);}

#define NIX_OPENAL_AUDIO_FORMAT(CHANNELS, BITSPS)	((CHANNELS==1 && BITSPS==16) ? AL_FORMAT_MONO16 : (CHANNELS==1 && BITSPS==8) ? AL_FORMAT_MONO8 : (CHANNELS==2 && BITSPS==16) ? AL_FORMAT_STEREO16 : (CHANNELS==2 && BITSPS==8) ? AL_FORMAT_STEREO8 : 0)
#define NIX_OPENSL_AUDIO_SAMPLE_FORMAT(BITSPS)		(BITSPS==8 ? SL_PCMSAMPLEFORMAT_FIXED_8 : BITSPS==16 ? SL_PCMSAMPLEFORMAT_FIXED_16 : BITSPS==24 ? SL_PCMSAMPLEFORMAT_FIXED_24 : BITSPS==32 ? SL_PCMSAMPLEFORMAT_FIXED_32 : 0)

//-------------------------------
//-- AUDIO BUFFERS
//-------------------------------
typedef enum ENNixBufferState_ {
	ENNixBufferState_Free = 50,				//Empty, data was played or waiting for data capture
	ENNixBufferState_LoadedForPlay,			//Filled with data but not attached to source queue
	ENNixBufferState_AttachedForPlay,		//Filled with data and attached to source queue
	//
	ENNixBufferState_AttachedForCapture,	//Filled with data and attached to capture queue
	ENNixBufferState_LoadedWithCapture		//Filled with data but not attached to capture queue
	#ifdef NIX_MSWAIT_BEFORE_DELETING_BUFFERS
	, ENNixBufferState_WaitingForDeletion	//The buffer was unqueued and is waiting for deletion (OpenAL gives error when deleting a buffer inmediately after unqueue)
	#endif
} ENNixBufferState;

//Audio buffer description
typedef struct STNix_bufferDesc_ {
	ENNixBufferState	state;
	STNix_audioDesc		audioDesc;
	NixUI8*				dataPointer;
	NixUI32				dataBytesCount;
} STNix_bufferDesc;

typedef struct STNix_bufferAL_ {
	NixUI8				regInUse;
	NixUI32				retainCount;
	#ifdef NIX_OPENAL
	ALuint				idBufferAL; //ALuint
	#endif
	STNix_bufferDesc	bufferDesc;
	#ifdef NIX_MSWAIT_BEFORE_DELETING_BUFFERS
	NixUI32				_msWaitingForDeletion;
	#endif
} STNix_bufferAL;

#define NIX_BUFFER_SECONDS(FLT_DEST, BUFFER) if(BUFFER.audioDesc.channels!=0 && BUFFER.audioDesc.bitsPerSample!=0 && BUFFER.audioDesc.samplerate!=0) FLT_DEST = ((float)BUFFER.dataBytesCount / (float)(BUFFER.audioDesc.channels * (BUFFER.audioDesc.bitsPerSample / 8)) / (float)BUFFER.audioDesc.samplerate); else FLT_DEST=0.0f;
#define NIX_BUFFER_SECONDS_P(FLT_DEST, BUFFER) if(BUFFER->audioDesc.channels!=0 && BUFFER->audioDesc.bitsPerSample!=0 && BUFFER->audioDesc.samplerate!=0) FLT_DEST = ((float)BUFFER->dataBytesCount / (float)(BUFFER->audioDesc.channels * (BUFFER->audioDesc.bitsPerSample / 8)) / (float)BUFFER->audioDesc.samplerate); else FLT_DEST=0.0f;

#define NIX_BUFFER_IS_COMPATIBLE(BUFFER, CHANNELS, BITPS, FREQ) (BUFFER.channels==CHANNELS && BUFFER.bitsPerSample==BITPS && BUFFER.samplerate==FREQ)
#define NIX_BUFFER_IS_COMPATIBLE_P(BUFFER, CHANNELS, BITPS, FREQ) (BUFFER->channels==CHANNELS && BUFFER->bitsPerSample==BITPS && BUFFER->samplerate==FREQ)

#define NIX_BUFFER_ARE_COMPATIBLE(BUFFER1, BUFFER2) (BUFFER1.channels==BUFFER2.channels && BUFFER1.bitsPerSample==BUFFER2.bitsPerSample && BUFFER1.samplerate==BUFFER2.samplerate)
#define NIX_BUFFER_ARE_COMPATIBLE_P(BUFFER1, BUFFER2) (BUFFER1->channels==BUFFER2->channels && BUFFER1->bitsPerSample==BUFFER2->bitsPerSample && BUFFER1->samplerate==BUFFER2->samplerate)

//-------------------------------
//-- AUDIO SOURCES
//-------------------------------
typedef enum ENNixSourceState_ {
	ENNixSourceState_Stopped,			//La fuente esta detenida o solo-inicializada y lista para ser asignada
	ENNixSourceState_Playing,			//La fuente se esta reproduciendo o pausada
} ENNixSourceState;

//Tipo de la fuente segun los bufferes anexados
typedef enum ENNixSourceType_ {
	ENNixSourceType_Undefined = 90,
	ENNixSourceType_Static,
	ENNixSourceType_Stream
} ENNixSourceType;

#define NIX_SOURCE_TYPE_STR(SRCTYPE) (SRCTYPE==ENNixSourceType_Undefined?"Undefined":SRCTYPE==ENNixSourceType_Static?"Static":SRCTYPE==ENNixSourceType_Stream?"Stream":"ENNixSourceType_notValid")

#ifdef NIX_OPENSL
typedef struct STNix_OpenSLSourceCallbackParam_ {
	void*		eng;
	NixUI32		sourceIndex;
} STNix_OpenSLSourceCallbackParam;
#endif

typedef struct STNix_source_ {
	#ifdef NIX_OPENAL
	ALuint		idSourceAL;	//ALuint
	#elif defined(NIX_OPENSL)
	SLObjectItf slPlayerObject;
	SLPlayItf	slPlayerIntf;
	SLVolumeItf	volumeIntf;
	SLmillibel	volumeMax;
	NixBOOL		repeat;
	SLAndroidSimpleBufferQueueItf		slPlayerBufferQueue;
	NixUI16								slQueueSize;
	STNix_OpenSLSourceCallbackParam*	callbackParam;
	STNix_audioDesc						sourceFormat;
	#endif
	NixUI8		regInUse;
	NixUI8		sourceType;			//ENNixSourceType
	NixUI8		sourceState;		//ENNixSourceState
	NixUI8		audioGroupIndex;	//Audio group index
	float		volume;				//Individual volume (this sound)
	//
	NixUI8		isReusable;
	NixUI32		retainCount;
	//Callbacks
	PTRNIX_SourceReleaseCallback releaseCallBack;
	void*		releaseCallBackUserData;
	PTRNIX_StreamBufferUnqueuedCallback bufferUnqueuedCallback;
	void*		bufferUnqueuedCallbackData;
	//Index of buffers attached to this source
	NixUI16*	queueBuffIndexes;
	NixUI16		queueBuffIndexesUse;
	NixUI16		queueBuffIndexesSize;
	NixUI16		buffStreamUnqueuedCount;	//For streams, how many buffers had been unqueued since last "nixTick()". For bufferUnqueuedCallback.
} STNix_source;

typedef struct STNix_EngineObjetcs_ {
	NixUI32				maskCapabilities;
	#ifdef NIX_OPENAL
	ALCcontext*			contextAL;				//OpenAL specific
	ALCdevice*			deviceAL;				//OpenAL specific
	#elif defined(NIX_OPENSL)
	SLObjectItf 		slObject;				//OpenSL specific
	SLEngineItf 		slEngine;				//OpenSL specific
	SLObjectItf 		slOutputMixObject;		//OpenSL specific
	#endif
	//Audio sources
	STNix_source*		sourcesArr;
	NixUI16				sourcesArrUse;
	NixUI16				sourcesArrSize;
	//Audio buffers (complete short sounds and stream portions)
	STNix_bufferAL*		buffersArr;
	NixUI16				buffersArrUse;
	NixUI16				buffersArrSize;
	//Audio capture
	#ifdef NIX_OPENAL
	ALCdevice*			deviceCaptureAL;				//OpenAL specific
	NixUI32				captureMainBufferBytesCount;	//OpenAL specific
	#elif defined(NIX_OPENSL)
	SLObjectItf			slRecObject;				//OpenSL specific
	SLRecordItf			slRecRecord;				//OpenSL specific
	SLAndroidSimpleBufferQueueItf slRecBufferQueue;	//OpenSL specific
	#endif
	NixUI8				captureInProgess;
	NixUI32				captureSamplesPerBuffer;
	STNix_audioDesc		captureFormat;
	//Capture buffers
	STNix_bufferDesc*	buffersCaptureArr;
	NixUI16				buffersCaptureArrFirst;
	NixUI16				buffersCaptureArrFilledCount;
	NixUI16				buffersCaptureArrSize;
	PTRNIX_CaptureBufferFilledCallback buffersCaptureCallback;
	void*				buffersCaptureCallbackUserData;
} STNix_EngineObjetcs;

//-------------------------------
//-- AUDIO GROUPS
//-------------------------------
typedef struct STNix_AudioGroup_ {
	NixBOOL				enabled;
	float				volume;
} STNix_AudioGroup;

STNix_AudioGroup __nixSrcGroups[NIX_AUDIO_GROUPS_SIZE];

#ifdef NIX_OPENAL
void	__nixSreamsOpenALRemoveProcecedBuffers(STNix_Engine* engAbs); //OpenAL specific
void	__nixCaptureOpenALMoveSamplesToBuffers(STNix_Engine* engAbs); //OpenAL specific
#endif

#ifdef NIX_OPENSL
void	__nixSourceBufferQueueSLCallback(SLAndroidSimpleBufferQueueItf bq, void* param); //OpenSL specific
void	__nixCaptureBufferFilledCallback(SLAndroidSimpleBufferQueueItf bq, void* param); //OpenSL specific
NixBOOL	__nixSourceInitOpenSLPlayer(STNix_EngineObjetcs* eng, const NixUI16 sourceIndex, STNix_source* src, STNix_audioDesc* audioDesc); //OpenSL specific
void	__nixCaptureOpenSLConsumeFilledBuffers(STNix_Engine* engAbs);
#endif

//Audio buffers
NixUI16	__nixBufferCreate(STNix_EngineObjetcs* eng);
void	__nixBufferDestroy(STNix_bufferAL* buffer, const NixUI16 bufferIndex);
void	__nixBufferRetain(STNix_bufferAL* buffer);
void	__nixBufferRelease(STNix_bufferAL* buffer, const NixUI16 bufferIndex);
NixBOOL __nixBufferSetData(STNix_EngineObjetcs* eng, STNix_bufferAL* buffer, const STNix_audioDesc* audioDesc, const NixUI8* audioDataPCM, const NixUI32 audioDataPCMBytes);

//Audio sources
void	__nixSourceInit(STNix_source* src);
void	__nixSourceFinalize(STNix_Engine* engAbs, STNix_source* src, const NixUI16 sourceIndex, const NixBOOL forReuse);
void	__nixSourceRetain(STNix_source* source);
void	__nixSourceRelease(STNix_Engine* engAbs, STNix_source* source, const NixUI16 sourceIndex);
NixUI16	__nixSourceAdd(STNix_EngineObjetcs* eng, STNix_source* source);
NixUI16	__nixSourceAssign(STNix_Engine* engAbs, NixUI8 lookIntoReusable, NixUI8 audioGroupIndex, PTRNIX_SourceReleaseCallback releaseCallBack, void* releaseCallBackUserData, const NixUI16 queueSize, PTRNIX_StreamBufferUnqueuedCallback bufferUnqueueCallback, void* bufferUnqueueCallbackData);
//Audio sources queue
NixUI16	__nixSrcQueueFirstBuffer(STNix_source* src);
void	__nixSrcQueueSetUniqueBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex);
void	__nixSrcQueueAddStreamBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex);
void	__nixSrcQueueAddBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex);
void	__nixSrcQueueClear(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex);
void	__nixSrcQueueRemoveBuffersOldest(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex, NixUI32 buffersCountFromOldest);
void	__nixSrcQueueRemoveBuffersNewest(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex, NixUI32 buffersCountFromNewest);

//++++++++++++++++++
//++++++++++++++++++
//++++++++++++++++++
//++ PRIVATE CODE ++
//++++++++++++++++++
//++++++++++++++++++
//++++++++++++++++++

//
#define NIX_GET_SOURCE_START(ENG_OBJS, SRC_INDEX, SRC_DEST_POINTER) \
	NIX_ASSERT(SRC_INDEX!=0) \
	NIX_ASSERT(SRC_INDEX < ENG_OBJS->sourcesArrSize) \
	if(SRC_INDEX != 0 && SRC_INDEX < ENG_OBJS->sourcesArrSize){ \
		STNix_source* SRC_DEST_POINTER = &ENG_OBJS->sourcesArr[SRC_INDEX]; \
		NIX_ASSERT(SRC_DEST_POINTER->regInUse) \
		NIX_ASSERT(SRC_DEST_POINTER->retainCount!=0 /*|| SRC_DEST_POINTER->isReusable*/) \
		if(SRC_DEST_POINTER->regInUse && (SRC_DEST_POINTER->retainCount!=0 /*|| SRC_DEST_POINTER->isReusable*/))

#define NIX_GET_SOURCE_END \
	} \

#define NIX_GET_BUFFER_START(ENG_OBJS, BUFF_INDEX, BUFF_DEST_POINTER) \
	NIX_ASSERT(BUFF_INDEX != 0) \
	NIX_ASSERT(BUFF_INDEX < ENG_OBJS->buffersArrUse) \
	if(BUFF_INDEX != 0 && BUFF_INDEX < ENG_OBJS->buffersArrUse){ \
		STNix_bufferAL* BUFF_DEST_POINTER = &ENG_OBJS->buffersArr[BUFF_INDEX]; \
		NIX_ASSERT(BUFF_DEST_POINTER->regInUse) \
		if(BUFF_DEST_POINTER->regInUse)

#define NIX_GET_BUFFER_END \
	} \

//----------------------------------------------------------
//--- Audio buffers
//----------------------------------------------------------
NixUI16 __nixBufferCreate(STNix_EngineObjetcs* eng){
	STNix_bufferAL audioBuffer;
	NixUI16 i; const NixUI16 useCount = eng->buffersArrUse;
	//Format struct
	audioBuffer.regInUse				= NIX_TRUE;
	audioBuffer.retainCount				= 1; //retained by creator
	audioBuffer.bufferDesc.state		= ENNixBufferState_Free;
	//audioBuffer.bufferDesc.audioDesc
	audioBuffer.bufferDesc.dataBytesCount = 0;
	audioBuffer.bufferDesc.dataPointer	= NULL;
	#ifdef NIX_OPENAL
	{
		ALuint idBufferOpenAL = 0; ALenum errorAL;
		alGenBuffers(1, &idBufferOpenAL); //el ID=0 es valido en OpenAL (no usar '0' para validar los IDs)
		errorAL = alGetError();
		if(errorAL != AL_NONE){
			PRINTF_ERROR("alGenBuffers failed: #%d '%s' idBufferAL(%d)\n", errorAL, STR_ERROR_AL(errorAL), idBufferOpenAL);
			return 0;
		} else {
			audioBuffer.idBufferAL = idBufferOpenAL;
		}
	}
	#endif
	//Look for a available register
	for(i=1; i<useCount; i++){ //Source index zero is reserved
		STNix_bufferAL* source = &eng->buffersArr[i];
		if(!source->regInUse){
			eng->buffersArr[i]		= audioBuffer;
			PRINTF_INFO("Buffer created(%d).\n", i);
			return i;
		}
	}
	//Add new register
	if(eng->buffersArrUse < NIX_BUFFERS_MAX){
		if(eng->buffersArrSize >= eng->buffersArrUse){
			STNix_bufferAL* buffersArr;
			eng->buffersArrSize	+= NIX_BUFFERS_GROWTH;
			NIX_MALLOC(buffersArr, STNix_bufferAL, sizeof(STNix_bufferAL) * eng->buffersArrSize, "buffersArr")
			if(eng->sourcesArr!=NULL){
				if(eng->sourcesArrUse!=0) memcpy(buffersArr, eng->buffersArr, sizeof(STNix_bufferAL) * eng->buffersArrUse);
				NIX_FREE(eng->buffersArr);
			}
			eng->buffersArr		= buffersArr;
		}
		eng->buffersArr[eng->buffersArrUse++] = audioBuffer;
		PRINTF_INFO("Buffer created(%d).\n", (eng->buffersArrUse-1));
		return (eng->buffersArrUse-1);
	}
	//Cleanup, if something went wrong
	__nixBufferDestroy(&audioBuffer, 0);
	return 0;
}

void __nixBufferDestroy(STNix_bufferAL* buffer, const NixUI16 bufferIndex){
	PRINTF_INFO("Buffer Destroying(%d).\n", bufferIndex);
	NIX_ASSERT(buffer->regInUse)
	#ifdef NIX_OPENAL
	alDeleteBuffers(1, &buffer->idBufferAL); VERIFICA_ERROR_AL("alDeleteBuffers");
	#endif
	if(buffer->bufferDesc.dataPointer!=NULL){
		NIX_FREE(buffer->bufferDesc.dataPointer)
		buffer->bufferDesc.dataPointer = NULL;
	}
	buffer->regInUse = NIX_FALSE;
}

void __nixBufferRetain(STNix_bufferAL* buffer){
	NIX_ASSERT(buffer->regInUse)
	buffer->retainCount++;
}

void __nixBufferRelease(STNix_bufferAL* buffer, const NixUI16 bufferIndex){
	NIX_ASSERT(buffer->regInUse)
	NIX_ASSERT(buffer->retainCount!=0)
	buffer->retainCount--;
	if(buffer->retainCount==0){
		#ifdef NIX_MSWAIT_BEFORE_DELETING_BUFFERS
		PRINTF_INFO("Buffer Programming destruction(%d).\n", bufferIndex);
		buffer->_msWaitingForDeletion		= 0;
		buffer->bufferDesc.state	= ENNixBufferState_WaitingForDeletion;
		#else
		__nixBufferDestroy(buffer, bufferIndex);
		#endif
	}
}

NixBOOL __nixBufferSetData(STNix_EngineObjetcs* eng, STNix_bufferAL* buffer, const STNix_audioDesc* audioDesc, const NixUI8* audioDataPCM, const NixUI32 audioDataPCMBytes){
	#ifdef NIX_OPENAL
	//OPENAL ony supports 8 and 16 bits integer (mono and stereo)
	if(audioDesc->samplesFormat==ENNix_sampleFormat_int){
		const ALenum dataFormat = NIX_OPENAL_AUDIO_FORMAT(audioDesc->channels, audioDesc->bitsPerSample);
		NIX_ASSERT(buffer->regInUse)
		NIX_ASSERT(buffer->retainCount!=0)
		if(dataFormat!=0){
			ALenum errorAL;
			if(audioDataPCM==NULL){
				NixUI8* audioTmp; NIX_MALLOC(audioTmp, NixUI8, audioDataPCMBytes, "__nixBufferSetData::audioTmp")
				alBufferData(buffer->idBufferAL, dataFormat, audioTmp, audioDataPCMBytes, audioDesc->samplerate); //PENDIENTE: validar frecuencia
				NIX_FREE(audioTmp)
			} else {
				alBufferData(buffer->idBufferAL, dataFormat, audioDataPCM, audioDataPCMBytes, audioDesc->samplerate); //PENDIENTE: validar frecuencia
			}
			errorAL = alGetError();
			if(errorAL != AL_NONE){
				PRINTF_ERROR("alBufferData failed: #%d '%s'.\n", errorAL, STR_ERROR_AL(errorAL));
			} else {
				buffer->bufferDesc.audioDesc		= *audioDesc;
				if(buffer->bufferDesc.dataPointer!=NULL){ NIX_FREE(buffer->bufferDesc.dataPointer); }
				buffer->bufferDesc.dataPointer		= NULL;
				buffer->bufferDesc.dataBytesCount	= audioDataPCMBytes;
				buffer->bufferDesc.state			= ENNixBufferState_LoadedForPlay;
				return NIX_TRUE;
			}
		}
	}
	#elif defined(NIX_OPENSL)
	NIX_ASSERT(buffer->regInUse)
	NIX_ASSERT(buffer->retainCount!=0)
	buffer->bufferDesc.audioDesc		= *audioDesc;
	if(buffer->bufferDesc.dataPointer!=NULL){ NIX_FREE(buffer->bufferDesc.dataPointer); }
	NIX_MALLOC(buffer->bufferDesc.dataPointer, NixUI8, sizeof(NixUI8) * audioDataPCMBytes, "bufferDesc.dataPointer")
	if(audioDataPCM!=NULL) memcpy(buffer->bufferDesc.dataPointer, audioDataPCM, sizeof(NixUI8) * audioDataPCMBytes);
	buffer->bufferDesc.dataBytesCount		= audioDataPCMBytes;
	buffer->bufferDesc.state = ENNixBufferState_LoadedForPlay;
	return NIX_TRUE;
	#endif
	return NIX_FALSE;
}

//----------------------------------------------------------
//--- Audio sources
//----------------------------------------------------------
void __nixSourceInit(STNix_source* src){
	#ifdef NIX_OPENAL
	src->idSourceAL			= 0;
	#elif defined(NIX_OPENSL)
	src->slPlayerObject				= NULL;
	src->slPlayerIntf				= NULL;
	src->volumeIntf					= NULL;
	src->volumeMax					= 0;
	src->repeat						= NIX_FALSE;
	src->slPlayerBufferQueue		= NULL;
	src->callbackParam				= NULL;
	src->sourceFormat.bitsPerSample	= 0;
	src->sourceFormat.blockAlign	= 0;
	src->sourceFormat.channels		= 0;
	src->sourceFormat.samplerate	= 0;
	#endif
	src->regInUse					= NIX_TRUE;
	src->sourceType					= ENNixSourceType_Undefined;
	src->sourceState				= ENNixSourceState_Stopped;
	src->audioGroupIndex			= 0;
	src->volume						= 1.0f;
	//
	src->isReusable					= NIX_FALSE;
	src->retainCount				= 1; //retained by creator
	src->releaseCallBack			= NULL;
	src->releaseCallBackUserData	= NULL;
	//
	src->queueBuffIndexes			= NULL;
	src->queueBuffIndexesUse		= 0;
	src->queueBuffIndexesSize		= 0;
	//
	src->buffStreamUnqueuedCount	= 0;
}

NixUI16 __nixSourceAdd(STNix_EngineObjetcs* eng, STNix_source* source){
	//Look for a available register
	NixUI16 i; const NixUI16 useCount = eng->sourcesArrUse;
	for(i=1; i<useCount; i++){ //Source index zero is reserved
		STNix_source* src = &eng->sourcesArr[i];
		if(!src->regInUse){
			(*src) = (*source);
			PRINTF_INFO("Source created(%d) using free register.\n", i);
			return i;
		}
	}
	//Add new register
	if(eng->sourcesArrUse < NIX_SOURCES_MAX){
		if(eng->sourcesArrSize >= eng->sourcesArrUse){
			STNix_source* sourcesArr;
			eng->sourcesArrSize	+= NIX_SOURCES_GROWTH;
			NIX_MALLOC(sourcesArr, STNix_source, sizeof(STNix_source) * eng->sourcesArrSize, "sourcesArr")
			if(eng->sourcesArr!=NULL){
				if(eng->sourcesArrUse!=0) memcpy(sourcesArr, eng->sourcesArr, sizeof(STNix_source) * eng->sourcesArrUse);
				NIX_FREE(eng->sourcesArr);
			}
			eng->sourcesArr		= sourcesArr;
		}
		eng->sourcesArr[eng->sourcesArrUse++] = (*source);
		PRINTF_INFO("Source created(%d) growing array.\n", (eng->sourcesArrUse - 1));
		return (eng->sourcesArrUse - 1);
	}
	return 0;
}

void __nixSourceFinalize(STNix_Engine* engAbs, STNix_source* source, const NixUI16 sourceIndex, const NixBOOL forReuse){
	STNix_EngineObjetcs* eng	= (STNix_EngineObjetcs*)engAbs->o;
	NIX_ASSERT(source->regInUse == NIX_TRUE)
	if(source->queueBuffIndexes!=NULL){
		__nixSrcQueueClear(eng, source, sourceIndex);
		NIX_FREE(source->queueBuffIndexes);
		source->queueBuffIndexes = NULL;
		source->queueBuffIndexesUse = 0;
		source->queueBuffIndexesSize = 0;
	}
	if(source->releaseCallBack!=NULL){
		(*source->releaseCallBack)(engAbs, source->releaseCallBackUserData, sourceIndex);
		source->releaseCallBack = NULL;
		source->releaseCallBackUserData = NULL;
	}
	if(!forReuse){
		#ifdef NIX_OPENAL
		ALuint idFuenteAL = source->idSourceAL;
		alSourceStop(idFuenteAL);
		alDeleteSources(1, &idFuenteAL); VERIFICA_ERROR_AL("alDeleteSources");
		source->idSourceAL = 0;
		#elif defined(NIX_OPENSL)
		if(source->slPlayerObject!=NULL) {
			(*source->slPlayerObject)->Destroy(source->slPlayerObject);
			source->slPlayerObject		= NULL;
			source->slPlayerIntf 		= NULL;
			source->slPlayerBufferQueue	= NULL;
		}
		if(source->callbackParam!=NULL){ NIX_FREE(source->callbackParam); source->callbackParam = NULL; }
		#endif
		//PENDIENTE: liberar bufferes asociados a la fuente
		source->regInUse = NIX_FALSE;
		PRINTF_INFO("Source destroyed(%d).\n", sourceIndex);
	} else {
		PRINTF_INFO("Source destroyed for reuse(%d).\n", sourceIndex);
	}
}

void __nixSourceRetain(STNix_source* source){
	NIX_ASSERT(source->regInUse)
	source->retainCount++;
}

void __nixSourceRelease(STNix_Engine* engAbs, STNix_source* source, const NixUI16 sourceIndex){
	NIX_ASSERT(source->regInUse)
	NIX_ASSERT(source->retainCount!=0)
	source->retainCount--;
	if(source->retainCount==0){
		__nixSourceFinalize(engAbs, source, sourceIndex, source->isReusable);
	}
}

/*float __nixSourceSecondsInBuffers(STNix_source* src){
	float total = 0.0f, secsBuffer;
	NixUI32 i; const NixUI32 useCount = src->buffersArrUse;
	STNix_bufferDesc* buffDesc;
	for(i=0; i<useCount; i++){
		buffDesc = &src->buffersArr[i]._buffer.bufferDesc;
		NIX_BUFFER_SECONDS_P(secsBuffer, buffDesc);
		total += secsBuffer;
	}
	return total;
}*/

//----------------------------------------------------------
//--- Sources queues
//----------------------------------------------------------

void __nixSrcQueueSetUniqueBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex){
	NIX_ASSERT(src->queueBuffIndexesUse==0 || (src->sourceType==ENNixSourceType_Static && src->queueBuffIndexesUse==1)); //Se supone que el buffer debe estar vacio o ser un buffer no-stream
	__nixSrcQueueAddBuffer(src, buffer, buffIndex);
	src->sourceType = ENNixSourceType_Static;
}

void __nixSrcQueueAddStreamBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex){
	NIX_ASSERT(src->sourceType == ENNixSourceType_Stream || src->sourceType == ENNixSourceType_Undefined) //No se supone deba usarse fuera de streams o bufferes sin definir
	__nixSrcQueueAddBuffer(src, buffer, buffIndex);
	src->sourceType = ENNixSourceType_Stream;
}

void __nixSrcQueueAddBuffer(STNix_source* src, STNix_bufferAL* buffer, const NixUI16 buffIndex){
	if(src->queueBuffIndexesSize <= src->queueBuffIndexesUse){
		NixUI16* buffArr;
		src->queueBuffIndexesSize += 4;
		NIX_MALLOC(buffArr, NixUI16, sizeof(NixUI16) * src->queueBuffIndexesSize, "queueBuffIndexes")
		if(src->queueBuffIndexes != NULL){
			if(src->queueBuffIndexesUse != 0) memcpy(buffArr, src->queueBuffIndexes, sizeof(NixUI16) * src->queueBuffIndexesUse);
			NIX_FREE(src->queueBuffIndexes);
		}
		src->queueBuffIndexes = buffArr;
	}
	src->queueBuffIndexes[src->queueBuffIndexesUse++] = buffIndex;
	__nixBufferRetain(buffer);
	buffer->bufferDesc.state = ENNixBufferState_AttachedForPlay;
}

void __nixSrcQueueClear(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex){
	__nixSrcQueueRemoveBuffersOldest(eng, src, sourceIndex, src->queueBuffIndexesUse);
}

void __nixSrcQueueRemoveBuffersOldest(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex, NixUI32 buffersCountFromOldest){
	NixUI32 i;
	NIX_ASSERT(buffersCountFromOldest <= src->queueBuffIndexesUse)
	PRINTF_INFO("Source(%d), unqueueing %d of %d buffers.\n", sourceIndex, buffersCountFromOldest, src->queueBuffIndexesUse);
	if(buffersCountFromOldest > src->queueBuffIndexesUse) buffersCountFromOldest = src->queueBuffIndexesUse;
	//Release buffers
	for(i=0; i<buffersCountFromOldest; i++){
		const NixUI16 buffIndex	= src->queueBuffIndexes[i];
		NIX_GET_BUFFER_START(eng, buffIndex, buffer){
			PRINTF_INFO("Unqueueing source(%d) buffer(%d, retainCount=%d) from queue.\n", sourceIndex, buffIndex, buffer->retainCount);
			NIX_ASSERT(buffer->bufferDesc.state == ENNixBufferState_AttachedForPlay)
			buffer->bufferDesc.state	= ENNixBufferState_LoadedForPlay;
			#ifdef NIX_OPENAL
			alSourceUnqueueBuffers(src->idSourceAL, 1, &buffer->idBufferAL); VERIFICA_ERROR_AL("alSourceUnqueueBuffers");
			#endif
			__nixBufferRelease(buffer, buffIndex);
		} NIX_GET_BUFFER_END
	}
	//Rearrange array elements
	src->queueBuffIndexesUse -= buffersCountFromOldest;
	for(i=0; i<src->queueBuffIndexesUse; i++){
		src->queueBuffIndexes[i] = src->queueBuffIndexes[i + buffersCountFromOldest];
	}
	if(src->queueBuffIndexesUse==0){
		PRINTF_INFO("Source(%d) empty queue, changed from type(%d) to (%d 'Undefined')", sourceIndex, src->sourceType, ENNixSourceType_Undefined);
		src->sourceType = ENNixSourceType_Undefined;
	}
}

void __nixSrcQueueRemoveBuffersNewest(STNix_EngineObjetcs* eng, STNix_source* src, const NixUI16 sourceIndex, NixUI32 buffersCountFromNewest){
	NixUI32 i;
	NIX_ASSERT(buffersCountFromNewest <= src->queueBuffIndexesUse)
	PRINTF_INFO("Source(%d), unqueueing %d of %d buffers.\n", sourceIndex, buffersCountFromNewest, src->queueBuffIndexesUse);
	if(buffersCountFromNewest > src->queueBuffIndexesUse) buffersCountFromNewest = src->queueBuffIndexesUse;
	//Release buffers
	for(i=(src->queueBuffIndexesUse - buffersCountFromNewest); i<src->queueBuffIndexesUse; i++){
		const NixUI16 buffIndex	= src->queueBuffIndexes[i];
		NIX_GET_BUFFER_START(eng, buffIndex, buffer){
			PRINTF_INFO("Unqueueing source(%d) buffer(%d, retainCount=%d) from queue.\n", sourceIndex, buffIndex, buffer->retainCount);
			NIX_ASSERT(buffer->bufferDesc.state == ENNixBufferState_AttachedForPlay)
			buffer->bufferDesc.state	= ENNixBufferState_LoadedForPlay;
			#ifdef NIX_OPENAL
			alSourceUnqueueBuffers(src->idSourceAL, 1, &buffer->idBufferAL); VERIFICA_ERROR_AL("alSourceUnqueueBuffers");
			#endif
			__nixBufferRelease(buffer, buffIndex);
		} NIX_GET_BUFFER_END
	}
	src->queueBuffIndexesUse -= buffersCountFromNewest;
	if(src->queueBuffIndexesUse==0){
		PRINTF_INFO("Source(%d) empty queue, changed from type(%d) to (%d 'Undefined')", sourceIndex, src->sourceType, ENNixSourceType_Undefined);
		src->sourceType = ENNixSourceType_Undefined;
	}
}

NixUI16 __nixSrcQueueFirstBuffer(STNix_source* src){
	if(src->queueBuffIndexesUse>0) return src->queueBuffIndexes[0];
	return 0;
}

//-------------------------------
//-- ENGINES
//-------------------------------
NixUI8 nixInit(STNix_Engine* engAbs, const NixUI16 pregeneratedSources){
	STNix_EngineObjetcs* eng;		NIX_MALLOC(eng, STNix_EngineObjetcs, sizeof(STNix_EngineObjetcs), "STNix_EngineObjetcs")
	engAbs->o						= eng;
	#ifdef NIX_OPENAL
	eng->contextAL			= NULL;
	eng->deviceAL			= NULL;
	eng->deviceCaptureAL		= NULL;
	eng->captureMainBufferBytesCount	= 0;
	#elif defined(NIX_OPENSL)
	eng->slObject					= NULL;
	eng->slEngine					= NULL;
	eng->slOutputMixObject			= NULL;
	eng->slRecObject				= NULL;
	eng->slRecRecord				= NULL;
	eng->slRecBufferQueue			= NULL;
	#endif
	eng->maskCapabilities			= 0;
	//
	eng->sourcesArrUse				= 1; //Source index zero is reserved
	eng->sourcesArrSize			= 1 + (pregeneratedSources!=0 ? pregeneratedSources : NIX_SOURCES_GROWTH);
	NIX_MALLOC(eng->sourcesArr, STNix_source, sizeof(STNix_source) * eng->sourcesArrSize, "sourcesArr")
	eng->sourcesArr[0].regInUse = 0;
	//
	eng->buffersArrUse				= 1; //Buffer index zero is reserved
	eng->buffersArrSize			= 1 + NIX_BUFFERS_GROWTH;
	NIX_MALLOC(eng->buffersArr, STNix_bufferAL, sizeof(STNix_bufferAL) * eng->buffersArrSize, "buffersArr")
	eng->buffersArr[0].regInUse = 0;
	//
	eng->captureInProgess			= 0;
	eng->captureSamplesPerBuffer	= 0;
	//eng->captureFormat
	//
	eng->buffersCaptureArr			= NULL;
	eng->buffersCaptureArrFirst	= 0;
	eng->buffersCaptureArrFilledCount = 0;
	eng->buffersCaptureArrSize		= 0;
	//Audio groups
	{
		NixUI16 i;
		for(i=0; i<NIX_AUDIO_GROUPS_SIZE; i++){
			__nixSrcGroups[i].enabled	= NIX_TRUE;
			__nixSrcGroups[i].volume	= 1.0f;
		}
	}
	//
	#ifdef NIX_OPENAL
	eng->deviceAL			= alcOpenDevice(NULL);
	if(eng->deviceAL==NULL){
		PRINTF_ERROR("OpenAL::alcOpenDevice failed.\n");
	} else {
		eng->contextAL = alcCreateContext(eng->deviceAL, NULL);
		if(eng->contextAL==NULL){
			PRINTF_ERROR("OpenAL::alcCreateContext failed\n");
		} else {
			if(alcMakeContextCurrent(eng->contextAL)==AL_FALSE){
				PRINTF_ERROR("OpenAL::alcMakeContextCurrent failed\n");
			} else {
				//Masc of capabilities
				eng->maskCapabilities			|= (alcIsExtensionPresent(eng->deviceAL, "ALC_EXT_CAPTURE")!=ALC_FALSE || alcIsExtensionPresent(eng->deviceAL, "ALC_EXT_capture")!=ALC_FALSE) ? NIX_CAP_AUDIO_CAPTURE : 0;
				eng->maskCapabilities			|= (alIsExtensionPresent("AL_EXT_STATIC_BUFFER")!=AL_FALSE) ? NIX_CAP_AUDIO_STATIC_BUFFERS : 0;
				eng->maskCapabilities			|= (alIsExtensionPresent("AL_EXT_OFFSET")!=AL_FALSE) ? NIX_CAP_AUDIO_SOURCE_OFFSETS : 0;
				//Pregenerate reusable sources
				{
					NixUI16 sourcesCreated= 0;
					while(sourcesCreated < pregeneratedSources){
						ALuint idFuenteAL; alGenSources(1, &idFuenteAL); //el ID=0 es valido en OpenAL (no usar '0' para validar los IDs)
						if(alGetError()!=AL_NONE){
							break;
						} else {
							STNix_source audioSource;
							__nixSourceInit(&audioSource);
							audioSource.idSourceAL		= idFuenteAL;
							audioSource.isReusable		= NIX_TRUE;
							audioSource.retainCount		= 0; //Not retained by creator
							if(__nixSourceAdd(eng, &audioSource)==0){
								alDeleteSources(1, &idFuenteAL); VERIFICA_ERROR_AL("alDeleteSources");
								break;
							} else {
								sourcesCreated++;
							}
						}
					}
				}
				//
				return NIX_TRUE;
			}
		}
	}
	#elif defined(NIX_OPENSL)
	if(slCreateEngine(&eng->slObject, 0, NULL, 0, NULL, NULL)!=SL_RESULT_SUCCESS){
		PRINTF_ERROR("OpenSL::slCreateEngine failed.\n");
	} else {
		if((*eng->slObject)->Realize(eng->slObject, SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS){
			PRINTF_ERROR("OpenSL slObject->Realize() failed.\n");
		} else {
			if((*eng->slObject)->GetInterface(eng->slObject, SL_IID_ENGINE, &eng->slEngine)!=SL_RESULT_SUCCESS){
				PRINTF_ERROR("OpenSL slObject->GetInterface(SL_IID_ENGINE) failed.\n");
			} else {
				//const SLInterfaceID ids[] = {SL_IID_VOLUME /*SL_IID_ENVIRONMENTALREVERB*/ /*SL_IID_VOLUME*/};
				//const SLboolean req[] = {SL_BOOLEAN_TRUE};
				if((*eng->slEngine)->CreateOutputMix(eng->slEngine, &(eng->slOutputMixObject), 0, NULL, NULL)!=SL_RESULT_SUCCESS){
					PRINTF_ERROR("OpenSL slEngine->CreateOutputMix() failed.\n");
				} else {
					if((*eng->slOutputMixObject)->Realize(eng->slOutputMixObject, SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS){
						PRINTF_ERROR("OpenSL slOutputMixObject->Realize() failed.\n");
					} else {
						//Ignore pregenerated reusable sources
						PRINTF_INFO("OpenSL init OK.\n");
						return NIX_TRUE;
					}
				}
			}
		}
		if(eng->slOutputMixObject!=NULL) (*eng->slObject)->Destroy(eng->slOutputMixObject);
		if(eng->slObject!=NULL) (*eng->slObject)->Destroy(eng->slObject);
		eng->slObject			= NULL;
		eng->slEngine			= NULL;
		eng->slOutputMixObject	= NULL;
	}
	#endif //#ifdef NIX_OPENAL
	return NIX_FALSE;
}

void nixGetStatusDesc(STNix_Engine* engAbs, STNix_StatusDesc* dest){
	STNix_EngineObjetcs* eng	= (STNix_EngineObjetcs*)engAbs->o;
	NixUI16 i;
	const NixUI16 useSources	= eng->sourcesArrUse;
	const NixUI16 usePlayBuffs	= eng->buffersArrUse;
	const NixUI16 useRecBuffs	= eng->buffersCaptureArrSize;
	//Format destination
	dest->countSources			= 0;
	dest->countSourcesReusable	= 0;
	dest->countSourcesAssigned	= 0;
	dest->countSourcesStatic	= 0;
	dest->countSourcesStream	= 0;
	//
	dest->countPlayBuffers		= 0;
	dest->sizePlayBuffers		= 0;
	dest->sizePlayBuffersAtSW	= 0;
	//
	dest->countRecBuffers		= 0;
	dest->sizeRecBuffers		= 0;
	dest->sizeRecBuffersAtSW	= 0;
	//Read sources
	for(i=1; i<useSources; i++){
		STNix_source* source	= &eng->sourcesArr[i];
		if(source->regInUse){
			dest->countSources++;
			if(source->isReusable) dest->countSourcesReusable++;
			if(source->retainCount!=0) dest->countSourcesAssigned++;
			if(source->sourceType==ENNixSourceType_Static) dest->countSourcesStatic++;
			if(source->sourceType==ENNixSourceType_Stream) dest->countSourcesStream++;
		}
	}
	//Read play buffers
	for(i=1; i<usePlayBuffs; i++){
		STNix_bufferAL* buffer	= &eng->buffersArr[i];
		if(buffer->regInUse){
			dest->countPlayBuffers++;
			dest->sizePlayBuffers += buffer->bufferDesc.dataBytesCount;
			if(buffer->bufferDesc.dataPointer!=NULL) dest->sizePlayBuffersAtSW += buffer->bufferDesc.dataBytesCount;
		}
	}
	//Read capture buffers
	#ifdef NIX_OPENAL
	dest->sizeRecBuffers		+= eng->captureMainBufferBytesCount;
	#endif
	for(i=0; i<useRecBuffs; i++){
		STNix_bufferDesc* buffer = &eng->buffersCaptureArr[i];
		dest->countRecBuffers++;
		dest->sizeRecBuffers	+= buffer->dataBytesCount;
		if(buffer->dataPointer!=NULL) dest->sizeRecBuffersAtSW	+= buffer->dataBytesCount;
	}
}

NixUI32	nixCapabilities(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	return eng->maskCapabilities;
}

void nixPrintCaps(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->contextAL==NULL){
		ALCint versionMayorALC, versionMenorALC;
		const char* strAlVersion; 
		const char* strAlRenderer;
		const char* strAlVendor;
		const char* strAlExtensions;
		const char* strAlcExtensions;
		const char* defDeviceName;
		strAlVersion		= alGetString(AL_VERSION);	VERIFICA_ERROR_AL("alGetString(AL_VERSION)");
		strAlRenderer		= alGetString(AL_RENDERER);	VERIFICA_ERROR_AL("alGetString(AL_RENDERER)");
		strAlVendor			= alGetString(AL_VENDOR);	VERIFICA_ERROR_AL("alGetString(AL_VENDOR)");
		strAlExtensions		= alGetString(AL_EXTENSIONS);	VERIFICA_ERROR_AL("alGetString(AL_EXTENSIONS)");
		strAlcExtensions	= alcGetString(eng->deviceAL, ALC_EXTENSIONS); VERIFICA_ERROR_AL("alcGetString(ALC_EXTENSIONS)");
		alcGetIntegerv(eng->deviceAL, ALC_MAJOR_VERSION, sizeof(versionMayorALC), &versionMayorALC);
		alcGetIntegerv(eng->deviceAL, ALC_MINOR_VERSION, sizeof(versionMenorALC), &versionMenorALC);
		//
		PRINTF_INFO("----------- OPENAL -------------\n");
		PRINTF_INFO("Version:	      AL('%s') ALC(%d.%d):\n", strAlVersion, versionMayorALC, versionMenorALC);
		PRINTF_INFO("Renderizador:     '%s'\n", strAlRenderer);
		PRINTF_INFO("Vendedor:         '%s'\n", strAlVendor);
		PRINTF_INFO("EXTCaptura:       %s\n", (eng->maskCapabilities & NIX_CAP_AUDIO_CAPTURE)?"Soportado":"NO SOPORTADO");
		PRINTF_INFO("EXTBuffEstaticos: %s\n", (eng->maskCapabilities & NIX_CAP_AUDIO_STATIC_BUFFERS)?"Soportado":"NO SOPORTADO");
		PRINTF_INFO("EXTOffsets:       %s\n", (eng->maskCapabilities & NIX_CAP_AUDIO_SOURCE_OFFSETS)?"Soportado":"NO SOPORTADO");
		PRINTF_INFO("Extensiones AL:   '%s'\n", strAlExtensions);
		PRINTF_INFO("Extensiones ALC:  '%s'\n", strAlcExtensions);
		//List sound devices
		defDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER); VERIFICA_ERROR_AL("alcGetString(ALC_DEFAULT_DEVICE_SPECIFIER)")
		PRINTF_INFO("DefautlDevice:  '%s'\n", defDeviceName);
		{
			NixSI32 pos = 0, deviceCount = 0;
			const ALCchar* deviceList = alcGetString(NULL, ALC_DEVICE_SPECIFIER); VERIFICA_ERROR_AL("alcGetString(ALC_DEVICE_SPECIFIER)")
			while(deviceList[pos]!='\0'){
				const char* strDevice = &(deviceList[pos]); NixUI32 strSize = 0;
				while(strDevice[strSize]!='\0') strSize++;
				pos += strSize + 1; deviceCount++;
				PRINTF_INFO("Device #%d:  '%s'\n", deviceCount, strDevice);
			}
		}
		//List capture devices
		if(eng->maskCapabilities & NIX_CAP_AUDIO_CAPTURE){
			const char* defCaptureDeviceName = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER); VERIFICA_ERROR_AL("alcGetString(ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER)")
			PRINTF_INFO("DefautlCapture:  '%s'\n", defCaptureDeviceName);
			{
				NixSI32 pos = 0, deviceCount = 0;
				const ALCchar* deviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER); VERIFICA_ERROR_AL("alcGetString(ALC_CAPTURE_DEVICE_SPECIFIER)")
				while(deviceList[pos]!='\0'){
					const char* strDevice = &(deviceList[pos]); NixUI32 strSize = 0;
					while(strDevice[strSize]!='\0') strSize++;
					pos += strSize + 1; deviceCount++;
					PRINTF_INFO("Capture #%d:  '%s'\n", deviceCount, strDevice);
				}
			}
		}
		
	}
	#endif //#ifdef NIX_OPENAL
}

void nixFinalize(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NixUI16 buffersInUse = 0, sourcesInUse = 0;
	//Destroy capture
	nixCaptureFinalize(engAbs);
	//Destroy sources
	if(eng->sourcesArr!=NULL){
		NixUI16 i; const NixUI16 useCount = eng->sourcesArrUse;
		for(i=1; i<useCount; i++){ //Source index zero is reserved
			STNix_source* source = &eng->sourcesArr[i];
			if(source->regInUse){
				__nixSourceFinalize(engAbs, source, i, NIX_FALSE/*not for reuse*/);
				sourcesInUse++;
			}
		}
		NIX_FREE(eng->sourcesArr); eng->sourcesArr = NULL;
	}
	//Destroy buffers
	if(eng->buffersArr!=NULL){
		NixUI16 i; const NixUI16 useCount = eng->buffersArrUse;
		for(i=1; i<useCount; i++){
			STNix_bufferAL* buffer = &eng->buffersArr[i];
			if(buffer->regInUse){
				__nixBufferDestroy(buffer, i);
				buffersInUse++;
			}
		}
		NIX_FREE(eng->buffersArr); eng->buffersArr = NULL;
	}
	PRINTF_WARNING("nixFinalize, forced elimination of %d sources and %d buffers.\n", sourcesInUse, buffersInUse);
	//Destroy context and device
	#ifdef NIX_OPENAL
	if(alcMakeContextCurrent(NULL)==AL_FALSE){
		PRINTF_ERROR("alcMakeContextCurrent(NULL) failed\n");
	} else {
		alcDestroyContext(eng->contextAL); VERIFICA_ERROR_AL("alcDestroyContext");
		if(alcCloseDevice(eng->deviceAL)==AL_FALSE){
			PRINTF_ERROR("alcCloseDevice failed\n");
		} VERIFICA_ERROR_AL("alcCloseDevice");
	}
	#elif defined(NIX_OPENSL)
	//Output mix object
	if (eng->slOutputMixObject != NULL) {
		(*eng->slOutputMixObject)->Destroy(eng->slOutputMixObject);
		eng->slOutputMixObject = NULL;
	}
	//Engine Object
	if(eng->slObject!=NULL){
		(*eng->slObject)->Destroy(eng->slObject);
		eng->slObject = NULL;
		eng->slEngine = NULL;
	}
	#endif
	//
	NIX_FREE(eng)
	engAbs->o = NULL;
	
}

void nixGetContext(STNix_Engine* engAbs, void* dest){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	//The pointer must be a "ALCcontext**"
	memcpy(dest, &eng->contextAL, sizeof(eng->contextAL));
	#elif defined(NIX_OPENSL)
	//The pointer must be a "SLEngineItf*"
	memcpy(dest, &eng->slEngine, sizeof(eng->slEngine));
	#endif
}

void nixTick(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	//Delete buffers
	#ifdef NIX_MSWAIT_BEFORE_DELETING_BUFFERS
	{
		NixUI16 i; const NixUI16 use = eng->buffersArrUse;
		for(i=1; i<use; i++){
			STNix_bufferAL* buffer = &eng->buffersArr[i];
			if(buffer->regInUse && buffer->bufferDesc.state == ENNixBufferState_WaitingForDeletion){
				buffer->_msWaitingForDeletion += (1000 / 30);
				if(buffer->_msWaitingForDeletion >= NIX_MSWAIT_BEFORE_DELETING_BUFFERS){
					__nixBufferDestroy(buffer, i);
				}
			}
		}
	}
	#endif
	//
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL){
		__nixCaptureOpenALMoveSamplesToBuffers(engAbs); //OpenAL specific
	}
	__nixSreamsOpenALRemoveProcecedBuffers(engAbs); //OpenAL specific
	#elif defined(NIX_OPENSL)
	if(eng->buffersCaptureArrSize!=0 && eng->buffersCaptureArrFilledCount!=0 && eng->buffersCaptureCallback!=NULL){
		__nixCaptureOpenSLConsumeFilledBuffers(engAbs);
	}
	#endif
	//STREAMS: notifify recently played & unattached buffers.
	{
		NixUI16 iSrc; const NixUI16 useSrc = eng->sourcesArrUse;
		for(iSrc=1; iSrc<useSrc; iSrc++){ //Source index zero is reserved
			STNix_source* source = &eng->sourcesArr[iSrc];
			if(source->regInUse && source->retainCount!=0){
				//if(source->sourceType==ENNixSourceType_Stream){ //Do not validate 'sourceType' because when all buffers are removed from source, it is restored to 'typeUndefined', (ex: when a 'tick' takes too long)
					const NixUI16 unqueuedCount = source->buffStreamUnqueuedCount; //Create and evaluate a copy variable, in case another thread modify the value
					if(unqueuedCount != 0){
						PRINTF_INFO("Queue, removing ant invoquien callback for %d unqueued buffers.\n", unqueuedCount);
						//OpenSL specific, remove from queue (this allows execution thread control)
						#ifdef NIX_OPENSL
						__nixSrcQueueRemoveBuffersOldest(eng, source, iSrc, unqueuedCount);
						#endif
						//Callback
						if(source->bufferUnqueuedCallback!=NULL){ (*source->bufferUnqueuedCallback)(engAbs, source->bufferUnqueuedCallbackData, iSrc, unqueuedCount); }
						//
						NIX_ASSERT(unqueuedCount <= source->buffStreamUnqueuedCount)
						source->buffStreamUnqueuedCount -= unqueuedCount;
					}
				//}
			}
		}
	}
}

//------------------------------------------

NixUI16 nixSourceAssignStatic(STNix_Engine* engAbs, NixUI8 lookIntoReusable, NixUI8 audioGroupIndex, PTRNIX_SourceReleaseCallback releaseCallBack, void* releaseCallBackUserData){
	return __nixSourceAssign(engAbs, lookIntoReusable, audioGroupIndex, releaseCallBack, releaseCallBackUserData, 1/*queueSize*/, NULL, NULL);
}

NixUI16 nixSourceAssignStream(STNix_Engine* engAbs, NixUI8 lookIntoReusable, NixUI8 audioGroupIndex, PTRNIX_SourceReleaseCallback releaseCallBack, void* releaseCallBackUserData, const NixUI16 queueSize, PTRNIX_StreamBufferUnqueuedCallback bufferUnqueueCallback, void* bufferUnqueueCallbackData){
	return __nixSourceAssign(engAbs, lookIntoReusable, audioGroupIndex, releaseCallBack, releaseCallBackUserData, queueSize, bufferUnqueueCallback, bufferUnqueueCallbackData);
}

NixUI16 __nixSourceAssign(STNix_Engine* engAbs, NixUI8 lookIntoReusable, NixUI8 audioGroupIndex, PTRNIX_SourceReleaseCallback releaseCallBack, void* releaseCallBackUserData, const NixUI16 queueSize, PTRNIX_StreamBufferUnqueuedCallback bufferUnqueueCallback, void* bufferUnqueueCallbackData){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	//Search into reusable sources
	if(lookIntoReusable && eng->sourcesArr!=NULL){
		NixUI16 i; const NixUI16 useCount = eng->sourcesArrUse;
		for(i=1; i<useCount; i++){ //Source index zero is reserved
			STNix_source* source = &eng->sourcesArr[i];
			if(source->regInUse && source->retainCount==0 && source->isReusable){
				STNix_AudioGroup* grp		= &__nixSrcGroups[audioGroupIndex];
				NIX_ASSERT(source->sourceState==ENNixSourceState_Stopped)
				NIX_ASSERT(source->queueBuffIndexesUse==0)
				if(source->releaseCallBack!=NULL) (*source->releaseCallBack)(engAbs, source->releaseCallBackUserData, i);
				source->audioGroupIndex			= audioGroupIndex;
				source->releaseCallBack			= releaseCallBack;
				source->releaseCallBackUserData	= releaseCallBackUserData;
				source->bufferUnqueuedCallback		= bufferUnqueueCallback;
				source->bufferUnqueuedCallbackData	= bufferUnqueueCallbackData;
				#if defined(NIX_OPENSL)
				source->slQueueSize					= queueSize;
				#endif
				__nixSourceRetain(source);
				#ifdef NIX_OPENAL
				alSourcef(source->idSourceAL, AL_GAIN, source->volume * (grp->enabled ? grp->volume : 0.0f));
				VERIFICA_ERROR_AL("alSourcef(AL_GAIN)");
				#elif defined(NIX_OPENSL)
				if(source->volumeIntf!=NULL){
					(*source->volumeIntf)->SetVolumeLevel(source->volumeIntf, SL_MILLIBEL_MIN + ((source->volumeMax - SL_MILLIBEL_MIN) * source->volume * (grp->enabled ? grp->volume : 0.0f)));
				}
				#endif
				return i;
			}
		}
	}
	//Try to Generate new source
	#ifdef NIX_OPENAL
	{
		ALuint idFuenteAL; ALenum errorAL;
		alGenSources(1, &idFuenteAL);
		errorAL = alGetError();
		if(errorAL!=AL_NO_ERROR){
			PRINTF_ERROR("alGenSources failed with error #%d\n", (NixSI32)errorAL);
		} else {
			NixUI16 newSourceIndex;
			STNix_source audioSource;
			__nixSourceInit(&audioSource);
			audioSource.idSourceAL					= idFuenteAL;
			audioSource.audioGroupIndex				= audioGroupIndex;
			audioSource.releaseCallBack				= releaseCallBack;
			audioSource.releaseCallBackUserData		= releaseCallBackUserData;
			audioSource.bufferUnqueuedCallback		= bufferUnqueueCallback;
			audioSource.bufferUnqueuedCallbackData	= bufferUnqueueCallbackData;
			NIX_ASSERT(audioSource.regInUse)
			newSourceIndex = __nixSourceAdd(eng, &audioSource);
			if(newSourceIndex==0){
				alDeleteSources(1, &idFuenteAL); VERIFICA_ERROR_AL("alDeleteSources");
			} else {
				STNix_AudioGroup* grp		= &__nixSrcGroups[audioGroupIndex];
				alSourcef(audioSource.idSourceAL, AL_GAIN, audioSource.volume * (grp->enabled ? grp->volume : 0.0f));
				VERIFICA_ERROR_AL("alSourcef(AL_GAIN)");
				return newSourceIndex;
			}
		}
	}
	#elif defined(NIX_OPENSL)
	{
		NixUI16 newSourceIndex;
		STNix_source audioSource;
		__nixSourceInit(&audioSource);
		audioSource.slQueueSize					= queueSize;
		audioSource.audioGroupIndex				= audioGroupIndex;
		audioSource.releaseCallBack				= releaseCallBack;
		audioSource.releaseCallBackUserData		= releaseCallBackUserData;
		audioSource.bufferUnqueuedCallback		= bufferUnqueueCallback;
		audioSource.bufferUnqueuedCallbackData	= bufferUnqueueCallbackData;
		newSourceIndex = __nixSourceAdd(eng, &audioSource);
		if(newSourceIndex!=0){
			STNix_AudioGroup* grp		= &__nixSrcGroups[audioGroupIndex];
			if(audioSource.volumeIntf!=NULL){
				(*audioSource.volumeIntf)->SetVolumeLevel(audioSource.volumeIntf, SL_MILLIBEL_MIN + ((audioSource.volumeMax - SL_MILLIBEL_MIN) * audioSource.volume * (grp->enabled ? grp->volume : 0.0f)));
			}
			NIX_ASSERT(eng->sourcesArr[newSourceIndex].regInUse)
			return newSourceIndex;
		}
	}
	#endif //#elif defined(NIX_OPENSL)
	//
	return 0;
}

NixUI32	nixSourceRetainCount(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		return source->retainCount;
	} NIX_GET_SOURCE_END
	return 0;
}

void nixSourceRetain(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		__nixSourceRetain(source);
	} NIX_GET_SOURCE_END
}

void nixSourceRelease(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source){
		__nixSourceRelease(engAbs, source, sourceIndex);
	} NIX_GET_SOURCE_END
}

void nixSourceSetRepeat(STNix_Engine* engAbs, const NixUI16 sourceIndex, const NixBOOL repeat){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source){
		#ifdef NIX_OPENAL
		alSourcei(source->idSourceAL, AL_LOOPING, repeat?AL_TRUE:AL_FALSE); VERIFICA_ERROR_AL("alSourcei(AL_LOOPING)");
		#elif defined(NIX_OPENSL)
		source->repeat	= repeat;
		#endif
	} NIX_GET_SOURCE_END
}

NixUI32 nixSourceGetSamples(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	NixUI32 r = 0;
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		NixUI16 i; const NixUI16 use = source->queueBuffIndexesUse;
		for(i=0; i<use; i++){
			const NixUI16 iBuff = source->queueBuffIndexes[i];
			NIX_GET_BUFFER_START(eng, iBuff, buffer) {
				NIX_ASSERT(buffer->bufferDesc.audioDesc.blockAlign!=0)
				NIX_ASSERT(buffer->bufferDesc.audioDesc.samplerate!=0)
				if(buffer->bufferDesc.audioDesc.blockAlign!=0){
					r += (buffer->bufferDesc.dataBytesCount / buffer->bufferDesc.audioDesc.blockAlign);
				}
			} NIX_GET_BUFFER_END
		}
	} NIX_GET_SOURCE_END
	return r;
}

NixUI32 nixSourceGetBytes(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	NixUI32 r = 0;
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		NixUI16 i; const NixUI16 use = source->queueBuffIndexesUse;
		for(i=0; i<use; i++){
			const NixUI16 iBuff = source->queueBuffIndexes[i];
			NIX_GET_BUFFER_START(eng, iBuff, buffer) {
				NIX_ASSERT(buffer->bufferDesc.audioDesc.blockAlign!=0)
				NIX_ASSERT(buffer->bufferDesc.audioDesc.samplerate!=0)
				if(buffer->bufferDesc.audioDesc.blockAlign!=0){
					r += buffer->bufferDesc.dataBytesCount;
				}
			} NIX_GET_BUFFER_END
		}
	} NIX_GET_SOURCE_END
	return r;
}

NixFLOAT nixSourceGetSeconds(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	NixFLOAT r = 0.0f;
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		NixUI16 i; const NixUI16 use = source->queueBuffIndexesUse;
		for(i=0; i<use; i++){
			const NixUI16 iBuff = source->queueBuffIndexes[i];
			NIX_GET_BUFFER_START(eng, iBuff, buffer) {
				NIX_ASSERT(buffer->bufferDesc.audioDesc.blockAlign!=0)
				NIX_ASSERT(buffer->bufferDesc.audioDesc.samplerate!=0)
				if(buffer->regInUse && buffer->bufferDesc.audioDesc.blockAlign!=0 && buffer->bufferDesc.audioDesc.samplerate!=0){
					r += ((float)buffer->bufferDesc.dataBytesCount / (float)buffer->bufferDesc.audioDesc.blockAlign) / (float)buffer->bufferDesc.audioDesc.samplerate;
				}
			} NIX_GET_BUFFER_END
		}
	} NIX_GET_SOURCE_END
	return r;
}

NixFLOAT nixSourceGetVoume(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		return source->volume;
	} NIX_GET_SOURCE_END
	return 0.0f;
}

void nixSourceSetVolume(STNix_Engine* engAbs, const NixUI16 sourceIndex, const float volume){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		STNix_AudioGroup* grp = &__nixSrcGroups[source->audioGroupIndex];
		source->volume = volume;
		//PENDIENTE HABILITAR GRUPOS DE AUDIO
		//NBASSERT(fuenteAL->_indiceGrupoAudio>=0 && fuenteAL->_indiceGrupoAudio<NBAUDIO_CONTEO_GRUPOS_AUDIO)
		#ifdef NIX_OPENAL
		alSourcef(source->idSourceAL, AL_GAIN, volume * (grp->enabled ? grp->volume : 0.0f));
		VERIFICA_ERROR_AL("alSourcef(AL_GAIN)");
		#elif defined(NIX_OPENSL)
		if(source->volumeIntf!=NULL){
			(*source->volumeIntf)->SetVolumeLevel(source->volumeIntf, SL_MILLIBEL_MIN + ((source->volumeMax - SL_MILLIBEL_MIN) * volume * (grp->enabled ? grp->volume : 0.0f)));
		}
		#endif
	} NIX_GET_SOURCE_END
}

NixUI16 nixSourceGetBuffersCount(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		return source->queueBuffIndexesUse;
	} NIX_GET_SOURCE_END
	return 0;
}

NixUI32 nixSourceGetOffsetSamples(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	NixUI32 r = 0;
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		if(source->queueBuffIndexesUse!=0){
			ALint offset; alGetSourcei(source->idSourceAL, AL_SAMPLE_OFFSET, &offset); VERIFICA_ERROR_AL("alGetSourcei(AL_SAMPLE_OFFSET)");
			r = offset;
		}
		#endif
	} NIX_GET_SOURCE_END
	return r;
}

NixUI32 nixSourceGetOffsetBytes(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	NixUI32 r = 0;
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		if(source->queueBuffIndexesUse!=0){
			ALint offset; alGetSourcei(source->idSourceAL, AL_BYTE_OFFSET, &offset); VERIFICA_ERROR_AL("alGetSourcei(AL_BYTE_OFFSET)");
			r = offset;
		}
		#endif
	} NIX_GET_SOURCE_END
	return r;
}

void nixSourceSetOffsetSamples(STNix_Engine* engAbs, const NixUI16 sourceIndex, const NixUI32 offsetSamples){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		if(source->queueBuffIndexesUse!=0){
			ALint offset = offsetSamples; alSourcei(source->idSourceAL, AL_SAMPLE_OFFSET, offset); VERIFICA_ERROR_AL("alGetSourcei(AL_SAMPLE_OFFSET)");
		}
		#endif
	} NIX_GET_SOURCE_END
}

void nixSourcePlay(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		if(source->queueBuffIndexesUse!=0){
			alSourcePlay(source->idSourceAL);	VERIFICA_ERROR_AL("alSourcePlay");
		}
		#elif defined(NIX_OPENSL)
		if(source->slPlayerIntf!=NULL && source->queueBuffIndexesUse!=0){
			if((*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_PLAYING)!=SL_RESULT_SUCCESS){
				NIX_ASSERT(NIX_FALSE)
			}
		}
		#endif
		source->sourceState = ENNixSourceState_Playing;
	} NIX_GET_SOURCE_END
}

NixBOOL nixSourceIsPlaying(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		ALint sourceState;
		alGetSourcei(source->idSourceAL, AL_SOURCE_STATE, &sourceState);	VERIFICA_ERROR_AL("alGetSourcei(AL_SOURCE_STATE)");
		return (sourceState==AL_PLAYING ? NIX_TRUE : NIX_FALSE);
		#elif defined(NIX_OPENSL)
		if(source->slPlayerIntf!=NULL){
			SLuint32 playerState;
			if((*source->slPlayerIntf)->GetPlayState(source->slPlayerIntf, &playerState)==SL_RESULT_SUCCESS){ //TODO: 'GetPlayState' slows down the engine?
				return (playerState == SL_PLAYSTATE_PLAYING? NIX_TRUE : NIX_FALSE);
			}
		}
		#endif
	} NIX_GET_SOURCE_END
	return NIX_FALSE;
}

void nixSourcePause(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		alSourcePause(source->idSourceAL);	VERIFICA_ERROR_AL("alSourcePause");
		#elif defined(NIX_OPENSL)
		if(source->slPlayerIntf != NULL){
			(*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_PAUSED);
		}
		#endif
		source->sourceState = ENNixSourceState_Playing;
	} NIX_GET_SOURCE_END
}

void nixSourceStop(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		alSourceStop(source->idSourceAL); VERIFICA_ERROR_AL("alSourceStop");
		#elif defined(NIX_OPENSL)
		if(source->slPlayerIntf != NULL){
			(*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_STOPPED);
		}
		#endif
		source->sourceState = ENNixSourceState_Stopped;
	} NIX_GET_SOURCE_END
}

void nixSourceRewind(STNix_Engine* engAbs, const NixUI16 sourceIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		#ifdef NIX_OPENAL
		alSourceRewind(source->idSourceAL); VERIFICA_ERROR_AL("alSourceRewind");
		#endif
	} NIX_GET_SOURCE_END
}

NixBOOL nixSourceSetBuffer(STNix_Engine* engAbs, const NixUI16 sourceIndex, const NixUI16 bufferIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	PRINTF_INFO("Queuing source(%d) buffer(%d).\n", sourceIndex, bufferIndex);
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		__nixSrcQueueClear(eng, source, sourceIndex);
		if(bufferIndex==0){
			//Set buffer to NONE (default if the above failed)
			#ifdef NIX_OPENAL
			alSourcei(source->idSourceAL, AL_BUFFER, AL_NONE); VERIFICA_ERROR_AL("alSourcei(AL_BUFFER)");
			#endif
			return NIX_TRUE;
		} else {
			NIX_GET_BUFFER_START(eng, bufferIndex, buffer) {
				#ifdef NIX_OPENAL
				//Set buffer
				alSourcei(source->idSourceAL, AL_BUFFER, buffer->idBufferAL);
				if(alGetError()!=AL_NONE){
					NIX_ASSERT(NIX_FALSE)
				} else {
					__nixSrcQueueSetUniqueBuffer(source, buffer, bufferIndex); PRINTF_INFO("Queued STATIC source(%d) buffer(%d) (%d in queue).\n", sourceIndex, bufferIndex, source->queueBuffIndexesUse);
					//Play if its playing
					if(source->sourceState == ENNixSourceState_Playing){
						ALint sourceState;
						alGetSourcei(source->idSourceAL, AL_SOURCE_STATE, &sourceState);	VERIFICA_ERROR_AL("alGetSourcei(AL_SOURCE_STATE)");
						if(sourceState!=AL_PLAYING){
							alSourcePlay(source->idSourceAL);	VERIFICA_ERROR_AL("alSourcePlay");
						}
					}
					return NIX_TRUE;
				}
				#elif defined(NIX_OPENSL)
				//Init source (if needed)
				if(source->slPlayerObject==NULL){
					if(__nixSourceInitOpenSLPlayer(eng, sourceIndex, source, &buffer->bufferDesc.audioDesc)==NIX_FALSE){
						PRINTF_ERROR("__nixSourceInitOpenSLPlayer failed.\n");
					}
				}
				//Verify source and buffer properties
				if(source->slPlayerObject!=NULL){
					NIX_ASSERT(buffer->bufferDesc.audioDesc.channels==source->sourceFormat.channels && buffer->bufferDesc.audioDesc.bitsPerSample==source->sourceFormat.bitsPerSample && buffer->bufferDesc.audioDesc.samplerate==source->sourceFormat.samplerate)
					if(buffer->bufferDesc.audioDesc.channels==source->sourceFormat.channels && buffer->bufferDesc.audioDesc.bitsPerSample==source->sourceFormat.bitsPerSample && buffer->bufferDesc.audioDesc.samplerate==source->sourceFormat.samplerate){
						NIX_ASSERT(buffer->bufferDesc.dataBytesCount!=0 && buffer->bufferDesc.dataPointer!=NULL)
						if(buffer->bufferDesc.dataBytesCount!=0 && buffer->bufferDesc.dataPointer!=NULL){
							//Queue bufferIndex (it is important to add the bufferIndex before 'sl->Enqueue' because the callbacks)
							__nixSrcQueueSetUniqueBuffer(source, buffer, bufferIndex); PRINTF_INFO("Queued STATIC source(%d) buffer(%d) (%d in queue).\n", sourceIndex, bufferIndex, source->queueBuffIndexesUse);
							//Queue slBuffer
							if((*source->slPlayerBufferQueue)->Enqueue(source->slPlayerBufferQueue, buffer->bufferDesc.dataPointer, buffer->bufferDesc.dataBytesCount)!=SL_RESULT_SUCCESS){
								PRINTF_ERROR("OpenSL slPlayerBufferQueue->Enqueue(buffer #%d of) failed.\n", (source->queueBuffIndexesUse + 1), source->slQueueSize);
								__nixSrcQueueRemoveBuffersNewest(eng, source, sourceIndex, 1);
							} else {
								//Play if its playing
								if(source->sourceState == ENNixSourceState_Playing){
									SLuint32 playerState;
									if((*source->slPlayerIntf)->GetPlayState(source->slPlayerIntf, &playerState)!=SL_RESULT_SUCCESS){ //TODO: 'GetPlayState' slows down the engine?
										PRINTF_ERROR("OpenSL slPlayerIntf->GetPlayState() failed.\n");
										NIX_ASSERT(NIX_FALSE)
									} else if(playerState!=SL_PLAYSTATE_PLAYING){
										if((*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_PLAYING)!=SL_RESULT_SUCCESS){
											PRINTF_ERROR("OpenSL slPlayerIntf->SetPlayState(SL_PLAYSTATE_PLAYING) failed.\n");
										}
									}
								}
								return NIX_TRUE;
							}
						}
					}
				}
				#endif
			} NIX_GET_BUFFER_END
		} //if(bufferIndex==0){
	} NIX_GET_SOURCE_END
	return NIX_FALSE;
}

NixBOOL nixSourceStreamAppendBuffer(STNix_Engine* engAbs, const NixUI16 sourceIndex, const NixUI16 streamBufferIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		NIX_GET_BUFFER_START(eng, streamBufferIndex, buffer) {
			NIX_ASSERT(buffer->bufferDesc.state==ENNixBufferState_LoadedForPlay)
			if(buffer->bufferDesc.state == ENNixBufferState_LoadedForPlay){
				#ifdef NIX_OPENAL
				ALenum errorAL;
				alSourceQueueBuffers(source->idSourceAL, 1, &buffer->idBufferAL);
				errorAL = alGetError();
				if(errorAL!=AL_NONE){
					PRINTF_ERROR("ERROR '%s' alSourceQueueBuffers\n", STR_ERROR_AL(errorAL)); NIX_ASSERT(0)
				} else {
					__nixSrcQueueAddStreamBuffer(source, buffer, streamBufferIndex); PRINTF_INFO("Queued STREAM source(%d) buffer(%d) (%d in queue).\n", sourceIndex, streamBufferIndex, source->queueBuffIndexesUse);
					if(source->sourceState == ENNixSourceState_Playing){
						//Play if its playing
						ALint sourceState;
						alGetSourcei(source->idSourceAL, AL_SOURCE_STATE, &sourceState);	VERIFICA_ERROR_AL("alGetSourcei(AL_SOURCE_STATE)");
						if(sourceState!=AL_PLAYING){
							alSourcePlay(source->idSourceAL);	VERIFICA_ERROR_AL("alSourcePlay");
						}
					}
					return NIX_TRUE;
				}
				#elif defined(NIX_OPENSL)
				//Init source (if needed)
				if(source->slPlayerObject==NULL){
					if(__nixSourceInitOpenSLPlayer(eng, sourceIndex, source, &buffer->bufferDesc.audioDesc)==NIX_FALSE){
						PRINTF_ERROR("__nixSourceInitOpenSLPlayer failed.\n");
					}
				}
				//Verify source and buffer properties
				if(source->slPlayerObject!=NULL){
					NIX_ASSERT(buffer->bufferDesc.audioDesc.channels==source->sourceFormat.channels && buffer->bufferDesc.audioDesc.bitsPerSample==source->sourceFormat.bitsPerSample && buffer->bufferDesc.audioDesc.samplerate==source->sourceFormat.samplerate)
					if(buffer->bufferDesc.audioDesc.channels==source->sourceFormat.channels && buffer->bufferDesc.audioDesc.bitsPerSample==source->sourceFormat.bitsPerSample && buffer->bufferDesc.audioDesc.samplerate==source->sourceFormat.samplerate){
						NIX_ASSERT(buffer->bufferDesc.dataBytesCount!=0 && buffer->bufferDesc.dataPointer!=NULL)
						if(buffer->bufferDesc.dataBytesCount!=0 && buffer->bufferDesc.dataPointer!=NULL){
							//Queue bufferIndex (it is important to add the bufferIndex before 'sl->Enqueue' because the callbacks)
							__nixSrcQueueAddStreamBuffer(source, buffer, streamBufferIndex); PRINTF_INFO("Queued STREAM source(%d) buffer(%d) (%d in queue).\n", sourceIndex, streamBufferIndex, source->queueBuffIndexesUse);
							//Queue slBuffer
							if((*source->slPlayerBufferQueue)->Enqueue(source->slPlayerBufferQueue, buffer->bufferDesc.dataPointer, buffer->bufferDesc.dataBytesCount)!=SL_RESULT_SUCCESS){
								PRINTF_WARNING("OpenSL slPlayerBufferQueue->Enqueue(buffer #%d of %d) failed.\n", (source->queueBuffIndexesUse + 1), source->slQueueSize);
								__nixSrcQueueRemoveBuffersNewest(eng, source, sourceIndex, 1);
							} else {
								//Play if its playing
								if(source->sourceState == ENNixSourceState_Playing){
									SLuint32 playerState;
									if((*source->slPlayerIntf)->GetPlayState(source->slPlayerIntf, &playerState)!=SL_RESULT_SUCCESS){ //TODO: 'GetPlayState' slows down the engine?
										PRINTF_ERROR("OpenSL slPlayerIntf->GetPlayState() failed.\n");
										NIX_ASSERT(NIX_FALSE)
									} else if(playerState!=SL_PLAYSTATE_PLAYING){
										if((*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_PLAYING)!=SL_RESULT_SUCCESS){
											PRINTF_ERROR("OpenSL slPlayerIntf->SetPlayState(SL_PLAYSTATE_PLAYING) failed.\n");
										}
									}
								}
								return NIX_TRUE;
							}
						}
					}
				}
				#endif
			}
		} NIX_GET_BUFFER_END
	} NIX_GET_SOURCE_END
	return NIX_FALSE;
}

NixBOOL nixSourceHaveBuffer(STNix_Engine* engAbs, const NixUI16 sourceIndex, const NixUI16 bufferIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NBASSERT(bufferIndex > 0)
	NBASSERT(bufferIndex < source->buffersArrUse)
	NIX_GET_SOURCE_START(eng, sourceIndex, source) {
		UI16 i; const UI16 use = source->queueBuffIndexesUse;
		for(i=0; i<use; i++){
			NBASSERT(source->queueBuffIndexes[i] > 0)
			NBASSERT(source->queueBuffIndexes[i] < source->buffersArrUse)
			if(source->queueBuffIndexes[i] == bufferIndex) return NIX_TRUE;
		}
	} NIX_GET_SOURCE_END
	return NIX_FALSE;
}

//--------------------
//--  Audio groups  --
//--------------------

NixBOOL	nixSrcGroupIsEnabled(STNix_Engine* engAbs, const NixUI8 groupIndex){
	NIX_ASSERT(groupIndex < NIX_AUDIO_GROUPS_SIZE)
	if(groupIndex < NIX_AUDIO_GROUPS_SIZE){
		return __nixSrcGroups[groupIndex].enabled;
	}
	return NIX_FALSE;
}

NixFLOAT nixSrcGroupGetVolume(STNix_Engine* engAbs, const NixUI8 groupIndex){
	NIX_ASSERT(groupIndex < NIX_AUDIO_GROUPS_SIZE)
	if(groupIndex < NIX_AUDIO_GROUPS_SIZE){
		return __nixSrcGroups[groupIndex].volume;
	}
	return 0.0f;
}

void nixSrcGroupSetEnabled(STNix_Engine* engAbs, const NixUI8 groupIndex, const NixBOOL enabled){
	NIX_ASSERT(groupIndex < NIX_AUDIO_GROUPS_SIZE)
	if(groupIndex < NIX_AUDIO_GROUPS_SIZE){
		STNix_AudioGroup* grp		= &__nixSrcGroups[groupIndex];
		STNix_EngineObjetcs* eng	= (STNix_EngineObjetcs*)engAbs->o;
		NixUI16 i; const NixUI16 use = eng->sourcesArrUse;
		for(i=0; i<use; i++){
			STNix_source* source = &eng->sourcesArr[i];
			if(source->regInUse && source->audioGroupIndex==groupIndex){
				#ifdef NIX_OPENAL
				alSourcef(source->idSourceAL, AL_GAIN, source->volume * (enabled ? grp->volume : 0.0f));
				VERIFICA_ERROR_AL("alSourcef(AL_GAIN)");
				#elif defined(NIX_OPENSL)
				if(source->volumeIntf!=NULL){
					(*source->volumeIntf)->SetVolumeLevel(source->volumeIntf, SL_MILLIBEL_MIN + ((source->volumeMax - SL_MILLIBEL_MIN) * source->volume * (enabled ? grp->volume : 0.0f)));
				}
				#endif
			}
		}
		grp->enabled = enabled;
	}
}

void nixSrcGroupSetVolume(STNix_Engine* engAbs, const NixUI8 groupIndex, const NixFLOAT volume){
	NIX_ASSERT(groupIndex < NIX_AUDIO_GROUPS_SIZE)
	if(groupIndex < NIX_AUDIO_GROUPS_SIZE){
		STNix_AudioGroup* grp		= &__nixSrcGroups[groupIndex];
		if(grp->enabled){
			STNix_EngineObjetcs* eng	= (STNix_EngineObjetcs*)engAbs->o;
			NixUI16 i; const NixUI16 use = eng->sourcesArrUse;
			for(i=0; i<use; i++){
				STNix_source* source = &eng->sourcesArr[i];
				if(source->regInUse && source->audioGroupIndex==groupIndex){
					#ifdef NIX_OPENAL
					alSourcef(source->idSourceAL, AL_GAIN, source->volume * volume);
					VERIFICA_ERROR_AL("alSourcef(AL_GAIN)");
					#elif defined(NIX_OPENSL)
					if(source->volumeIntf!=NULL){
						(*source->volumeIntf)->SetVolumeLevel(source->volumeIntf, SL_MILLIBEL_MIN + ((source->volumeMax - SL_MILLIBEL_MIN) * source->volume * volume));
					}
					#endif
				}
			}
		}
		grp->volume = volume;
	}
}


#ifdef NIX_OPENSL
void __nixSourceBufferQueueSLCallback(SLAndroidSimpleBufferQueueItf bq, void* pParam){
	STNix_OpenSLSourceCallbackParam* param = (STNix_OpenSLSourceCallbackParam*)pParam;
	STNix_EngineObjetcs* eng = param->eng; NIX_ASSERT(eng!=NULL)
	NIX_GET_SOURCE_START(eng, param->sourceIndex, source){
		NIX_ASSERT(source->queueBuffIndexesSize!=0)
		if(source->queueBuffIndexesSize!=0){
			switch (source->sourceType) {
				case ENNixSourceType_Stream:
					//STREAM, release oldest buffer
					source->buffStreamUnqueuedCount++; //This number will be processed at 'nixTick'
					break;
				case ENNixSourceType_Static:
					{ //Append again the same buffer
						SLAndroidSimpleBufferQueueState bufQueueState;
						if((*source->slPlayerBufferQueue)->GetState(source->slPlayerBufferQueue, &bufQueueState)!=SL_RESULT_SUCCESS){
							PRINTF_ERROR("OpenSL slPlayerBufferQueue->GetState() failed.\n");
						} else {
							NIX_ASSERT(bufQueueState.count==0)
							if(bufQueueState.count==0){
								const NixUI16 firtBuffIndex = source->queueBuffIndexes[0];
								NIX_GET_BUFFER_START(eng, firtBuffIndex, buff){
									NIX_ASSERT(buff->bufferDesc.dataBytesCount!=0 && buff->bufferDesc.dataPointer!=NULL)
									if(buff->bufferDesc.dataBytesCount!=0 && buff->bufferDesc.dataPointer!=NULL){
										if((*source->slPlayerBufferQueue)->Enqueue(source->slPlayerBufferQueue, buff->bufferDesc.dataPointer, buff->bufferDesc.dataBytesCount)!=SL_RESULT_SUCCESS){
											PRINTF_ERROR("OpenSL slPlayerBufferQueue->Enqueue(FisrtBuffer) failed.\n");
										} else {
											SLuint32 playerState;
											PRINTF_INFO("Queued(re) Static SrcIndex(%d) Type(%d) Buffer.\n", param->sourceIndex, source->sourceType);
											if((*source->slPlayerIntf)->GetPlayState(source->slPlayerIntf, &playerState)!=SL_RESULT_SUCCESS){ //TODO: 'GetPlayState' slows down the engine?
												PRINTF_ERROR("OpenSL slPlayerIntf->GetPlayState() failed.\n"); NIX_ASSERT(NIX_FALSE)
											} else {
												if(source->repeat && source->sourceState==ENNixSourceState_Playing){
													if(playerState!=SL_PLAYSTATE_PLAYING){
														if((*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_PLAYING)!=SL_RESULT_SUCCESS){
															PRINTF_ERROR("OpenSL slPlayerIntf->SetPlayState(SL_PLAYSTATE_PLAYING) failed.\n"); NIX_ASSERT(0)
														}
													}
												} else {
													if(playerState!=SL_PLAYSTATE_STOPPED){
														if((*source->slPlayerIntf)->SetPlayState(source->slPlayerIntf, SL_PLAYSTATE_STOPPED)!=SL_RESULT_SUCCESS){
															PRINTF_ERROR("OpenSL slPlayerIntf->SetPlayState(SL_PLAYSTATE_STOPPED) failed.\n"); NIX_ASSERT(0)
														}
													}
													source->sourceState = ENNixSourceState_Stopped;
												}
											}
										}
									}
								} NIX_GET_BUFFER_END
							}
						}
					}
					break;
				default:
					NIX_ASSERT(NIX_FALSE)
					break;
			}
		}
	} NIX_GET_SOURCE_END
}
#endif

#ifdef NIX_OPENSL //OpenSL specific
NixBOOL __nixSourceInitOpenSLPlayer(STNix_EngineObjetcs* eng, const NixUI16 sourceIndex, STNix_source* src, STNix_audioDesc* audioDesc){
	NixUI32 slFormatBitPerSample	= NIX_OPENSL_AUDIO_SAMPLE_FORMAT(audioDesc->bitsPerSample);
	NixUI32 slFormatSampleRate		= audioDesc->samplerate * 1000;
	PRINTF_INFO("INIT SOURCE with channels(%d) bitsPerSample(%d) samplerate(%d) blockAlign(%d).\n", audioDesc->channels, audioDesc->bitsPerSample, audioDesc->samplerate, audioDesc->blockAlign);
	NIX_ASSERT(src->slPlayerObject==NULL)
	NIX_ASSERT(src->slPlayerIntf==NULL)
	NIX_ASSERT(src->slPlayerBufferQueue==NULL)
	NIX_ASSERT(src->callbackParam==NULL)
	if(slFormatBitPerSample!=0 && slFormatSampleRate!=0){
		SLObjectItf slPlayerObject	= NULL;
		SLPlayItf slPlayerIntf		= NULL;
		SLVolumeItf slVolumeIntf	= NULL;
		SLmillibel slVolumeMax		= 0;
		SLAndroidSimpleBufferQueueItf slPlayerBufferQueue = NULL;
		//
		int speakers	= (audioDesc->channels == 1) ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
		SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, audioDesc->channels, slFormatSampleRate, slFormatBitPerSample, slFormatBitPerSample, speakers, SL_BYTEORDER_LITTLEENDIAN};
		SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, src->slQueueSize };
		SLDataSource audioSrc = {&loc_bufq, &format_pcm};
		// configure audio sink
		SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, eng->slOutputMixObject};
		SLDataSink audioSnk = {&loc_outmix, NULL};
		// create audio player
		const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
		const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
		if((*eng->slEngine)->CreateAudioPlayer(eng->slEngine, &slPlayerObject, &audioSrc, &audioSnk, 2, ids, req)!=SL_RESULT_SUCCESS){
			PRINTF_ERROR("OpenSL slEngine->CreateAudioPlayer() failed.\n");
		} else {
			if((*slPlayerObject)->Realize(slPlayerObject, SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS){
				PRINTF_ERROR("OpenSL slPlayerObject->Realize() failed.\n");
			} else{
				if((*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_PLAY, &(slPlayerIntf))!=SL_RESULT_SUCCESS){
					PRINTF_ERROR("OpenSL slPlayerObject->GetInterface(SL_IID_PLAY) failed.\n");
				} else {
					if((*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_VOLUME, &(slVolumeIntf))!=SL_RESULT_SUCCESS){
						PRINTF_ERROR("OpenSL slPlayerObject->GetInterface(SL_IID_VOLUME) failed.\n");
					} else {
						if((*slVolumeIntf)->GetMaxVolumeLevel(slVolumeIntf, &(slVolumeMax))!=SL_RESULT_SUCCESS){
							PRINTF_ERROR("OpenSL slVolumeIntf->GetMaxVolumeLevel() failed.\n");
						} else {
							if((*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(slPlayerBufferQueue))!=SL_RESULT_SUCCESS){
								PRINTF_ERROR("OpenSL slPlayerObject->GetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE) failed.\n");
							} else {
								STNix_OpenSLSourceCallbackParam* callbackParam;
								NIX_MALLOC(callbackParam, STNix_OpenSLSourceCallbackParam, sizeof(STNix_OpenSLSourceCallbackParam), "STNix_OpenSLSourceCallbackParam")
								callbackParam->eng			= eng;
								callbackParam->sourceIndex	= sourceIndex;
								if((*slPlayerBufferQueue)->RegisterCallback(slPlayerBufferQueue, __nixSourceBufferQueueSLCallback, callbackParam)!=SL_RESULT_SUCCESS){
									PRINTF_ERROR("OpenSL slPlayerBufferQueue->RegisterCallback() failed.\n");
								} else {
									STNix_AudioGroup* grp		= &__nixSrcGroups[src->audioGroupIndex];
									src->slPlayerObject			= slPlayerObject;
									src->slPlayerIntf			= slPlayerIntf;
									src->volumeIntf				= slVolumeIntf;
									src->volumeMax				= slVolumeMax;
									src->slPlayerBufferQueue	= slPlayerBufferQueue;
									src->sourceFormat			= *audioDesc;
									if(src->volumeIntf!=NULL){
										(*src->volumeIntf)->SetVolumeLevel(src->volumeIntf, SL_MILLIBEL_MIN + ((src->volumeMax - SL_MILLIBEL_MIN) * src->volume * (grp->enabled ? grp->volume : 0.0f)));
									}
									PRINTF_INFO("source OpenSLObject created(%d).\n", sourceIndex);
									return NIX_TRUE;
								}
								NIX_FREE(callbackParam); //cleanup when init fails
							}
						}
					}
				}
				(*slPlayerObject)->Destroy(slPlayerObject); //cleanup when init fails
			}
		}
		return NIX_FALSE;
	}
}
#endif


#ifdef NIX_OPENAL
void __nixSreamsOpenALRemoveProcecedBuffers(STNix_Engine* engAbs){ //OpenAL specific
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NixUI16 i; const NixUI16 useCount = eng->sourcesArrUse;
	for(i=1; i<useCount; i++){ //Source index zero is reserved
		STNix_source* source = &eng->sourcesArr[i];
		if(source->regInUse && source->queueBuffIndexesUse!=0){
			NIX_ASSERT(source->retainCount!=0)
			if(source->sourceType==ENNixSourceType_Stream){
				ALint buffersProcessedCount = 0;
				alGetSourceiv(source->idSourceAL, AL_BUFFERS_PROCESSED, &buffersProcessedCount); VERIFICA_ERROR_AL("alGetSourceiv(AL_BUFFERS_PROCESSED)");
				NIX_ASSERT(buffersProcessedCount <= source->queueBuffIndexesUse) //Just checking, this should be always be true
				if(buffersProcessedCount > 0){
					//Remove from source's bufferIndex array
					__nixSrcQueueRemoveBuffersOldest(eng, source, i, buffersProcessedCount);
					source->buffStreamUnqueuedCount += buffersProcessedCount; PRINTF_INFO("AL source->buffStreamUnqueuedCount = %d\n", source->buffStreamUnqueuedCount);
				}
			}
		}
	}
}
#endif

//------------------------------------------

NixUI16 nixBufferWithData(STNix_Engine* engAbs, const STNix_audioDesc* audioDesc, const NixUI8* audioDataPCM, const NixUI32 audioDataPCMBytes){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NixUI16 i; const NixUI16 useCount = eng->buffersArrUse;
	//NIX_ASSERT(eng->buffersArrUse < 25) //TEMPORAL
	NIX_ASSERT(audioDesc->blockAlign == ((audioDesc->bitsPerSample / 8) * audioDesc->channels))
	//Look for a available existent buffer
	/*{
		for(i=1; i<useCount; i++){ //Buffer index zero is reserved
			STNix_bufferAL* buffer = &eng->buffersArr[i];
			if(buffer->regInUse && buffer->bufferDesc.state == ENNixBufferState_Free){
				if(__nixBufferSetData(eng, buffer, audioDesc, audioDataPCM, audioDataPCMBytes)){
					__nixBufferRetain(buffer);
					return i;
				}
			}
		}
	}*/
	//Create a new buffer
	{
		const NixUI16 iBuff = __nixBufferCreate(eng);
		if(iBuff != 0){
			if(__nixBufferSetData(eng, &eng->buffersArr[iBuff], audioDesc, audioDataPCM, audioDataPCMBytes)){
				return iBuff;
			} else {
				__nixBufferRelease(&eng->buffersArr[iBuff], iBuff);
			}
		}
	}
	return 0;
}

NixBOOL nixBufferSetData(STNix_Engine* engAbs, const NixUI16 buffIndex, const STNix_audioDesc* audioDesc, const NixUI8* audioDataPCM, const NixUI32 audioDataPCMBytes){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_BUFFER_START(eng, buffIndex, buffer) {
		return __nixBufferSetData(eng, buffer, audioDesc, audioDataPCM, audioDataPCMBytes);
	} NIX_GET_BUFFER_END
	return NIX_FALSE;
}

NixUI32	nixBufferRetainCount(STNix_Engine* engAbs, const NixUI16 buffIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_BUFFER_START(eng, buffIndex, buffer) {
		return buffer->retainCount;
	} NIX_GET_BUFFER_END
	return 0;
}

void nixBufferRetain(STNix_Engine* engAbs, const NixUI16 buffIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_BUFFER_START(eng, buffIndex, buffer) {
		__nixBufferRetain(buffer);
	} NIX_GET_BUFFER_END
}

void nixBufferRelease(STNix_Engine* engAbs, const NixUI16 buffIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_GET_BUFFER_START(eng, buffIndex, buffer) {
		__nixBufferRelease(buffer, buffIndex);
	} NIX_GET_BUFFER_END
}

float nixBufferSeconds(STNix_Engine* engAbs, const NixUI16 buffIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	float r = 0.0f;
	NIX_ASSERT(buffIndex!=0)
	if(buffIndex!=0){
		NIX_ASSERT(buffIndex < eng->buffersArrUse)
		if(buffIndex < eng->buffersArrUse){
			NIX_ASSERT(eng->buffersArr[buffIndex].regInUse)
			if(eng->buffersArr[buffIndex].regInUse){
				STNix_bufferDesc* bufferDesc = &eng->buffersArr[buffIndex].bufferDesc;
				STNix_audioDesc* audioDesc = &bufferDesc->audioDesc;
				if(bufferDesc->dataBytesCount!=0 && audioDesc->bitsPerSample!=0 && audioDesc->channels!=0 && audioDesc->samplerate!=0){
					r = (float)bufferDesc->dataBytesCount / (float)(audioDesc->samplerate * audioDesc->channels * audioDesc->bitsPerSample / 8);
				}
			}
		}
	}
	return r;
}

STNix_audioDesc nixBufferAudioDesc(STNix_Engine* engAbs, const NixUI16 buffIndex){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	STNix_audioDesc r;
	r.channels = 0;
	r.bitsPerSample = 0;
	r.samplerate = 0;
	r.samplerate = 0;
	NIX_ASSERT(buffIndex!=0)
	if(buffIndex!=0){
		NIX_ASSERT(buffIndex < eng->buffersArrUse)
		if(buffIndex < eng->buffersArrUse){
			NIX_ASSERT(eng->buffersArr[buffIndex].regInUse)
			if(eng->buffersArr[buffIndex].regInUse){
				r = eng->buffersArr[buffIndex].bufferDesc.audioDesc;
			}
		}
	}
	return r;
}

#ifdef NIX_OPENSL
void __nixCaptureBufferFilledCallback(SLAndroidSimpleBufferQueueItf bq, void* param){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)param;
	NIX_ASSERT(eng->buffersCaptureArrSize!=0)
	if(eng->buffersCaptureArrSize!=0){
		NixUI16 iBuffer = eng->buffersCaptureArrFirst + eng->buffersCaptureArrFilledCount;
		if(iBuffer >= eng->buffersCaptureArrSize) iBuffer -= eng->buffersCaptureArrSize;
		NIX_ASSERT(iBuffer < eng->buffersCaptureArrSize)
		NIX_ASSERT(eng->buffersCaptureArr[iBuffer].state==ENNixBufferState_AttachedForCapture)
		NIX_ASSERT(eng->buffersCaptureArrFirst < eng->buffersCaptureArrSize)
		NIX_ASSERT(eng->buffersCaptureArrFilledCount < eng->buffersCaptureArrSize)
		eng->buffersCaptureArr[iBuffer].state = ENNixBufferState_LoadedWithCapture;
		eng->buffersCaptureArrFilledCount++;
	}
	//PRINTF_INFO("__nixCaptureBufferFilledCallback (%d buffers filled of %d).\n", eng->buffersCaptureArrFilledCount, eng->buffersCaptureArrSize);
}
#endif

#ifdef NIX_OPENSL
void __nixCaptureOpenSLConsumeFilledBuffers(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng	= (STNix_EngineObjetcs*)engAbs->o;
	NixUI16 qBuffers			= eng->buffersCaptureArrFilledCount;
	NixUI16 i, iBuffer			= eng->buffersCaptureArrFirst;
	//
	NIX_ASSERT((eng->buffersCaptureCallback!=NULL))
	NIX_ASSERT(qBuffers <= eng->buffersCaptureArrFilledCount)
	NIX_ASSERT(eng->buffersCaptureArrSize!=0)
	NIX_ASSERT(eng->buffersCaptureCallback!=NULL)
	//
	for(i=0; i<qBuffers; i++){
		STNix_bufferDesc* buff = &eng->buffersCaptureArr[iBuffer];
		//Notify callback
		NIX_ASSERT(buff->state == ENNixBufferState_LoadedWithCapture)
		(*eng->buffersCaptureCallback)(engAbs, eng->buffersCaptureCallbackUserData, buff->audioDesc, buff->dataPointer, buff->dataBytesCount, eng->captureSamplesPerBuffer);
		buff->state = ENNixBufferState_Free;
		iBuffer++; if(iBuffer >= eng->buffersCaptureArrSize) iBuffer -= eng->buffersCaptureArrSize;
		//Re-queue buffer
		NIX_ASSERT(buff->state == ENNixBufferState_Free)
		if(buff->state == ENNixBufferState_Free  && eng->captureInProgess){
			if((*eng->slRecBufferQueue)->Enqueue(eng->slRecBufferQueue, buff->dataPointer, buff->dataBytesCount)!=SL_RESULT_SUCCESS){
				PRINTF_ERROR("Could'nt re-queue capture buffer\n");
				NIX_ASSERT(NIX_FALSE)
			} else {
				buff->state	= ENNixBufferState_AttachedForCapture;
			}
		}
	}
	eng->buffersCaptureArrFirst		= iBuffer;
	eng->buffersCaptureArrFilledCount	-= qBuffers;
}
#endif

NixBOOL nixCaptureInit(STNix_Engine* engAbs, const STNix_audioDesc* audioDesc, const NixUI16 buffersCount, const NixUI16 samplesPerBuffer, PTRNIX_CaptureBufferFilledCallback bufferCaptureCallback, void* bufferCaptureCallbackUserData){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	PRINTF_INFO("CaptureInit with channels(%d) bitsPerSample(%d) samplerate(%d) blockAlign(%d).\n", audioDesc->channels, audioDesc->bitsPerSample, audioDesc->samplerate, audioDesc->blockAlign);
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL==NULL && buffersCount!=0 && samplesPerBuffer!=0){
		const ALenum dataFormat = NIX_OPENAL_AUDIO_FORMAT(audioDesc->channels, audioDesc->bitsPerSample);
		if(dataFormat!=0 && audioDesc->samplesFormat==ENNix_sampleFormat_int){
			eng->captureMainBufferBytesCount	= (audioDesc->bitsPerSample / 8) * audioDesc->channels * samplesPerBuffer * 2;
			eng->deviceCaptureAL		= alcCaptureOpenDevice(NULL/*nomDispositivo*/, audioDesc->samplerate, dataFormat, eng->captureMainBufferBytesCount);
			VERIFICA_ERROR_AL("alcCaptureOpenDevice")
			if(eng->deviceCaptureAL==NULL){
				PRINTF_ERROR("alcCaptureOpenDevice failed\n");
				eng->captureMainBufferBytesCount	= 0;
			} else {
				NixUI16 i; NixUI32 bytesPerBuffer;
				eng->captureFormat				= *audioDesc;
				eng->captureInProgess			= NIX_FALSE;
				eng->captureSamplesPerBuffer	= samplesPerBuffer;
				//Free last capture buffers
				if(eng->buffersCaptureArr!=NULL){
					NixUI16 i; const NixUI16 useCount = eng->buffersCaptureArrSize;
					for(i=0; i<useCount; i++){ NIX_FREE(eng->buffersCaptureArr[i].dataPointer); }
					NIX_FREE(eng->buffersCaptureArr);
					eng->buffersCaptureArr = NULL;
					eng->buffersCaptureArrSize = 0;
				}
				//Create capture buffers
				bytesPerBuffer					= (audioDesc->bitsPerSample / 8) * audioDesc->channels * samplesPerBuffer;
				eng->buffersCaptureArrFirst	= 0;
				eng->buffersCaptureArrFilledCount = 0;
				eng->buffersCaptureArrSize		= buffersCount;
				NIX_MALLOC(eng->buffersCaptureArr, STNix_bufferDesc, sizeof(STNix_bufferDesc) * buffersCount, "buffersCaptureArr")
				eng->buffersCaptureCallback	= bufferCaptureCallback;
				eng->buffersCaptureCallbackUserData = bufferCaptureCallbackUserData;
				for(i=0; i<buffersCount; i++){
					STNix_bufferDesc* buff = &eng->buffersCaptureArr[i];
					buff->dataBytesCount			= bytesPerBuffer;
					NIX_MALLOC(buff->dataPointer, NixUI8, sizeof(NixUI8) * bytesPerBuffer, "buffCap.dataPointer")
					buff->state		= ENNixBufferState_Free;
					buff->audioDesc	= *audioDesc;
				}
				return NIX_TRUE;
			}
		}
	}
	#elif defined(NIX_OPENSL)
	if(eng->slRecObject==NULL && buffersCount!=0 && samplesPerBuffer!=0){
		NixUI32 slFormatBitPerSample	= NIX_OPENSL_AUDIO_SAMPLE_FORMAT(audioDesc->bitsPerSample);
		NixUI32 slFormatSampleRate		= audioDesc->samplerate * 1000;
		if(slFormatBitPerSample!=0 && slFormatSampleRate!=0 && audioDesc->samplesFormat==ENNix_sampleFormat_int){
			//Configure audio source
			SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
			SLDataSource audioSrc		= {&loc_dev, NULL};
			//Configure audio sink
			int speakers				= (audioDesc->channels==1 ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT));
			SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, buffersCount};
			SLDataFormat_PCM format_pcm	= {SL_DATAFORMAT_PCM, audioDesc->channels, slFormatSampleRate, slFormatBitPerSample, slFormatBitPerSample, speakers, SL_BYTEORDER_LITTLEENDIAN};
			SLDataSink audioSnk			= {&loc_bq, &format_pcm};
			//Create audio recorder
			//(requires the RECORD_AUDIO permission at AndroidManifest.xml)
			const SLInterfaceID id[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
			const SLboolean req[] = {SL_BOOLEAN_TRUE};
			SLObjectItf slRecObject;
			SLRecordItf slRecRecord;
			SLAndroidSimpleBufferQueueItf slRecBufferQueue;
			if((*eng->slEngine)->CreateAudioRecorder(eng->slEngine, &slRecObject, &audioSrc, &audioSnk, 1, id, req)!=SL_RESULT_SUCCESS){
				PRINTF_ERROR("ERROR al slEngine->CreateAudioRecorder().\n");
			} else {
				if((*slRecObject)->Realize(slRecObject, SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS){
					PRINTF_ERROR("ERROR al slRecObject->Realize().\n");
				} else {
					if((*slRecObject)->GetInterface(slRecObject, SL_IID_RECORD, &slRecRecord)!=SL_RESULT_SUCCESS){
						PRINTF_ERROR("ERROR al slRecObject->GetInterface(SL_IID_RECORD).\n");
					} else {
						if((*slRecObject)->GetInterface(slRecObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &slRecBufferQueue)!=SL_RESULT_SUCCESS){
							PRINTF_ERROR("ERROR al slRecObject->GetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE).\n");
						} else {
							if((*slRecBufferQueue)->RegisterCallback(slRecBufferQueue, __nixCaptureBufferFilledCallback, eng)!=SL_RESULT_SUCCESS){
								PRINTF_ERROR("ERROR al slRecBufferQueue->RegisterCallback().\n");
							} else {
								//Release existents capture-buffers
								if(eng->buffersCaptureArr!=NULL){
									NixUI16 i; const NixUI16 useCount = eng->buffersCaptureArrSize;
									for(i=0; i<useCount; i++){ NIX_FREE(eng->buffersCaptureArr[i].dataPointer); }
									NIX_FREE(eng->buffersCaptureArr);
									eng->buffersCaptureArr = NULL;
									eng->buffersCaptureArrSize = 0;
								}
								//Create capture buffers
								NixUI16 i; NixUI32 bytesPerBuffer = (audioDesc->bitsPerSample / 8) * audioDesc->channels * samplesPerBuffer;
								eng->buffersCaptureArrFirst	= 0;
								eng->buffersCaptureArrFilledCount = 0;
								eng->buffersCaptureArrSize		= buffersCount;
								NIX_MALLOC(eng->buffersCaptureArr, STNix_bufferDesc, sizeof(STNix_bufferDesc) * buffersCount, "buffersCaptureArr")
								eng->buffersCaptureCallback	= bufferCaptureCallback;
								eng->buffersCaptureCallbackUserData = bufferCaptureCallbackUserData;
								for(i=0; i<buffersCount; i++){
									STNix_bufferDesc* buff		= &eng->buffersCaptureArr[i];
									buff->dataBytesCount			= bytesPerBuffer;
									NIX_MALLOC(buff->dataPointer, NixUI8, sizeof(NixUI8) * bytesPerBuffer, "buffCap.dataPointer")
									buff->state		= ENNixBufferState_Free;
									buff->audioDesc			= *audioDesc;
									//
									if((*slRecBufferQueue)->Enqueue(slRecBufferQueue, buff->dataPointer, buff->dataBytesCount)!=SL_RESULT_SUCCESS){
										PRINTF_ERROR("Could'nt queue capture buffer #%d of %d\n", i+1, buffersCount);
										NIX_FREE(buff->dataPointer);
										eng->buffersCaptureArrSize = i;
										break;
									} else {
										buff->state	= ENNixBufferState_AttachedForCapture;
									}
								}
								//
								eng->captureFormat				= *audioDesc;
								eng->captureInProgess			= NIX_FALSE;
								eng->captureSamplesPerBuffer	= samplesPerBuffer;
								//
								eng->slRecObject				= slRecObject;
								eng->slRecRecord				= slRecRecord;
								eng->slRecBufferQueue			= slRecBufferQueue;
								return NIX_TRUE;
							}
						}
					}
				}
				(*slRecObject)->Destroy(slRecObject);
			}
		}
	}
	#endif
	return NIX_FALSE;
}

void nixCaptureFinalize(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	//Destroy capture device
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL){
		alcCaptureStop(eng->deviceCaptureAL); VERIFICA_ERROR_AL("alcCaptureStop");
		if(alcCaptureCloseDevice(eng->deviceCaptureAL)==AL_FALSE){
			PRINTF_ERROR("alcCaptureCloseDevice failed\n");
		}
		eng->captureMainBufferBytesCount = 0;
		eng->deviceCaptureAL = NULL;
	}
	#elif defined(NIX_OPENSL)
	if(eng->slRecObject != NULL) {
		(*eng->slRecBufferQueue)->Clear(eng->slRecBufferQueue);
		(*eng->slRecObject)->Destroy(eng->slRecObject);
		eng->slRecObject		= NULL;
		eng->slRecRecord		= NULL;
		eng->slRecBufferQueue	= NULL;
	}
	#endif
	//Destroy capture buffers
	if(eng->buffersCaptureArr!=NULL){
		NixUI16 i; const NixUI16 useCount = eng->buffersCaptureArrSize;
		for(i=0; i<useCount; i++){
			NIX_FREE(eng->buffersCaptureArr[i].dataPointer);
		}
		NIX_FREE(eng->buffersCaptureArr);
		eng->buffersCaptureArr				= NULL;
		eng->buffersCaptureArrSize			= 0;
		eng->buffersCaptureArrFilledCount	= 0;
	}
}

NixBOOL nixCaptureIsOnProgress(STNix_Engine* engAbs){
	return ((STNix_EngineObjetcs*)engAbs->o)->captureInProgess;
}

NixBOOL nixCaptureStart(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL /*&& !eng->captureInProgess*/){
		ALenum error;
		alcCaptureStart(eng->deviceCaptureAL);
		error = alGetError();
		if(error!=AL_NO_ERROR){
			PRINTF_ERROR("Could not start audio capture error#(%d)\n", (NixSI32)error);
		} else {
			eng->captureInProgess = NIX_TRUE;
			return NIX_TRUE;
		}
	}
	#elif defined(NIX_OPENSL)
	if(eng->slRecObject != NULL /*&& !eng->captureInProgess*/) {
		if((*eng->slRecRecord)->SetRecordState(eng->slRecRecord, SL_RECORDSTATE_RECORDING)!=SL_RESULT_SUCCESS){
			PRINTF_ERROR("Could not slRecObject->SetRecordState(SL_RECORDSTATE_RECORDING)\n");
		} else {
			eng->captureInProgess = NIX_TRUE;
			return NIX_TRUE;
		}
	}
	#endif
	return NIX_FALSE;
}

void nixCaptureStop(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL){
		alcCaptureStop(eng->deviceCaptureAL);
		eng->captureInProgess = NIX_FALSE;
	}
	#elif defined(NIX_OPENSL)
	if(eng->slRecObject != NULL /*&& !eng->captureInProgess*/) {
		(*eng->slRecRecord)->SetRecordState(eng->slRecRecord, SL_RECORDSTATE_STOPPED);
		eng->captureInProgess = NIX_FALSE;
	}
	#endif
}

NixUI32 nixCaptureFilledBuffersCount(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL){
		__nixCaptureOpenALMoveSamplesToBuffers(engAbs);
		return eng->buffersCaptureArrFilledCount;
	}
	#endif
	return 0;
}

NixUI32 nixCaptureFilledBuffersSamples(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL){
		__nixCaptureOpenALMoveSamplesToBuffers(engAbs);
		return (eng->buffersCaptureArrFilledCount * eng->captureSamplesPerBuffer);
	}
	#endif
	return 0;
}

float nixCaptureFilledBuffersSeconds(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	#ifdef NIX_OPENAL
	if(eng->deviceCaptureAL!=NULL && eng->captureFormat.samplerate!=0){
		return (float)nixCaptureFilledBuffersSamples(engAbs) / (float)eng->captureFormat.samplerate;
	}
	#endif
	return 0.0f;
}

void nixCaptureFilledBuffersRelease(STNix_Engine* engAbs, NixUI32 quantBuffersToRelease){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	if(quantBuffersToRelease > eng->buffersCaptureArrFilledCount) quantBuffersToRelease = eng->buffersCaptureArrFilledCount;
	eng->buffersCaptureArrFirst += quantBuffersToRelease;
	if(eng->buffersCaptureArrFirst >= eng->buffersCaptureArrSize) eng->buffersCaptureArrFirst -= eng->buffersCaptureArrSize;
}

#ifdef NIX_OPENAL
void __nixCaptureOpenALMoveSamplesToBuffers(STNix_Engine* engAbs){
	STNix_EngineObjetcs* eng = (STNix_EngineObjetcs*)engAbs->o;
	NIX_ASSERT(eng->deviceCaptureAL!=NULL) //This is a private method and must be called when "deviceCaptureAL" is not null
	if(eng->buffersCaptureArrFilledCount != eng->buffersCaptureArrSize){
		ALCint samplesCaptured	= 0; const ALCint samplesPerBuffer = eng->captureSamplesPerBuffer;
		alcGetIntegerv(eng->deviceCaptureAL, ALC_CAPTURE_SAMPLES, 1, &samplesCaptured); VERIFICA_ERROR_AL("alcGetIntegerv(ALC_CAPTURE_SAMPLES)")
		while(samplesCaptured>=samplesPerBuffer && eng->buffersCaptureArrFilledCount!=eng->buffersCaptureArrSize){
			STNix_bufferDesc* buffToFill;
			NixUI16 buffIndex = eng->buffersCaptureArrFirst + eng->buffersCaptureArrFilledCount; if(buffIndex>=eng->buffersCaptureArrSize) buffIndex -= eng->buffersCaptureArrSize;
			buffToFill = &eng->buffersCaptureArr[buffIndex];
			NIX_ASSERT(buffIndex < eng->buffersCaptureArrSize) //Just checking, this should be always be true
			NIX_ASSERT(buffToFill->state == ENNixBufferState_Free) //Just checking, this should be always be true
			alcCaptureSamples(eng->deviceCaptureAL, buffToFill->dataPointer, samplesPerBuffer);
			buffToFill->state = ENNixBufferState_LoadedWithCapture;
			eng->buffersCaptureArrFilledCount++;
			samplesCaptured -= samplesPerBuffer;
			//BufferFilledCallback
			if(eng->buffersCaptureCallback!=NULL){
				NIX_ASSERT(buffIndex==eng->buffersCaptureArrFirst)
				(*eng->buffersCaptureCallback)(engAbs, eng->buffersCaptureCallbackUserData, buffToFill->audioDesc, buffToFill->dataPointer, buffToFill->dataBytesCount, samplesPerBuffer);
				buffToFill->state = ENNixBufferState_Free;
				eng->buffersCaptureArrFirst++; if(eng->buffersCaptureArrFirst >= eng->buffersCaptureArrSize) eng->buffersCaptureArrFirst -= eng->buffersCaptureArrSize;
				eng->buffersCaptureArrFilledCount--;
			}
		}
	}
}
#endif



