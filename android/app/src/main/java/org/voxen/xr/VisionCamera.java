package org.voxen.xr;
import android.app.Activity;
import android.hardware.camera2.*;
import android.media.Image;
import android.media.ImageReader;
import android.graphics.ImageFormat;
import android.os.*;
import android.util.Log;
import android.util.Size;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.Collections;

final class VisionCamera {
    static { System.loadLibrary("voxen"); }
    static native int nativeInit(String path);
    static native void nativeFrame(byte[] pixels,int width,int height);
    private final Activity activity;
    private HandlerThread thread;
    private Handler handler;
    private CameraDevice camera;
    private CameraCaptureSession session;
    private ImageReader reader;
    private long last;
    private int frames;
    private volatile boolean running;
    VisionCamera(Activity activity) { this.activity=activity; }
    void start() {
        if(thread!=null) return;
        File root=new File(activity.getExternalFilesDir(null),"References");
        root.mkdirs();
        thread=new HandlerThread("VoxenVision");
        thread.start();
        handler=new Handler(thread.getLooper());
        running=true;
        handler.post(()-> {
            try {
                if(nativeInit(root.getAbsolutePath())==0) return;
                CameraManager manager=activity.getSystemService(CameraManager.class);
                for(String id:manager.getCameraIdList()) {
                    CameraCharacteristics c=manager.getCameraCharacteristics(id);
                    android.hardware.camera2.params.StreamConfigurationMap map=c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                    if(map==null) continue;
                    Size[] sizes=map.getOutputSizes(ImageFormat.YUV_420_888);
                    if(sizes==null || sizes.length==0) continue;
                    Size size=sizes[0];
                    for(Size s:sizes) if(Math.abs(s.getWidth()-1280)<Math.abs(size.getWidth()-1280)) size=s;
                    reader=ImageReader.newInstance(size.getWidth(),size.getHeight(),ImageFormat.YUV_420_888,2);
                    reader.setOnImageAvailableListener(this::frame,handler);
                    Log.i("VoxenVision","Opening camera "+id+" "+size);
                    try { manager.openCamera(id,new CameraDevice.StateCallback(){
                        public void onOpened(CameraDevice d) {
                            if(!running){d.close();return;}
                            camera=d;
                            Log.i("VoxenVision","Camera opened "+d.getId());
                            try {
                                d.createCaptureSession(Collections.singletonList(reader.getSurface()),new CameraCaptureSession.StateCallback(){
                                    public void onConfigured(CameraCaptureSession s){
                                        if(!running){s.close();return;}
                                        session=s;
                                        Log.i("VoxenVision","Capture session configured");
                                        try {
                                            CaptureRequest.Builder b=d.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                                            b.addTarget(reader.getSurface());
                                            s.setRepeatingRequest(b.build(),null,handler);
                                            Log.i("VoxenVision","Repeating capture started");
                                        } catch(Exception e){Log.e("VoxenVision","Capture",e);}
                                    }
                                    public void onConfigureFailed(CameraCaptureSession s){Log.e("VoxenVision","Configure failed");}
                                },handler);
                            } catch(Exception e){Log.e("VoxenVision","Session",e);}
                        }
                        public void onDisconnected(CameraDevice d){d.close();}
                        public void onError(CameraDevice d,int e){d.close();Log.e("VoxenVision","Camera error "+e);}
                    },handler);
                    return;
                    } catch(SecurityException e) {
                        Log.w("VoxenVision","Camera "+id+" denied; trying next",e);
                        reader.close(); reader=null;
                    }
                }
                Log.w("VoxenVision","No YUV camera");
            } catch(Exception | LinkageError e){Log.e("VoxenVision","Start",e);}
        });
    }
    private void frame(ImageReader r){
        try(Image img=r.acquireLatestImage()){
            if(img==null || !running) return;
            long now=SystemClock.elapsedRealtime();
            if(now-last<200) return;
            last=now;
            int w=img.getWidth(),h=img.getHeight();
            Image.Plane p=img.getPlanes()[0];
            ByteBuffer b=p.getBuffer();
            int stride=p.getRowStride(),pixel=p.getPixelStride(),base=b.position();
            byte[] gray=new byte[w*h];
            for(int y=0;y<h;y++){
                if(pixel==1){b.position(base+y*stride);b.get(gray,y*w,w);}
                else for(int x=0;x<w;x++)gray[y*w+x]=b.get(base+y*stride+x*pixel);
            }
            if (++frames % 10 == 1) Log.i("VoxenVision","Frame "+frames+" "+w+"x"+h);
            nativeFrame(gray,w,h);
        } catch(Exception e){Log.e("VoxenVision","Frame",e);}
    }
    void stop(){
        if(thread==null)return;
        running=false;
        HandlerThread old=thread;
        handler.post(()->{
            if(session!=null){session.close();session=null;}
            if(camera!=null){camera.close();camera=null;}
            if(reader!=null){reader.close();reader=null;}
            old.quitSafely();
        });
        try{old.join();}catch(InterruptedException e){Thread.currentThread().interrupt();}
        thread=null;
    }
}
