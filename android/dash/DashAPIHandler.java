package dash;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;

import com.getpebble.android.kit.util.PebbleDictionary;

public class DashAPIHandler {

    public static void handleGetData(Context context, int type, PebbleDictionary out) {
        switch(type) {
            case DashAPIKeys.DataTypeBatteryPercent:
                Intent batteryStatus = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
                int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
                out.addInt32(DashAPIKeys.AppKeyDataValue, Math.round((level / scale) * 100.0F));
                break;
            case DashAPIKeys.DataTypeGSMOperatorName:
                break;
            case DashAPIKeys.DataTypeGSMStrength:
                break;
            case DashAPIKeys.DataTypeWifiNetworkName:
                WifiManager wifiMgr = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                WifiInfo wifiInfo = wifiMgr.getConnectionInfo();
                String name = wifiInfo.getSSID();
                name = name.replace("\"", "");
                if(name.equals("0x")) {
                    // On, but disconnected?
                    name = "Disconnected";
                } else if(name.equals("<unknown ssid>")) {
                    name = "Unknown";
                }
                out.addString(DashAPIKeys.AppKeyDataValue, name);
                break;
            case DashAPIKeys.DataTypeStorageFreePercent:
                break;
            case DashAPIKeys.DataTypeStorageFreeGBMajor:
                break;
            case DashAPIKeys.DataTypeStorageFreeGBMinor:
                break;
        }
    }

    public static void handleSetFeature(Context context, int featureType, int featureState, PebbleDictionary out) {
        switch(featureType) {
            case DashAPIKeys.FeatureTypeWifi:
                break;
            case DashAPIKeys.FeatureTypeBluetooth:
                break;
            case DashAPIKeys.FeatureTypeRinger:
                break;
            case DashAPIKeys.FeatureTypeAutoSync:
                break;
            case DashAPIKeys.FeatureTypeHotSpot:
                break;
            case DashAPIKeys.FeatureTypeAutoBrightness:
                break;
        }
    }

    public static void handleGetFeature(Context context, int featureType, PebbleDictionary out) {
        switch(featureType) {
            case DashAPIKeys.FeatureTypeWifi:
                break;
            case DashAPIKeys.FeatureTypeBluetooth:
                break;
            case DashAPIKeys.FeatureTypeRinger:
                break;
            case DashAPIKeys.FeatureTypeAutoSync:
                break;
            case DashAPIKeys.FeatureTypeHotSpot:
                break;
            case DashAPIKeys.FeatureTypeAutoBrightness:
                break;
        }
    }

}
