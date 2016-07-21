# dash-api

*Current API version: 1.1*

*Current published Android app version: 1.1*

Dash API (in reference to the 
[Dashboard app](https://play.google.com/store/apps/details?id=com.wordpress.ninedof.dashboard)) 
allows a Pebble developer to access some Android APIs through a shared Android 
app.

- [Setting Up](#setting-up)
- [Get Data](#get-data)
- [Available Data](#available-data)
- [Set a Feature State](#set-a-feature-state)
- [Get a Feature State](#get-a-feature-state)
- [Available Features](#available-features)
- [Error Codes](#error-codes)
- [Changelog](#changelog)


## How It Works

All apps that use the Dash API are serviced by the 
[Dash API Android app](https://play.google.com/store/apps/details?id=com.wordpress.ninedof.dashapi).
You should instruct your users to install this app in order for them to benefit
from the Dash API.

This is a 'many to one' service, meaning that any number of watchfaces or
watchapps can use the Dash API to show Android data or control Android features
_without a bespoke Android app needing to be written for each one_. Once a user
has installed the Android app, they can install as many Pebble apps that use the
Dash API without any further Android app installation required.

Developers using this library should direct their users to install 'Dash API for
Pebble' from the Google Play Store, or check that they have it installed
already. The library will inform you of this if a request times out by
delivering `ErrorCodeUnavailable` to your `DashAPIErrorCallback`.


## Example App

[Dual Gauge](https://github.com/C-D-Lewis/dual-gauge) is an example watchface
that uses this library to show both the watch and phone battery levels.


## Important Notes

- Since all messages to and from Pebble use the same outbox, developers should
  not attempt simultaneous requests with the Dash API. Once the callback has been
  received for one request it is safe to then make the next, etc. 

- When using `dash_api_check_is_available()`, wait for `ErrorCodeSuccess` in the
  `error_callback` before making further requests. This is a good best practice
  to follow when your app opens and begins making queries to the Dash API.


## Setting Up

1. Install the pebble package:

  ```
  $ pebble package install pebble-dash-api
  ```

2. Include the library where appropriate:

  ```c
  #include <pebble-dash-api/pebble-dash-api.h>
  ```

3. When the app is initialising, set up AppMessage for Dash API. This uses the 
   `pebble-events` package to play nice with other Pebble packages. Include the 
   name of your app (used in the Android app), and a callback to receive 
   `ErrorCode` values:

  ```c
  #define APP_NAME "My Dash API Client App"
  ```

  ```c
  static void error_callback(ErrorCode code) { 
    APP_LOG(APP_LOG_LEVEL_INFO, "Got ErrorCode %d", code);
  }
  ```

  ```c
  static void init() {
    dash_api_init(APP_NAME, error_callback);

    /* other init code */
  }
  ```

4. When all other packages using AppMessage are initialised (if any), open
   AppMessage for them all:

  ```c
  #include <pebble-events/pebble-events.h>
  ```

  ```c
  /* other library AppMessage init code, if any */

  events_app_message_open();
  ```

5. Check the library is available at the other end:

  > Wait for the callback before making further requests.

  ```c
  static void error_callback(ErrorCode code) {
    if(code == ErrorCodeSuccess) {
      // Available!
    } else if(code == ErrorCodeUnavailable 
           || code == ErrorCodeWrongVersion) {
      // Timed out, or was wrong version of the Android app
    }
  }

  dash_api_check_is_available();
  ```

6. Interact with Android through one of `dash_api_get_data()`,
   `dash_api_set_feature()`, or `dash_api_get_feature()`. See the sections below
   for code examples.


## Get Data 

To get some data from Android, choose a `DataType` from the Available Data
 table below, and make a call to the API with a handler to receive the result:

```c
static void get_callback(DataType type, DataValue result) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Phone Battery:\n%d%%", result.integer_value);
}

dash_api_get_data(DataTypeBatteryPercent, get_callback);
```


## Available Data

The table below details all the data items currently available via the Dash API.
See the information below the table to learn how to read the received data.

| Name | ResultValue Type | Example Value | Added In Version |
|------|------------------|---------------|------------------|
| `DataTypeBatteryPercent` | `integer_value` | `56` | 1.0 |
| `DataTypeWifiNetworkName` | `string_value` | `BTHub3-NCNR` | 1.0 |
| `DataTypeStoragePercentUsed` | `integer_value` | `22` | 1.0 |
| `DataTypeStorageFreeGBString` | `string_value` | `6.2 GB` | 1.0 |
| `DataTypeGSMOperatorName` | `string_value` | `Three UK` | 1.0 |
| `DataTypeGSMStrength` | `integer_value` | `68` | 1.0 |

`ResultValue` is a structure of two members to enable multiple data types. For
example, battery percentage will be read as an integer:

```c
static void get_callback(DataType type, DataValue result) {
  int battery_percent = result.integer_value;
}
```

Whereas GSM operator name will be read as a string:

> The string will only be valid for the duration of the callback, so should be
> copied out as required.

```c
static void get_callback(DataType type, DataValue result) {
  static char s_buff[32];
  snprintf(s_buff, sizeof(s_buff), "GSM Operator: %s", result.string_value);
  APP_LOG(APP_LOG_LEVEL_INFO, "%s", s_buff);
}
```


## Set a Feature State

> To _set_ the state of a feature, the client app will need to be granted
> permission in the Dash API Android app. The user will be notified and asked to
> switch it on within the app. This applies _only_ to setting a feature state.

To set the state of an Android feature (for example, turning on WiFi):

```c
static void set_callback(FeatureType type, FeatureState new_state) {
  APP_LOG(APP_LOG_LEVEL_INFO, "WiFi request successful!");
}

dash_api_set_feature(FeatureTypeWifi, FeatureStateOn, set_callback);
```


## Get a Feature State

To get the current state of a feature (for example, the state of WiFi):

> This will be a value from the `FeatureState` `enum`:

```c
static void get_feature_callback(FeatureType type, FeatureState new_state) {
  APP_LOG(APP_LOG_LEVEL_INFO, "WiFi state now %d!", new_state);
}

dash_api_get_feature(FeatureTypeWifi, get_feature_callback);
```


## Available Features

The table below details all the Android features that can be interacted with via
the Dash API. See the information below the table to learn how to read the
received data.

When using `dash_api_get_feature()`, the callback parameters will reflect the
current state of the feature.

When using `dash_api_set_feature()`, the callback parameters will mirror those
of the request.

| Name | Set Values | Added In Version |
|------|------------|------------------|
| `FeatureTypeWifi` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeBluetooth` | `FeatureStateOff` | 1.0 |
| `FeatureTypeRinger` | `FeatureStateRingerLoud`, `FeatureStateRingerVibrate`, `FeatureStateRingerSilent` | 1.0 |
| `FeatureTypeAutoSync` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeHotSpot` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeAutoBrightness` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |

> Since the only sensible Bluetooth instruction is to turn it off, there may not
> be a callback when using `FeatureTypeBluetooth`. You should assume that it has
> been turned off.
> 
> `FeatureTypeHotSpot` may take a few seconds to turn on and off, depending on
> the phone model.


## Error Codes

When a request fails, the reason will be communicated over app logs, as well as 
by delivering a `ErrorCode` value to the `DashAPIErrorCallback` registered with
`dash_api_init()`. The table below describes these values.

| Code | Description | Added In Version |
|------|-------------|------------------|
| `ErrorCodeSuccess` | The request was made successfully. Used for a `dash_api_check_is_available()` response. | 1.1 |
| `ErrorCodeSendingFailed` | The sending of the request failed, or there was no connection. | 1.1 |
| `ErrorCodeUnavailable` | The request timed out or the Android app was unavailable, or not installed. | 1.1 |
| `ErrorCodeNoPermissions` | This app has not been permitted in the Dash API Android app. | 1.1 |
| `ErrorCodeWrongVersion` | An old or incompatible version of the Dash API Android app is installed. | 1.1 |


## Changelog

**1.0**
- Initial release.

**1.0.1**
- Update README.md on NPM.

**1.1**
- Add `app_name` and `DashAPIErrorCallback` to `dash_api_init()` (formerly 
  `dash_api_init_appmessage()`).
- Change signatures of the main callbacks, directed failed result of requests to 
  the `DashAPIErrorCallback`.
- Add permission switches in the Android app for clients that wish to _set_ 
  a feature state. Reading data is unaffected, and does not require permission.
- Added a timeout mechanism to inform when the Android app may be unavailable.
- Added `dash_api_check_is_available()` to check the library is available. 
  `ErrorCodeSuccess` will be delivered to the `  DashAPIErrorCallback` if it is 
  available.


## TODO

These items are desirable, but not guaranteed to be added. 

- Protocol can be optimized into fewer keys
- Unread SMS count, missed calls etc
- Next Calendar event
- Music control
