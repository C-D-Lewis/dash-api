package dash;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class Service extends android.app.Service {

    private static final String TAG = Service.class.getName();
    private static final boolean DEBUG = true;

    private void parse(PebbleDictionary dict, final UUID uuid) {
        Context context = getApplicationContext();
        final PebbleDictionary out = new PebbleDictionary();

        // Get data request
        if(dict.getInteger(Keys.RequestTypeGetData) != null) {
            out.addInt32(Keys.RequestTypeGetData, 0);

            int type = dict.getInteger(Keys.AppKeyDataType).intValue();
            out.addInt32(Keys.AppKeyDataType, type);
            APIHandler.handleGetData(context, type, out);
        }

        // Set feature request
        if(dict.getInteger(Keys.RequestTypeSetFeature) != null) {
            out.addInt32(Keys.RequestTypeSetFeature, 0);

            int type = dict.getInteger(Keys.AppKeyFeatureType).intValue();
            out.addInt32(Keys.AppKeyFeatureType, type);
            int state = dict.getInteger(Keys.AppKeyFeatureState).intValue();
            out.addInt32(Keys.AppKeyFeatureState, state);

            APIHandler.handleSetFeature(context, type, state, out);
        }

        // Get feature request
        if(dict.getInteger(Keys.RequestTypeGetFeature) != null) {
            out.addInt32(Keys.RequestTypeGetFeature, 0);

            int type = dict.getInteger(Keys.AppKeyFeatureType).intValue();
            out.addInt32(Keys.AppKeyFeatureType, type);
            APIHandler.handleGetFeature(context, type, out);
        }

        // Wait at least xms for GSM signal strength listener to resolve
        // Does not appear to happen immediately
        new Thread(new Runnable() {

            @Override
            public void run() {
                if(out.size() > 1) {    // At least the RequestType tuple
                    PebbleKit.sendDataToPebble(getApplicationContext(), uuid, out);
                    Log.d(TAG, "Sent response to " + uuid.toString());
                    if(DEBUG) {
                        Log.d(TAG, "JSON out: " + out.toJsonString());
                    }
                }
            }

        }).start();

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) {
            Log.e(TAG, "Intent was null");
            return super.onStartCommand(intent, flags, startId);
        }

        try {
            Log.d(TAG, "onStartCommand()");

            //Get dictionary and parse
            Bundle extras = intent.getExtras();
            String jsonData = extras.getString("json");
            if(DEBUG) {
                Log.d(TAG, "JSON in: " + jsonData);
            }

            String uuid = extras.getString("uuid");
            parse(PebbleDictionary.fromJson(jsonData), UUID.fromString(uuid));
        } catch (Exception e) {
            Log.e(TAG, "onStartCommand() threw exception: " + e.getLocalizedMessage());
            e.printStackTrace();
        }

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

}
