package com.nibsa.nixtla.android.demo.captureeco;

public class DemoNativeInterface {

	static {
		System.loadLibrary("com.nibsa.nixtla.android.demo.captureeco.native");
	}
	 
	private boolean sucessOnInit = false;
	
	public DemoNativeInterface(){
		sucessOnInit = nativeInit();
		if(!sucessOnInit){
			System.out.println("ERROR nativeInit returned FALSE.");
		} else {
			System.out.println("Capturing and playing audio...");
		} 
	}
	
	public void tick(){
		if(sucessOnInit) nativeTick();
	}
	
	public String getStatusLog(){
		return nativeGetStatusLog();
	}
	
	public boolean actionCapture(int channels, int sampleRate, int bitsPerSample){
		if(sucessOnInit) return nativeActionCapture(channels, sampleRate, bitsPerSample);
		return false;
	}
	
	public void finalizeDemo(){
		if(sucessOnInit) nativeFinalize();
		sucessOnInit = false;
	}
	
	private static native boolean nativeInit();
	private static native void nativeFinalize();
	private static native void nativeTick();
	private static native boolean nativeActionCapture(int channels, int sampleRate, int bitsPerSample);
	private static native String nativeGetStatusLog();
	
}
