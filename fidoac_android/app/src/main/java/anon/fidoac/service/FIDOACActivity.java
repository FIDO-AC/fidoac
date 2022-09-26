package anon.fidoac.service;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

public class FIDOACActivity extends Activity {
    @Override
    protected void onStart() {
        super.onStart();
        if(this.getIntent().getData().getQueryParameter("challenge") != null) {
            Intent mIntent = new Intent(this, FIDOACService.class);
            mIntent.putExtra("challenge", this.getIntent().getData().getQueryParameter("challenge"));
            startService(mIntent);
        }else{
            Log.i("FIDOAC","Challenge not found");
        }
    }

}
