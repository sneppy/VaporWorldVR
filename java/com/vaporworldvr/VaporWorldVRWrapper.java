package com.vaporworldvr;

import android.app.Activity;
import android.view.Surface;


class VaporWorldVRWrapper {
	// ==================
	// Activity lifecycle
	// ==================

	public static native long onCreate(Activity activity);
	public static native void onStart(long handle);
	public static native void onResume(long handle);
	public static native void onPause(long handle);
	public static native void onStop(long handle);
	public static native void onDestroy(long handle);

	// =================
	// Surface lifecycle
	// =================

	public static native void onSurfaceCreated(long handle, Surface s);
	public static native void onSurfaceChanged(long handle, Surface s);
	public static native void onSurfaceDestroyed(long handle);
}
