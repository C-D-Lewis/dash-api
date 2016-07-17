package dash;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Environment;
import android.os.StatFs;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.getpebble.android.kit.util.PebbleDictionary;

import static android.content.ContentValues.TAG;

public class DashAPIHandler {

    public static void handleGetData(Context context, int type, final PebbleDictionary out) {
        switch(type) {
            case DashAPIKeys.DataTypeBatteryPercent:
                Intent batteryStatus = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
                int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
                out.addInt32(DashAPIKeys.AppKeyDataValue, Math.round((level / scale) * 100.0F));
                break;
            case DashAPIKeys.DataTypeGSMOperatorName:
                TelephonyManager tManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
                String operatorName = tManager.getNetworkOperatorName();
                if (operatorName.length() < 1) {
                    operatorName = "Unknown";
                }
                out.addString(DashAPIKeys.AppKeyDataValue, operatorName);
                break;
            case DashAPIKeys.DataTypeGSMStrength:
                final TelephonyManager manager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
                final SignalListener listener = new SignalListener() {

                    @Override
                    public void onPercentKnown(int percent) {
                        out.addInt32(DashAPIKeys.AppKeyDataValue, percent);
                    }

                };
                manager.listen(listener, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
                new Thread(new Runnable() {

                    @Override
                    public void run() {
                        try {
                            Thread.sleep(2000);
                            manager.listen(listener, PhoneStateListener.LISTEN_NONE);
                        } catch(Exception e) {
                            Log.e(TAG, "Exception waiting to stop listening to signal changes");    // Never happened yet
                            e.printStackTrace();
                        }
                    }

                }).start();
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
            case DashAPIKeys.DataTypeStorageFreeGBString: {
                StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());
                long free = statFs.getFreeBlocksLong() * (statFs.getBlockSizeLong());
                float gigs = free / 1073741824F;
                float temp = gigs;
                int major = (int) Math.floor(temp);
                temp -= major;
                temp *= 10.0F;
                int minor = Math.round(temp) % 10;
                out.addString(DashAPIKeys.AppKeyDataValue, "" + major + "." + minor + " GB");
            }   break;
            case DashAPIKeys.DataTypeStoragePercentUsed: {
                StatFs statFs = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());
                long free = statFs.getFreeBlocksLong() * (statFs.getBlockSizeLong());
                long total = statFs.getBlockCountLong() * statFs.getBlockSizeLong();
                int percent = Math.round(((float) free / (float) total) * 100);
                percent = 100 - percent;
                out.addInt32(DashAPIKeys.AppKeyDataValue, percent);
            }   break;
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
                WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
                out.addInt32(DashAPIKeys.AppKeyFeatureState,
                        wifiManager.isWifiEnabled() ? DashAPIKeys.FeatureStateOn : DashAPIKeys.FeatureStateOff);
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
