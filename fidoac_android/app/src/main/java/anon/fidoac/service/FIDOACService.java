package anon.fidoac.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import anon.fidoac.FileManager;

public class FIDOACService extends Service {

    private HttpServer server;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String challenge = intent.getStringExtra(FileManager.SERVER_CHALLENGE_ID);
        String proof = intent.getStringExtra(FileManager.PROOF_ID);
        String hash = intent.getStringExtra(FileManager.HASH_ID);
        String sign = intent.getStringExtra(FileManager.MEDIATOR_SIGNATURE_ID);
        int age_gt = intent.getIntExtra(FileManager.AGEGT_ID, 0);
        int cur_year = intent.getIntExtra(FileManager.CURYEAR_ID, 0);
        ArrayList<String> certs = intent.getStringArrayListExtra(FileManager.MEDIATOR_CERT_ID);

        Log.i("FIDOAC","Challenge: "+challenge);

        try {
            Map<String, String> data = new HashMap<>();
            data.put(FileManager.PROOF_ID,proof);
            data.put(FileManager.SERVER_CHALLENGE_ID,challenge);
            data.put(FileManager.HASH_ID, hash);
            data.put(FileManager.AGEGT_ID, Integer.toString(age_gt));
            data.put(FileManager.CURYEAR_ID, Integer.toString(cur_year));
            data.put(FileManager.MEDIATOR_SIGNATURE_ID, sign);
            int counter=0;
            for (String cert : certs){
                data.put(FileManager.MEDIATOR_CERT_ID+"_"+counter, cert);
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
