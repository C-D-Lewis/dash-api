#pragma once

#include <pebble.h>

/******************************** Enumerations ********************************/

// Types of data that can requested from the phone
typedef enum {
  DataTypeBatteryPercent = 678342,  // (integer_value) The percentage of battery life remaining
  DataTypeGSMOperatorName,          // (string_value)  The GSM operator name 
  DataTypeGSMStrength,              // (integer_value) The GSM network strength
  DataTypeWifiNetworkName,          // (string_value)  The Wifi network name
  DataTypeStoragePercentUsed,       // (integer_value) The used internal storage space as a percentage
  DataTypeStorageFreeGBString,      // (string_value)  The free space of the phone, measured in GB
} DataType;

// Types of feature change that can be requested of the phone
typedef enum {
  FeatureTypeWifi = 467822,         // Set Wifi state (FeatureStateOn | FeatureStateOff)
  FeatureTypeBluetooth,             // Set Bluetooth state (FeatureStateOff)
  FeatureTypeRinger,                // Set Ringer state (FeatureStateRingerLoud | FeatureStateRingerVibrate | FeatureStateRingerSilent)
  FeatureTypeAutoSync,              // Set the AutoSync state (FeatureStateOn | FeatureStateOff)
  FeatureTypeHotSpot,               // Set the hotspot state (FeatureStateOn | FeatureStateOff)
  FeatureTypeAutoBrightness         // Set the backlight auto brightness state (FeatureStateOn | FeatureStateOff)
} FeatureType;

// States for each FeatureType that can be requested
typedef enum {
  FeatureStateUnknown = 0,
  FeatureStateOff,
  FeatureStateOn,
  FeatureStateRingerLoud,
  FeatureStateRingerVibrate,
  FeatureStateRingerSilent
} FeatureState;

// Result values for Get requests. See DataType for which of these to use
typedef struct {
  int integer_value;
  char* string_value;
} DataValue;

/********************************* Callbacks **********************************/

// Callback called when a Set request returns from the phone side of this library
// Parameters:
//   FeatureType  - The type of feature that was originally requested
//   FeatureState - The state of the feature that was originally requested, or FeatureStateUnknown
//                  when dash_api_get_feature() fails.
//   bool         - Whether or not the request was successfully actioned (Can fail on both sides)
typedef void(DashAPIFeatureCallback)(FeatureType, FeatureState, bool);

// Callback called when a Get request returns from the phone side of this library
// Parameters:
//   DataType  - The type of data that was requested
//   DataValue - The data returned by the request (valid for the duration of the callback)
//   bool      - Whether or not the request was successfully actioned (Can fail on both sides)
typedef void(DashAPIDataCallback)(DataType, DataValue, bool);

/************************************ API *************************************/

// Get some data from the phone side of this library
// Parameters:
//   DataType            - The type of data to get
//   DashAPIDataCallback - The callback called when the request succeeds or fails
void dash_api_get_data(DataType type, DashAPIDataCallback *callback);

// Change the state of a phone feature
// Parameters:
//   FeatureType            - The type of feature to change state of
//   FeatureState           - The state to set this feature into
//   DashAPIFeatureCallback - The callback called when the request succeeds or fails 
void dash_api_set_feature(FeatureType type, FeatureState new_state, DashAPIFeatureCallback *callback);

// Get the state of a feature on the phone
// Parameters:
//   FeatureType            - The type of feature to get the state of
//   DashAPIFeatureCallback - The callback called when the request succeeds or fails
void dash_api_get_feature(FeatureType type, DashAPIFeatureCallback *callback);

// If the app does not already use AppMessage, use this to initialize it before using this library.
// If the app does already use AppMessage, its callbacks will need to be re-registered
// after each use of this library.
void dash_api_init_appmessage();
