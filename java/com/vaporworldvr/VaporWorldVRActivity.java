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

	/**
	 * The surface used to render the application.
	 */
	private SurfaceView surfaceView;

	/**
	 * The holder for the surface.
	 */
	private SurfaceHolder surfaceHolder;

	/**
	 * Pointer to the native application thread.
	 */
	private long nativeAppHandle;

	// =======================
	// Activity implementation
	// =======================

	/**
	 * Called when the activity is created.
	 */
	@Override
	protected void onCreate(Bundle bundle) {
		super.onCreate(bundle);

		// Create the surface and register callbacks
		surfaceView = new SurfaceView(this);
		surfaceView.getHolder().addCallback(this);
		setContentView(surfaceView);

		// Call native method
		nativeAppHandle = VaporWorldVRWrapper.onCreate(this);
	}

	/**
	 * Called when the activity is started.
	 */
	@Override
	protected void onStart() {
		super.onStart();
		VaporWorldVRWrapper.onStart(nativeAppHandle);
	}

	/**
	 * Called when the activity is resumed.
	 */
	@Override
	protected void onResume() {
		super.onResume();
		VaporWorldVRWrapper.onResume(nativeAppHandle);
	}

	/**
	 * Called when the activity is paused.
	 */
	@Override
	protected void onPause() {
		VaporWorldVRWrapper.onPause(nativeAppHandle);
		super.onPause();
	}

	/**
	 * Called when the activity is stopped.
	 */
	@Override
	protected void onStop() {
		VaporWorldVRWrapper.onStop(nativeAppHandle);
		super.onStop();
	}

	/**
	 * Called after the activity is destroyed.
	 */
	@Override
	protected void onDestroy() {
		if (surfaceHolder != null) {
			// It may happen that the activity is destroyed before the surface.
			// In that case we need to release the surface first
			VaporWorldVRWrapper.onSurfaceDestroyed(nativeAppHandle);
			surfaceHolder = null;
		}

		VaporWorldVRWrapper.onDestroy(nativeAppHandle);
		nativeAppHandle = 0;
		super.onDestroy();
	}

	// =====================================
	// SurfaceHolder.Callback implementation
	// =====================================

	/**
	 * Called when the surface is created.
	 */
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		if (nativeAppHandle != 0) {
			VaporWorldVRWrapper.onSurfaceCreated(nativeAppHandle, holder.getSurface());
			surfaceHolder = holder;
		}
	}

	/**
	 * Called when the surface changes.
	 */
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (nativeAppHandle != 0) {
			VaporWorldVRWrapper.onSurfaceChanged(nativeAppHandle, holder.getSurface());
			surfaceHolder = holder;
		}
	}

	/**
	 * Called when the surface is destroyed.
	 */
	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if (nativeAppHandle != 0) {
			VaporWorldVRWrapper.onSurfaceDestroyed(nativeAppHandle);
			surfaceHolder = null;
		}
	}
}
