# dash-api

*Current version: 1.0.1*

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


## How It Works

All apps that use the Dash API are serviced by the 
[Dash API Android app](https://play.google.com/store/apps/details?id=com.wordpress.ninedof.dashapi).
You should instruct your users to install this app in order for them to benefit
from the Dash API.

This is a 'many to one' service, meaning that any number of watchfaces or
watchapps can use the Dash API to show Android data or control Android features
_without a bespoke Android app needing to be written for each one_. Once a user
has installed the Android app, they can install as many Pebble apps that use the
Dash API without any further installation required.

Developers using this library should direct their users to install 'Dash API for
Pebble' from the Google Play Store, or check that they have it installed already.


## Example App

[Dual Gauge](https://github.com/C-D-Lewis/dual-gauge) is an example watchface
that uses this library to show both the watch and phone battery levels.


## Setting Up

1. Install the pebble package:

  ```
  $ pebble package install pebble-dash-api
  ```

2. Include the library where appropriate:

  ```c
  #include <pebble-dash-api/pebble-dash-api.h>
  ```

3. When the app is initialising, set up AppMessage. This uses the 
   `pebble-events` package to play nice with other Pebble packages:

  ```c
  static void init() {
    dash_api_init_appmessage();

    /* other init code */
  }
  ```

4. When all other packages using AppMessage are initialised (if any), open
   AppMessage:

  ```c
  #include <pebble-events/pebble-events.h>
  ```

  ```c
  /* other library AppMessage init code, if any */

  events_app_message_open();
  ```

5. Interact with Android through one of `dash_api_get_data()`,
   `dash_api_set_feature()`, or `dash_api_get_feature()`. See the sections below
   for code examples.

   > Always check the value of `success` in each callback to check for errors.


## Get Data 

To get some data from Android, choose a `DataType` from the Available Data
 table below, and make a call to the API with a handler to receive the result:

```c
static void get_callback(DataType type, DataValue result, bool success) {
  if(success) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone Battery:\n%d%%", result.integer_value);
  }
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
| `DataTypeStorageFreePercent` | `integer_value` | `22` | 1.0 |
| `DataTypeStorageFreeGBString` | `string_value` | `6.2 GB` | 1.0 |
| `DataTypeGSMOperatorName` | `string_value` | `Three UK` | 1.0 |
| `DataTypeGSMStrength` | `integer_value` | `68` | 1.0 |

`ResultValue` is a structure of two members to enable multiple data types. For
example, battery percentage will be read as an integer:

```c
static void get_callback(DataType type, DataValue result, bool success) {
  if(success) {
    int battery_percent = result.integer_value;
  }
}
```

whereas GSM operator name will be read as a string:

> The string will only be valid for the duration of the callback, so should be
> copied out as required.

```c
static void get_callback(DataType type, DataValue result, bool success) {
  if(success) {
    static char s_buff[32];
    snprintf(s_buff, sizeof(s_buff), "GSM Operator: %s", result.string_value);
    APP_LOG(APP_LOG_LEVEL_INFO, "%s", s_buff);
  }
}
```


## Set a Feature State

To set the state of an Android feature (for example, turning on WiFi):

```c
static void set_callback(FeatureType type, FeatureState new_state, bool success) {
  if(success) {
    APP_LOG(APP_LOG_LEVEL_INFO, "WiFi turned on successfully!");
  }
}

dash_api_set_feature(FeatureTypeWifi, FeatureStateOn, set_callback);
```


## Get a Feature State

To get the current state of a feature (for example, the state of WiFi):

> This will be a value from the `FeatureState` `enum`:

```c
static void get_feature_callback(FeatureType type, FeatureState new_state, bool success) {
  if(success) {
    APP_LOG(APP_LOG_LEVEL_INFO, "WiFi state now %d!", new_state);
  }
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

> Since the only sensible Bluetooth instruction is to turn it off, there may not
> be a callback when using `FeatureTypeBluetooth`. You should assume that it has
> been turned off.

| Name | Set Values | Added In Version |
|------|------------|------------------|
| `FeatureTypeWifi` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeBluetooth` | `FeatureStateOff` | 1.0 |
| `FeatureTypeRinger` | `FeatureStateRingerLoud`, `FeatureStateRingerVibrate`, `FeatureStateRingerSilent` | 1.0 |
| `FeatureTypeAutoSync` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeHotSpot` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |
| `FeatureTypeAutoBrightness` | `FeatureStateOn`, `FeatureStateOff` | 1.0 |

> `FeatureTypeHotSpot` may take a few seconds to turn on and off, depending on the phone model.


## TODO

- Protocol can be optimized into fewer keys
- Investigate popular requests (Unread SMS count, missed calls, next calendar event etc)
- More agressive parameter checking
- Use Locker API to show apps that made recent requests
