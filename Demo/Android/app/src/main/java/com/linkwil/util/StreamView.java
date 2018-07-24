package com.linkwil.util;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Process;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.nio.ByteBuffer;

public class StreamView extends SurfaceView implements SurfaceHolder.Callback2 {

	private static final String TAG = StreamView.class.getSimpleName();
		
	public static final int FLIP_NONE		= 0;
	public static final int FLIP_HORIZONTAL	= 1;
	public static final int FLIP_VERTICAL	= 2;
	public static final int FLIP_REVERSE	= 3;
	
	public static final int AR_AUTO	= 0;
	public static final int AR_FULL	= 1;
	public static final int AR_R133 = 2;
	public static final int AR_R177	= 3;

	private final int BACK_GROUND_COLOR = Color.rgb(0x21, 0x21, 0x21);

	private boolean mIsHWDecoderSupported = false;
	private boolean mIsForceSoftDecoder = false;
	private boolean mSurfaceReady;
	private SurfaceHolder mSurfaceHolder;
	private int mBitmapWidth, mBitmapHeight;
	
	private Bitmap mBitmap;
	private Bitmap.Config mBitmapConfig;
	private Rect mScreenRect;
	private int mFlip, mAspectRatio;
	private boolean mPause, mSetThreadPriority;

	private Matrix mScaleMatrix = new Matrix();
	private final float[] mMatrixValues = new float[9];

	private MediaCodec mCodec;
	private boolean mHasMediaCodecStarted = false;
	private ByteBuffer[] inputBuffers;

	public StreamView(Context context) {
		this(context, Bitmap.Config.ARGB_8888, AR_AUTO);
	}
	
	public StreamView(Context context, Bitmap.Config colorMode) {
		this(context, colorMode, AR_AUTO);
	}

	public StreamView(Context context, Bitmap.Config config, int aspectRatio) {
		super(context);
		mAspectRatio = aspectRatio;
		mSurfaceHolder = getHolder();
		mSurfaceHolder.addCallback(this);
		mBitmapConfig = config;
	}

	public boolean isHWDecoderSupported(){
		return ((!mIsForceSoftDecoder)&&mIsHWDecoderSupported);
	}

	public void setForceSoftwareDecode(boolean force){
		mIsForceSoftDecoder = force;
		if( mCodec != null ){
			if( mHasMediaCodecStarted ) {
				mCodec.stop();
                mCodec.release();
				mHasMediaCodecStarted = false;
			}
			mCodec = null;
		}
	}

	public boolean isSurfaceReady(){
		return mSurfaceReady;
	}
	
	public void setPause(boolean pause) {
		mPause = pause;
	}

	public synchronized void onOrientationChanged(){
		float scale = 1.0f;
		// 如果图片的宽或者高大于屏幕，则缩放至屏幕的宽或者高
		if( mBitmap != null ) {
			int bmpWidth = mBitmap.getWidth();
			int bmpHeight = mBitmap.getHeight();
			int viewWidth = getWidth();
			int viewHeight = getHeight();

			Configuration config = getResources().getConfiguration();
			if( config.orientation==Configuration.ORIENTATION_LANDSCAPE ) {
				if (viewHeight > bmpHeight * viewWidth * 1.0f / bmpWidth) {
					scale = viewWidth * 1.0f / bmpWidth;
				} else {
					scale = viewHeight * 1.0f / bmpHeight;
				}
			} else {
				scale = viewWidth * 1.0f / bmpWidth;
			}

			mScaleMatrix.reset();
			mScaleMatrix.postScale(scale, scale, 0, 0);
			mScaleMatrix.postTranslate((viewWidth - bmpWidth * scale) / 2, (viewHeight - bmpHeight * scale) / 2);

			onDrawImage(mSurfaceHolder);
		}
	}
	
