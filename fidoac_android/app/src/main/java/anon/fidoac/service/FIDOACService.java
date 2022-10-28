package anon.fidoac.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashMap;
import java.util.Map;

public class FIDOACService extends Service {

    private HttpServer server;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String challenge = intent.getStringExtra("challenge");
        String proof = intent.getStringExtra("proof");
        String hash = intent.getStringExtra("hash");
        String sign = intent.getStringExtra("mediator_sign");
        ArrayList<String> certs = intent.getStringArrayListExtra("mediator_cert");

        Log.i("FIDOAC","Challenge "+challenge);

        try {
            Map<String, String> data = new HashMap<>();
            data.put("client_zkproof",proof);
            data.put("relying_party_challenge",challenge);
            data.put("client_randomized_hash", hash);
            data.put("mediator_sign", sign);
            int counter=0;
            for (String cert : certs){
                data.put("mediator_cert_"+counter, cert);
                counter+=1;
            }

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
