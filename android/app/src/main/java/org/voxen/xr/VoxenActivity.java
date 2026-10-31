package org.voxen.xr;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayDeque;
import java.util.UUID;

/** Android document picker bridge. Call openPanoramaPicker() on the UI thread. */
public final class VoxenActivity extends NativeActivity {
    private android.media.MediaPlayer nativeVideoPlayer;
    public synchronized boolean startNativeVideo(String path, android.view.Surface surface) {
        stopNativeVideo();
        try {
            nativeVideoPlayer = new android.media.MediaPlayer();
            nativeVideoPlayer.setDataSource(path);
            nativeVideoPlayer.setSurface(surface);
            nativeVideoPlayer.setLooping(true);
            nativeVideoPlayer.setOnPreparedListener(player -> {
                synchronized (VoxenActivity.this) {
                    if (player == nativeVideoPlayer) {
                        player.start();
                        Log.i("VoxenVideo", "OpenXR Surface playback started: " + path);
                    }
                }
            });
            nativeVideoPlayer.setOnErrorListener((player, what, extra) -> {
                Log.e("VoxenVideo", "MediaPlayer error what=" + what + " extra=" + extra);
                return true;
            });
            nativeVideoPlayer.prepareAsync();
            Log.i("VoxenVideo", "OpenXR Android Surface player preparing: " + path);
            return true;
        } catch (Exception ex) {
            Log.e("VoxenVideo", "OpenXR Surface playback failed", ex);
            stopNativeVideo();
            return false;
        }
    }
    public synchronized void stopNativeVideo() {
        if(nativeVideoPlayer != null) {
            try { nativeVideoPlayer.stop(); } catch (IllegalStateException ignored) {}
            nativeVideoPlayer.release();
            nativeVideoPlayer = null;
        }
    }

    private static final int PICK_PANORAMA = 4201;
    private final ArrayDeque<String> imported = new ArrayDeque<>();

    public void openPanoramaPicker() {
        runOnUiThread(() -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("image/*");
            intent.putExtra(Intent.EXTRA_MIME_TYPES,
                    new String[]{"image/jpeg", "image/png"});
            try {
                startActivityForResult(intent, PICK_PANORAMA);
            } catch (Exception e) {
                Log.e("VoxenPanorama", "Document picker unavailable", e);
            }
        });
    }

    /** Returns a private absolute path once, or null. Native side must validate and decode. */
    public synchronized String pollImportedPanorama() {
        return imported.pollFirst();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_PANORAMA || resultCode != RESULT_OK ||
                data == null || data.getData() == null) return;
        Uri uri = data.getData();
        String mime = getContentResolver().getType(uri);
        String extension;
        if ("image/jpeg".equals(mime)) extension = ".jpg";
        else if ("image/png".equals(mime)) extension = ".png";
        else {
            Log.w("VoxenPanorama", "Unsupported image MIME: " + mime);
            return;
        }
        File directory = new File(getFilesDir(), "voxen-gallery/panoramas");
        if (!directory.isDirectory() && !directory.mkdirs()) {
            Log.e("VoxenPanorama", "Cannot create panorama directory");
            return;
        }
        File target = new File(directory, UUID.randomUUID().toString() + extension);
        final long maxBytes = 64L * 1024L * 1024L;
        try (InputStream input = getContentResolver().openInputStream(uri);
             FileOutputStream output = new FileOutputStream(target)) {
            if (input == null) throw new java.io.IOException("No input stream");
            byte[] buffer = new byte[65536];
            long total = 0;
            int count;
            while ((count = input.read(buffer)) != -1) {
                total += count;
                if (total > maxBytes) throw new java.io.IOException("Image exceeds 64 MiB");
                output.write(buffer, 0, count);
            }
            output.getFD().sync();
            synchronized (this) { imported.addLast(target.getAbsolutePath()); }
            Log.i("VoxenPanorama", "Imported " + target.getName());
        } catch (Exception e) {
            target.delete();
            Log.e("VoxenPanorama", "Import failed", e);
        }
    }
}
