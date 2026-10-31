package org.voxen.xr;

import android.app.NativeActivity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.media.MediaMetadataRetriever;
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
    // Temporary low-resolution visual path while zero-copy YCbCr sync is completed.
    // 512x256 RGBA is intentionally capped to avoid massive XR upload stalls.
    private MediaMetadataRetriever previewRetriever;
    private String previewPath;

    public synchronized byte[] extractVideoPreviewRgba(String path, long timeUs) {
        try {
            if (!path.equals(previewPath) || previewRetriever == null) {
                closeVideoPreview();
                previewRetriever = new MediaMetadataRetriever();
                previewRetriever.setDataSource(path);
                previewPath = path;
            }
            String durationText = previewRetriever.extractMetadata(
                    MediaMetadataRetriever.METADATA_KEY_DURATION);
            long durationUs = durationText == null ? 0L : Long.parseLong(durationText) * 1000L;
            long loopedTimeUs = durationUs > 0L ? Math.floorMod(timeUs, durationUs) : Math.max(0L, timeUs);
            Bitmap bitmap = previewRetriever.getScaledFrameAtTime(
                    loopedTimeUs, MediaMetadataRetriever.OPTION_CLOSEST,
                    512, 256);
            if (bitmap == null) return null;
            int[] argb = new int[512 * 256];
            bitmap.getPixels(argb, 0, 512, 0, 0, 512, 256);
            bitmap.recycle();
            byte[] rgba = new byte[argb.length * 4];
            for (int i = 0, j = 0; i < argb.length; i++, j += 4) {
                int p = argb[i];
                rgba[j] = (byte) (p >>> 16);
                rgba[j + 1] = (byte) (p >>> 8);
                rgba[j + 2] = (byte) p;
                rgba[j + 3] = (byte) 255;
            }
            return rgba;
        } catch (Exception ex) {
            Log.e("VoxenVideo", "Video preview decode failed", ex);
            closeVideoPreview();
            return null;
        }
    }

    public synchronized void closeVideoPreview() {
        if (previewRetriever != null) {
            previewRetriever.release();
            previewRetriever = null;
        }
        previewPath = null;
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
