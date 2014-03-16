package com.nibsa.nixtla.android.demo.playwav;

import android.content.res.AssetManager;

public class DemoNativeInterface {

	static {
		System.loadLibrary("com.nibsa.nixtla.android.demo.playwav.native");
	}
	
	private boolean _sucessOnInit = false;
	
	public DemoNativeInterface(){
		_sucessOnInit = nativeInit();
		if(!_sucessOnInit){
			System.out.println("nativeInit error...");
		} else {
			System.out.println("nativeInit success...");
		}
	}
	
	public boolean actionOnWav(AssetManager mgr, String wavPath, boolean repeat, float volume){
		if(_sucessOnInit){
			if(nativeActionOnWav(mgr, wavPath, repeat, volume)){
				System.out.println("Action for '"+wavPath+"' success.");
				return true;
			} else {
				System.out.println("Action for '" + wavPath + "' error.");
				return false;
			}
		}
		return false;
	}
	
	public void tick(){
		if(_sucessOnInit) nativeTick();
	}
	
	public String getStatusLog(){
		return nativeGetStatusLog();
	} 
	
	public void finalizeDemo(){
		if(_sucessOnInit) nativeFinalize(); 
		_sucessOnInit = false;
	}
	
	private static native boolean nativeInit();
	private static native boolean nativeActionOnWav(AssetManager mgr, String wavPath, boolean repeat, float volume);
	private static native void nativeFinalize();
	private static native void nativeTick();
	private static native String nativeGetStatusLog();
	
}
