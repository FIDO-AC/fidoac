package anon.fidoac.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class FIDOACService extends Service {

    private HttpServer server;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String challenge = intent.getStringExtra("challenge");
        Log.i("FIDOAC","Challenge "+challenge);


        try {
            Map<String, String> data = new HashMap<>();
            data.put("snark","test");
            data.put("challenge",challenge);
            server = new HttpServer(data,challenge);
        } catch (IOException e) {
            e.printStackTrace();
        }

        new Thread(new Runnable(){
            public void run() {
                while(true)
                {
                    try {
                        Thread.sleep(5000);
                        if(server.isDataFetched()){
                            Log.i("FIDOAC","Killing service");
                            server.stop();
                            stopSelf();
                            return;
                        }else{
                            Log.i("FIDOAC","Service still not served");
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();
        return START_NOT_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


}
