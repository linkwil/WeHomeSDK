package com.linkwil.util;

import android.util.SparseArray;

import java.util.concurrent.ArrayBlockingQueue;

/**
 * Created by xiao on 2018/1/5.
 */

abstract public class ECAudioFrameQueue {

    private ArrayBlockingQueue<SparseArray<Object>> mQueue;

    public ECAudioFrameQueue(int capacity) {
        mQueue = new ArrayBlockingQueue<>(capacity);
    }

    protected abstract boolean onTake(byte[] data, int dataLen, int payloadType, long timestamp, int playbackSession);

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
        int playbackSession = (int) vFrame.get(4);
        return onTake(data, dataLen, payloadType, timestamp, playbackSession);
    }

    public boolean add(byte[] data, int dataLen, int payloadType, long timestamp, int playbackSession) {
        SparseArray<Object> vFrame = new SparseArray<>();
        byte []audioData = new byte[dataLen];
        System.arraycopy(data, 0, audioData, 0, dataLen);
        vFrame.put(0, audioData);
        vFrame.put(1, dataLen);
        vFrame.put(2, payloadType);
        vFrame.put(3, timestamp);
        vFrame.put(4, playbackSession);
        return mQueue.add(vFrame);
    }
}
