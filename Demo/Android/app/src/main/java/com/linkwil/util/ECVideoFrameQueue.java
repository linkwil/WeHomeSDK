package com.linkwil.util;

import android.util.SparseArray;

import java.util.concurrent.ArrayBlockingQueue;

/**
 * Created by xiao on 2018/1/5.
 */

abstract public class ECVideoFrameQueue {

    private ArrayBlockingQueue<SparseArray<Object>> mQueue;

    public ECVideoFrameQueue(int capacity) {
        mQueue = new ArrayBlockingQueue<>(capacity);
    }

    protected abstract boolean onTake(byte[] data, int dataLen, int payloadType, long timestamp,
                                      int frameType, int videoWidth, int videoHeight, int wifiQuality, int playbackSession);

    public int size() {
        return mQueue.size();
    }

    public void clear() {
        mQueue.clear();
    }

    public boolean take() throws InterruptedException {
        SparseArray<Object> vFrame = mQueue.take();
        byte[] data = (byte[]) vFrame.get(0);
        int dataLen = (int) vFrame.get(1);
        int payloadType = (int) vFrame.get(2);
        long timestamp = (long) vFrame.get(3);
        int frameType = (int) vFrame.get(4);
        int videoWidth = (int) vFrame.get(5);
        int videoHeight = (int) vFrame.get(6);
        int wifiQuality = (int) vFrame.get(7);
        int playbackSession = (int) vFrame.get(8);
        return onTake(data, dataLen, payloadType, timestamp, frameType, videoWidth, videoHeight, wifiQuality, playbackSession);
    }

    public boolean add(byte[] data, int dataLen, int payloadType, long timestamp, int frameType,
                       int videoWidth, int videoHeight, int wifiQuality, int playbackSession) {
        SparseArray<Object> vFrame = new SparseArray<>();
        byte []videoData = new byte[dataLen];
        System.arraycopy(data, 0, videoData, 0, dataLen);
        vFrame.put(0, videoData);
        vFrame.put(1, dataLen);
        vFrame.put(2, payloadType);
        vFrame.put(3, timestamp);
        vFrame.put(4, frameType);
        vFrame.put(5, videoWidth);
        vFrame.put(6, videoHeight);
        vFrame.put(7, wifiQuality);
        vFrame.put(8, playbackSession);
        return mQueue.add(vFrame);
    }
}
