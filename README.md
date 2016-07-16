# dash-api

Dash API (in reference to the Dashboard app) allows a Pebble developer to show
data from a connected Android phone, and also control some of its features.


## How It Works

All apps that use the Dash API are serviced by the [Dash API Android app](LINK).
You should instruct your user to install this app in order for them to benefit
from the API.

> This means that any number of watchfaces or watchapps can show Android data or
> control Android features _without a bespoke Android app for each one_.


## Setting Up

1. Install the library into the project `src` directory:

  ```
  $ cp -r pebble/dash-api src
  ```

2. Include the library where appropriate:

  ```c
  #include "dash_api/dash_api.h"
  ```

3. If the app does not already use `AppMessage`, initialise it for Dash API
   when the app is initialising:

  ```c
  static void init() {
    dash_api_init_appmessage();

    /* other init code */
  }
  ```

  > If the app **does** already use `AppMessage`, you will need to re-register
  > your handlers after calling `dash_api_init_appmessage()`, since it uses
  > `AppMessageInboxReceived`.


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


## Available Data

The table below details all the data items currently available via the Dash API.
See the information below to learn how to read the received data.

| Name | ResultValue Type | Example Value | Added In Version |
|------|------------------|---------------|------------------|
| Battery Percentage | `integer_value` | `56` | 1.0 |

`ResultValue` is a union of two members to enable multiple data types. For
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