package com.vaporworldvr;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

class VaporWorldVRActivity extends Activity implements SurfaceHolder.Callback {
	static {
		// Load DLL library
		System.loadLibrary("vaporworld_vr");
	}

	// =======================
	// Activity implementation
	// =======================

	@Override
	protected void onCreate(Bundle bundle) {
		// TODO
	}

	@Override
	protected void onStart() {
		// TODO
	}

	@Override
	protected void onResume() {
		// TODO
	}

	@Override
	protected void onPause() {
		// TODO
	}

	@Override
	protected void onStop() {
		// TODO
	}

	@Override
	protected void onDestroy() {

	}

	// =====================================
	// SurfaceHolder.Callback implementation
	// =====================================

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		// TODO
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		// TODO
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO
	}
}
