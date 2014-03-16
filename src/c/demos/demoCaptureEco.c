//
//  main.c
//  NixtlaDemo
//
//  Created by Marcos Ortega on 11/02/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#include <stdio.h>	//printf

#if defined(_WIN32) || defined(WIN32)
	#include <windows.h> //Sleep
	#pragma comment(lib, "OpenAL32.lib") //link to openAL's lib
	#define DEMO_SLEEP_MILLISEC(MS) Sleep(MS)
#else
	#include <unistd.h> //sleep, usleep
	#define DEMO_SLEEP_MILLISEC(MS) usleep((MS) * 1000);
#endif

#include "nixtla-audio.h"

void bufferCapturedCallback(STNix_Engine* eng, void* userdata, const STNix_audioDesc audioDesc, const NixUI8* audioData, const NixUI32 audioDataBytes, const NixUI32 audioDataSamples);

int main(int argc, const char * argv[]){
	//
	STNix_Engine nix;
	if(nixInit(&nix, 8)){
		STNix_audioDesc audioDesc; NixUI16 iSourceStrm;
		nixPrintCaps(&nix);
		//Source for stream eco (play the captured audio)
		iSourceStrm = nixSourceAssignStream(&nix, 1, 0, NULL, NULL, 4, NULL, NULL);
		if(iSourceStrm!=0){
			nixSourcePlay(&nix, iSourceStrm);
			//Init the capture
			audioDesc.samplesFormat		= ENNix_sampleFormat_int;
			audioDesc.channels			= 1;
			audioDesc.bitsPerSample		= 16;
			audioDesc.samplerate		= 22050;
			audioDesc.blockAlign		= (audioDesc.bitsPerSample/8) * audioDesc.channels;
			if(nixCaptureInit(&nix, &audioDesc, 15, audioDesc.samplerate/10, &bufferCapturedCallback, &iSourceStrm)){
				if(nixCaptureStart(&nix)){
					printf("Capturing and playing audio...\n");
					while(1){ //Infinite loop, usually sync with your program main loop, or in a independent thread
						nixTick(&nix);
						DEMO_SLEEP_MILLISEC(1000 / 30); //30 ticks per second for this demo
					}
					nixCaptureStop(&nix);
				}
				nixCaptureFinalize(&nix);
			}
			nixSourceRelease(&nix, iSourceStrm);
		}
		nixFinalize(&nix);
	}
    return 0;
}

void bufferCapturedCallback(STNix_Engine* eng, void* userdata, const STNix_audioDesc audioDesc, const NixUI8* audioData, const NixUI32 audioDataBytes, const NixUI32 audioDataSamples){
	const NixUI16 iSource = *((NixUI16*)userdata);
	const NixUI16 iBuffer = nixBufferWithData(eng, &audioDesc, audioData, audioDataBytes);
	if(iBuffer==0){
		printf("bufferCapturedCallback, nixBufferWithData failed for iSource(%d)\n", iSource);
	} else {
		if(nixSourceStreamAppendBuffer(eng, iSource, iBuffer)){
			//PRINTF_INFO("bufferCapturedCallback, source(%d) new buffer(%d) attached\n", iSource, iBuffer);
		} else {
			printf("bufferCapturedCallback, ERROR attaching new buffer buffer(%d) to source(%d)\n", iBuffer, iSource);
		}
		nixBufferRelease(eng, iBuffer);
	}
}