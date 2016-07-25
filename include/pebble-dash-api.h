#pragma once

#include <pebble.h>

#define DASH_API_VERSION "1.2"

/******************************** Enumerations ********************************/

// Types of data that can requested from the phone
typedef enum {
  DataTypeBatteryPercent = 678342,  // (integer_value) The percentage of battery life remaining
  DataTypeGSMOperatorName,          // (string_value)  The GSM operator name 
  DataTypeGSMStrength,              // (integer_value) The GSM network strength
  DataTypeWifiNetworkName,          // (string_value)  The Wifi network name
  DataTypeStoragePercentUsed,       // (integer_value) The used internal storage space as a percentage
  DataTypeStorageFreeGBString,      // (string_value)  The free space of the phone, measured in GB
  DataTypeUnreadSMSCount,           // (integer_value) The number of unread SMS messages, if it could be read.
  DataTypeNextCalendarEventOneLine, // (string_value)  A short-form of the next calendar event, if any. "" if not.
                                    //                 The string will the time and title (e.g.: "14:00 Design Review")
                                    //                 This is derived from a collection of all events in all calendars
                                    //                 that occur within the next 24 hours.
  DataTypeNextCalendarEventTwoLine  // (string_value)  A two lined version of DataTypeNextCalendarEventOneLine.
                                    //                 The string will the time date on top, and title below 
                                    //                 (e.g.: "24 Jul 14:00\nDesign Review").
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

// Result codes for callbacks
typedef enum {
  ErrorCodeSuccess = 0,            // The request was made successfully
  ErrorCodeSendingFailed,          // The sending of the request failed, or there was no connection
  ErrorCodeUnavailable,            // The request timed out or the Android app was unavailable, or not installed
  ErrorCodeNoPermissions,          // This app has not been permitted in the Dash API Android app
  ErrorCodeWrongVersion            // An old or incompatible version of the Dash API Android app is installed
} ErrorCode;

/********************************* Callbacks **********************************/

// Callback called when a Set request returns from the phone side of this library.
// Parameters:
//   FeatureType  - The type of feature that was originally requested.
//   FeatureState - The state of the feature that was originally requested, or FeatureStateUnknown
//                  when dash_api_get_feature() fails.
typedef void(DashAPIFeatureCallback)(FeatureType, FeatureState);

// Callback called when a Get request returns from the phone side of this library.
// Parameters:
//   DataType   - The type of data that was requested.
//   DataValue  - The data returned by the request (valid for the duration of the callback).
typedef void(DashAPIDataCallback)(DataType, DataValue);

// Callback called when a request fails for some reason.
// Parameters:
//   ErrorCode - The code representing the result of the request
typedef void(DashAPIErrorCallback)(ErrorCode);

/************************************ API *************************************/

// Get some data from the phone side of this library.
// Parameters:
//   type     - The type of data to get.
//   callback - The callback called when the request succeeds or fails.
void dash_api_get_data(DataType type, DashAPIDataCallback *callback);

// Change the state of a phone feature.
// Parameters:
//   type      - The type of feature to change state of.
//   new_state - The state to set this feature into.
//   callback  - The callback called when the request succeeds or fails.
void dash_api_set_feature(FeatureType type, FeatureState new_state, DashAPIFeatureCallback *callback);

// Get the state of a feature on the phone.
// Parameters:
//   type     - The type of feature to get the state of.
//   callback - The callback called when the request succeeds or fails.
void dash_api_get_feature(FeatureType type, DashAPIFeatureCallback *callback);

// Intialise the library by calling this function before any of the others.
// Parameters:
//   app_name - The name of your app. This will be used to allow the user to manage permissions.
//              Max 32 characters.
//   callback - A central result callback that will be notified of various results of requests
//              using values of ErrorCode.
void dash_api_init(char *app_name, DashAPIErrorCallback *callback);

// Check to see if the Dash API is available. If the Android app is not installed, the request will
// likely result in ErrorCodeUnavailable. The result will be delievered to the DashAPIErrorCallback
// registered with dash_api_init().
void dash_api_check_is_available();
