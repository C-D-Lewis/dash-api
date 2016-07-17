package dash;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

import java.util.UUID;

public class DashAPIService extends Service {

    private static final String TAG = DashAPIService.class.getName();
    private static final boolean DEBUG = true;

    private void parse(PebbleDictionary dict, final UUID uuid) {
        Context context = getApplicationContext();
        final PebbleDictionary out = new PebbleDictionary();

        // Get data request
        if(dict.getInteger(DashAPIKeys.RequestTypeGetData) != null) {
            out.addInt32(DashAPIKeys.RequestTypeGetData, 0);

            int type = dict.getInteger(DashAPIKeys.AppKeyDataType).intValue();
            out.addInt32(DashAPIKeys.AppKeyDataType, type);
            DashAPIHandler.handleGetData(context, type, out);
        }

        // Set feature request
        if(dict.getInteger(DashAPIKeys.RequestTypeSetFeature) != null) {
            out.addInt32(DashAPIKeys.RequestTypeSetFeature, 0);

            int type = dict.getInteger(DashAPIKeys.AppKeyFeatureType).intValue();
            out.addInt32(DashAPIKeys.AppKeyFeatureType, type);
            int state = dict.getInteger(DashAPIKeys.AppKeyFeatureState).intValue();
            DashAPIHandler.handleSetFeature(context, type, state, out);
        }

        // Get feature request
        if(dict.getInteger(DashAPIKeys.RequestTypeGetFeature) != null) {
            out.addInt32(DashAPIKeys.RequestTypeGetFeature, 0);

            int type = dict.getInteger(DashAPIKeys.AppKeyFeatureType).intValue();
            out.addInt32(DashAPIKeys.AppKeyFeatureType, type);
            DashAPIHandler.handleGetFeature(context, type, out);
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
