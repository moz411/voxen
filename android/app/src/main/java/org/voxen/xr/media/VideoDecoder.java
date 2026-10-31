package org.voxen.xr.media;

import android.graphics.ImageFormat;
import android.hardware.HardwareBuffer;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

import java.io.File;
import java.io.IOException;

/**
 * Hardware video decoder for local MP4/Android-supported containers.
 *
 * Video frames are produced as PRIVATE ImageReader images backed by
 * HardwareBuffer, intended for a future Vulkan external-memory importer.
 * The consumer MUST close every Image returned by acquireLatestFrame().
 * No CPU pixel conversion or readback is performed.
 *
 * Threading: control commands run on the codec HandlerThread. The render
 * thread may call acquireLatestFrame(); ImageReader is thread-safe.
 * This class does not render frames or decode the audio track.
 */
public final class VideoDecoder implements AutoCloseable {
    private static final String TAG = "VoxenVideo";
    private static final long TIMEOUT_US = 0;
    private final HandlerThread thread = new HandlerThread("VoxenVideoDecoder");
    private final Handler handler;
    private MediaExtractor extractor;
    private MediaCodec codec;
    private ImageReader imageReader;
    private boolean playing;
    private boolean looping;
    private boolean closed;
    private long timelineStartNs;
    private long firstPtsUs = -1;
    private boolean inputEos;
    private int width;
    private int height;

    public VideoDecoder() {
        thread.start();
        handler = new Handler(thread.getLooper());
    }

    /** Open a local file asynchronously. A new file replaces the previous one. */
    public void open(String absolutePath, boolean loop) {
        if (closed) throw new IllegalStateException("VideoDecoder closed");
        handler.post(() -> {
            releaseCodec();
            try {
                File file = new File(absolutePath);
                if (!file.isFile() || !file.canRead())
                    throw new IOException("Video file not readable: " + absolutePath);
                extractor = new MediaExtractor();
                extractor.setDataSource(absolutePath);
                int videoTrack = -1;
                MediaFormat format = null;
                for (int i = 0; i < extractor.getTrackCount(); i++) {
                    MediaFormat f = extractor.getTrackFormat(i);
                    String mime = f.getString(MediaFormat.KEY_MIME);
                    if (mime != null && mime.startsWith("video/")) {
                        videoTrack = i;
                        format = f;
                        break;
                    }
                }
                if (videoTrack < 0 || format == null)
                    throw new IOException("No video track");
                extractor.selectTrack(videoTrack);
                width = format.getInteger(MediaFormat.KEY_WIDTH);
                height = format.getInteger(MediaFormat.KEY_HEIGHT);
                // PRIVATE images are not CPU-readable; GPU sampling is the only
                // intended use. Keep the queue small to limit memory pressure.
                imageReader = ImageReader.newInstance(width, height,
                        ImageFormat.PRIVATE, 3, HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE);
                Surface surface = imageReader.getSurface();
                codec = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));
                codec.configure(format, surface, null, 0);
                codec.start();
                looping = loop;
                playing = true;
                inputEos = false;
                firstPtsUs = -1;
                timelineStartNs = 0;
                Log.i(TAG, "Decoder ready " + width + "x" + height);
                handler.post(pump);
            } catch (Exception error) {
                Log.e(TAG, "Unable to open video", error);
                releaseCodec();
            }
        });
    }

    public void pause() {
        if (!closed) handler.post(() -> playing = false);
    }

    public void play() {
        if (!closed) handler.post(() -> {
            if (codec == null || playing) return;
            playing = true;
            // Reanchor timestamps without carrying over a long pause.
            firstPtsUs = -1;
            handler.post(pump);
        });
    }

    public void seekToMs(long milliseconds) {
        if (!closed) handler.post(() -> {
            if (codec == null || extractor == null) return;
            extractor.seekTo(Math.max(0, milliseconds) * 1000,
                    MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
            codec.flush();
            inputEos = false;
            firstPtsUs = -1;
            if (playing) handler.post(pump);
        });
    }

    public void setLooping(boolean loop) {
        if (!closed) handler.post(() -> looping = loop);
    }

    /**
     * Called by the render side. Ownership transfers to caller; close the Image
     * AFTER Vulkan has finished using its HardwareBuffer (including GPU fences).
     * Return value is null if no new frame is ready.
     */
    public Image acquireLatestFrame() {
        ImageReader reader = imageReader;
        if (reader == null) return null;
        try {
            return reader.acquireLatestImage();
        } catch (IllegalStateException ignored) {
            return null; // e.g. consumer has reached maxImages
        }
    }

    public int getWidth() { return width; }
    public int getHeight() { return height; }

    private final Runnable pump = new Runnable() {
        @Override public void run() {
            if (!playing || codec == null || extractor == null || closed) return;
            try {
                if (!inputEos) {
                    int in = codec.dequeueInputBuffer(TIMEOUT_US);
                    if (in >= 0) {
                        java.nio.ByteBuffer buffer = codec.getInputBuffer(in);
                        if (buffer == null) throw new IllegalStateException("Missing input buffer");
                        int count = extractor.readSampleData(buffer, 0);
                        if (count < 0) {
                            codec.queueInputBuffer(in, 0, 0, 0,
                                    MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                            inputEos = true;
                        } else {
                            long pts = extractor.getSampleTime();
                            codec.queueInputBuffer(in, 0, count, pts, 0);
                            extractor.advance();
                        }
                    }
                }
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                int out = codec.dequeueOutputBuffer(info, TIMEOUT_US);
                if (out >= 0) {
                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        codec.releaseOutputBuffer(out, false);
                        if (looping) {
                            extractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                            codec.flush();
                            inputEos = false;
                            firstPtsUs = -1;
                        } else {
                            playing = false;
                        }
                    } else {
                        if (firstPtsUs < 0) {
                            firstPtsUs = info.presentationTimeUs;
                            timelineStartNs = SystemClock.elapsedRealtimeNanos();
                        }
                        long presentNs = timelineStartNs
                                + (info.presentationTimeUs - firstPtsUs) * 1000L;
                        codec.releaseOutputBuffer(out, Math.max(
                                SystemClock.elapsedRealtimeNanos(), presentNs));
                    }
                }
                if (playing) handler.postDelayed(this, out == MediaCodec.INFO_TRY_AGAIN_LATER ? 4 : 1);
            } catch (Exception error) {
                Log.e(TAG, "Decode failed", error);
                releaseCodec();
            }
        }
    };

    private void releaseCodec() {
        playing = false;
        if (codec != null) {
            try { codec.stop(); } catch (Exception ignored) {}
            codec.release();
            codec = null;
        }
        if (extractor != null) {
            extractor.release();
            extractor = null;
        }
        if (imageReader != null) {
            imageReader.close();
            imageReader = null;
        }
        inputEos = false;
        firstPtsUs = -1;
        width = height = 0;
    }

    @Override public void close() {
        if (closed) return;
        closed = true;
        handler.post(() -> {
            releaseCodec();
            thread.quitSafely();
        });
    }
}