	protected void onDrawImage(SurfaceHolder holder) {
		if (!mSetThreadPriority) {
			mSetThreadPriority = true;
			Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY);
		}
		Canvas canvas = holder.lockCanvas();
		if (canvas != null) {
			if (mFlip != FLIP_NONE) {
				if (FLIP_REVERSE == mFlip) {
					canvas.rotate(180.0F);
					canvas.translate(-mScreenRect.width(), -mScreenRect.height());
				} else if (FLIP_HORIZONTAL == mFlip) {
					canvas.scale(-1.0F, 1.0F);
					canvas.translate(mScreenRect.width(), 0);
				} else if (FLIP_VERTICAL == mFlip) {
					canvas.rotate(180.0F);
					canvas.scale(-1.0F, 1.0F);
					canvas.translate(0, -mScreenRect.height());
				}
			}
			//canvas.drawBitmap(mBitmap, null, mScreenRect, null);
			canvas.drawColor(BACK_GROUND_COLOR);
			if( mBitmap != null ) {
				canvas.drawBitmap(mBitmap, mScaleMatrix, null);
			}
			holder.unlockCanvasAndPost(canvas);
		} else {
			Log.w(TAG, "lock canvas fail");
		}
	}

	public synchronized boolean hardDecodeImage(byte[] data, int dataLen) {

		if( mCodec == null ) {
			Log.d("LinkBell", "Init hw decoder");
			try {
				mCodec = MediaCodec.createDecoderByType("video/avc");
				MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", 1280, 720);
//                mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,
//                        MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
				mCodec.configure(mediaFormat, mSurfaceHolder.getSurface(), null, 0);
				mCodec.start();
				mHasMediaCodecStarted = true;
				mIsHWDecoderSupported = true;
			} catch (Exception e) {
				Log.e("LinkBell", "Create H264 hardware decoder fail:" + e.getMessage());
				mIsHWDecoderSupported = false;
			}
		}

		if( mCodec != null ){
			try {
				inputBuffers = mCodec.getInputBuffers();

				int inputBufferIndex = mCodec.dequeueInputBuffer(-1);
				if (inputBufferIndex >= 0) {
					ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
					inputBuffer.clear();
					inputBuffer.put(data, 0, dataLen);
					mCodec.queueInputBuffer(inputBufferIndex, 0, dataLen, 0, 0);
				}

				MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
				int outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 0);
				if (outputBufferIndex >= 0) {
					while (outputBufferIndex >= 0) {
						mCodec.releaseOutputBuffer(outputBufferIndex, true);
						outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 0);
					}

					return true;
				} else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
					// Ingore it
				} else if( outputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER ){
					// Ignore
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
				} else {
					Log.e("LinkBell", "HW decode H264 fail, now use software decoder");
					return false;
				}
			}catch (Exception e){
				Log.e("LinkBell", "HW decode H264 excpetion, now use software decoder");
				return false;
			}

			return true;
		}else{
			return false;
		}
	}
	
	/**
	 * direct draw image, the data must be 32-bit (ARGB8888) format.
	 * @param data
	 * @param dataLen
	 */
	public synchronized void drawImage(byte[] data, int dataLen, int width, int height) {
		
		// check parameters
		if (data == null) {
			Log.e(TAG, "data is null"); return;}
		if (dataLen < 0) {
			Log.e(TAG, "data length is negative (" + dataLen + ")"); return;}
		if (width < 1) {
			Log.e(TAG, "width is zero or negative (" + width + ")"); return;}
		if (height < 1) {
			Log.e(TAG, "height is zero or negative (" + height + ")"); return;};

		// check state
		if (!mSurfaceReady) {
			Log.w(TAG, "surface is not created"); return;}
		if (mPause) {
			Log.w(TAG, "surface is paused"); return;}
		
		// check screen rectangle
		if (mScreenRect == null) {
			Log.e(TAG, "screen rect is null, could not draw screen");
			return;
		}
		
		// create bitmap
		if (mBitmapWidth != width || mBitmapHeight != height) {
			if (mBitmap != null)
				mBitmap.recycle();
			mBitmapWidth = width;
			mBitmapHeight = height;
			mBitmap = Bitmap.createBitmap(mBitmapWidth, mBitmapHeight, mBitmapConfig);
			Log.d(TAG, "create new bitmap, width = " + mBitmapWidth + ", height = " + mBitmapHeight + ", config = " + mBitmapConfig);
		}
		
		// raw buffer copy to bitmap object
		mBitmap.copyPixelsFromBuffer(ByteBuffer.wrap(data, 0, dataLen));
		
		// draw bitmap
		onDrawImage(mSurfaceHolder);
	}


	public synchronized void drawImage(ByteBuffer data, int width, int height) {

		// check parameters
		if (data == null) {
			Log.e(TAG, "data is null"); return;}
		if (width < 1) {
			Log.e(TAG, "width is zero or negative (" + width + ")"); return;}
		if (height < 1) {
			Log.e(TAG, "height is zero or negative (" + height + ")"); return;};

		// check state
		if (!mSurfaceReady) {
			Log.w(TAG, "surface is not created"); return;}
		if (mPause) {
			Log.w(TAG, "surface is paused"); return;}

		// check screen rectangle
		if (mScreenRect == null) {
			Log.e(TAG, "screen rect is null, could not draw screen");
			return;
		}

		// create bitmap
		if (mBitmapWidth != width || mBitmapHeight != height) {
			if (mBitmap != null)
				mBitmap.recycle();
			mBitmapWidth = width;
			mBitmapHeight = height;
			mBitmap = Bitmap.createBitmap(mBitmapWidth, mBitmapHeight, mBitmapConfig);
			Log.d(TAG, "create new bitmap, width = " + mBitmapWidth + ", height = " + mBitmapHeight + ", config = " + mBitmapConfig);

			float scale = 1.0f;
			// 如果图片的宽或者高大于屏幕，则缩放至屏幕的宽或者高
			int bmpWidth = mBitmap.getWidth();
			int bmpHeight = mBitmap.getHeight();
			int viewWidth = getWidth();
			int viewHeight = getHeight();

			Configuration config = getResources().getConfiguration();
			if( config.orientation==Configuration.ORIENTATION_LANDSCAPE )
			{
				if (viewHeight > bmpHeight * viewWidth * 1.0f / bmpWidth) {
					scale = viewWidth * 1.0f / bmpWidth;
				} else {
					scale = viewHeight * 1.0f / bmpHeight;
				}
			}
			else
			{
				scale = viewWidth * 1.0f / bmpWidth;
			}

			mScaleMatrix.postScale(scale, scale, 0, 0);
            mScaleMatrix.postTranslate((viewWidth-bmpWidth*scale)/2, (viewHeight-bmpHeight*scale)/2);
		}

		// raw buffer copy to bitmap object
		mBitmap.copyPixelsFromBuffer(data);

		// draw bitmap
		onDrawImage(mSurfaceHolder);
	}
	
	public synchronized Bitmap getImage(int width, int height) {
		if (mBitmap != null && width > 0 && height > 0 && (!mBitmap.isRecycled())) {
			return mBitmap;
			//return Bitmap.createScaledBitmap(mBitmap, width, height, false);
		}else {
			Log.w(TAG, "bitmap is null or width or height is zero or negative, width = " + width + ", height = " + height);
			return null;
		}
	}

	// XXX need to implement
	public int getAspectRatio() {
		return mAspectRatio;
	}
	
	// XXX need to implement
	public void setAspectRatio(int aspectRatio) {
		if (aspectRatio < 0 || aspectRatio > 3) {
			Log.e(TAG, "setting aspect ratio fail (" + aspectRatio + ")");
			return;
		}
		mAspectRatio = aspectRatio;
	}
	
	public int getFlip() {
		return mFlip;
	}
	
	public void setFlip(int flip) {
		if (flip < 0 || flip > 3) {
			Log.e(TAG, "setting flip fail (" + flip + ")");
			return;
		}
		mFlip = flip;
	}

	/**
	 * Create a new bitmap instance by inner bitmap.
	 * @return
	 */
	public Bitmap getBitmap() {
		if (mBitmap != null)
			return Bitmap.createBitmap(mBitmap);
		return null;
	}
	
	@Override
	public synchronized void surfaceCreated(SurfaceHolder holder) {
		mSurfaceReady = true;
		onDrawImage(mSurfaceHolder);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		mScreenRect = new Rect(0, 0, width, height);

		if( mBitmap != null ) {
			float scale;

			int bmpWidth = mBitmap.getWidth();
			int bmpHeight = mBitmap.getHeight();
			int viewWidth = getWidth();
			int viewHeight = getHeight();

			Configuration config = getResources().getConfiguration();
			if( config.orientation==Configuration.ORIENTATION_LANDSCAPE )
			{
				if (viewHeight > bmpHeight * viewWidth * 1.0f / bmpWidth) {
					scale = viewWidth * 1.0f / bmpWidth;
				} else {
					scale = viewHeight * 1.0f / bmpHeight;
				}
			}
			else
			{
				scale = viewWidth * 1.0f / bmpWidth;
			}

			mScaleMatrix.reset();
			mScaleMatrix.postScale(scale, scale, 0, 0);
			mScaleMatrix.postTranslate((viewWidth - bmpWidth * scale) / 2, (viewHeight - bmpHeight * scale) / 2);

			onDrawImage(mSurfaceHolder);
		}
	}

	@Override
	public synchronized void surfaceDestroyed(SurfaceHolder holder) {
		mSurfaceReady = false;
		if( mBitmap != null ){
			mBitmap.recycle();
			mBitmap = null;
			mBitmapWidth = 0;
			mBitmapHeight = 0;
		}
	}

	@Override
	public void surfaceRedrawNeeded(SurfaceHolder holder) {
		try {
			Canvas canvas = holder.lockCanvas();
			if (canvas != null) {
				// draw bitmap
				if (mBitmap != null && mScreenRect != null) {
					// 硬解模式不需要重新描绘
					if( !mIsHWDecoderSupported ) {
						//canvas.drawBitmap(mBitmap, null, mScreenRect, null);
						canvas.drawBitmap(mBitmap, mScaleMatrix, null);
					}
				} else {
					canvas.drawColor(BACK_GROUND_COLOR);
				}
				holder.unlockCanvasAndPost(canvas);
			}
		}catch (Exception e){
			Log.e("DEMO","Lock unlock canvas fail:"+e.getMessage());
		}

	}
}
