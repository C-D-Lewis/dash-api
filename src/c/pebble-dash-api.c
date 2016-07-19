#include "pebble-dash-api.h"

#include <pebble-events/pebble-events.h>

#define INBOX_SIZE  256
#define OUTBOX_SIZE 256
#define DELAY_MS    200   // Enable opening AppMessage and an API query in the same event loop

typedef enum {
  RequestTypeGetData = 24784,
  RequestTypeSetFeature = 24785,
  RequestTypeGetFeature = 24786,
  RequestTypeResult = 24787
} RequestType;

typedef enum {
  AppKeyFeatureType = 47836,
  AppKeyFeatureState = 47837,
  AppKeyDataType = 47838,
  AppKeyDataValue = 47839,
  AppKeyUsesDashAPI = 47840,
  AppKeyAppName = 47841,
  AppKeyResultCode = 47842,
  AppKeyLibraryVersion = 47843
} AppKey;

static DashAPIDataCallback *s_last_get_data_cb;
static DashAPIFeatureCallback *s_last_set_feature_cb, *s_last_get_feature_cb;
static DashAPIResultCallback *s_result_callback;

static FeatureType s_last_get_feature_type, s_last_set_feature_type;
static DataType s_last_get_data_type;
static DictionaryIterator *s_outbox;
static char s_app_name[32];

/********************************* Internal ***********************************/

static bool in_flight() {
  return s_last_get_data_cb || s_last_set_feature_cb || s_last_get_feature_cb;
}

static void clear_callbacks() {
  if(s_last_get_data_cb) {
    s_last_get_data_cb = NULL;
  } else if(s_last_set_feature_cb) {
    s_last_set_feature_cb = NULL;
  } else if(s_last_get_feature_cb) {
    s_last_get_feature_cb = NULL;
  }
}

/**
 * Packet Formats 
 *
 * (From Android):
 *   RequestTypeGetData
 *     AppKeyDataType      - DataType
 *   RequestTypeSetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 *   RequestTypeGetFeature
 *     AppKeyFeatureType   - FeatureType
 *   RequestTypeResult
 *     AppKeyLibraryVersion
 *
 * (To Android):
 * HEADER:
 *   AppKeyUsesDashAPI
 *   AppKeyAppName
 * OTHER:
 *   RequestTypeGetData
 *     AppKeyDataType      - DataType
 *     AppKeyDataValue     - DataValue
 *   RequestTypeSetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 *   RequestTypeGetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 *   RequestTypeResult
 *     AppKeyResultCode    - ResultCodeNoPermissions | ResultCodeWrongVersion
 */
static void inbox_received_handler(DictionaryIterator *inbox, void *context) {
  if(!in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: no callbacks");
    return;  // Nothing expected, ignore
  }

  // Get data response
  if(dict_find(inbox, RequestTypeGetData)) {
    DataValue value;

    int type = dict_find(inbox, AppKeyDataType)->value->int32;
    switch(type) {
      // Data type will be integer
      case DataTypeBatteryPercent:
      case DataTypeGSMStrength:
      case DataTypeStoragePercentUsed:
        value.integer_value = dict_find(inbox, AppKeyDataValue)->value->int32;
        s_last_get_data_cb(type, value);
        break;

      // Data type will be string
      case DataTypeWifiNetworkName:
      case DataTypeGSMOperatorName:
      case DataTypeStorageFreeGBString:
        value.string_value = malloc(INBOX_SIZE);
        strcpy(value.string_value, dict_find(inbox, AppKeyDataValue)->value->cstring);
        s_last_get_data_cb(type, value);
        free(value.string_value);
        break;
    }
  }

  // Set feature response
  else if(dict_find(inbox, RequestTypeSetFeature)) {
    int type = dict_find(inbox, AppKeyFeatureType)->value->int32;
    int state = dict_find(inbox, AppKeyFeatureState)->value->int32;
    s_last_set_feature_cb(type, state);
  }

  // Get feature response
  else if(dict_find(inbox, RequestTypeGetFeature)) {
    int type = dict_find(inbox, AppKeyFeatureType)->value->int32;
    int state = dict_find(inbox, AppKeyFeatureState)->value->int32;
    s_last_get_feature_cb(type, state);
  } 

  // Is available result, or no permission result
  else if(dict_find(inbox, RequestTypeResult)) {
    int code = dict_find(inbox, AppKeyResultCode)->value->int32;
    switch(code) {
      case ResultCodeNoPermissions:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Permission for this app has not been granted within the Dash API Android app!");
        break;
      case ResultCodeWrongVersion:
        APP_LOG(APP_LOG_LEVEL_ERROR, "An incompatible version of the Dash API Android app is installed!");
        break;
    }
    s_result_callback(code);
  }

  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Unknown message type");
  }

  clear_callbacks();
}

static void write_header() {
  const int dummy = 0;
  dict_write_int(s_outbox, AppKeyUsesDashAPI, &dummy, sizeof(int), true);
  dict_write_cstring(s_outbox, AppKeyAppName, s_app_name);
}

static bool prepare_outbox() {
  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Bluetooth is disconnected!");
    s_result_callback(ResultCodeSendingFailed);
    return false;
  }

  if(in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_get_data() failed - request already in progress!");
    s_result_callback(ResultCodeSendingFailed);
    return false;
  }

  AppMessageResult result = app_message_outbox_begin(&s_outbox);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error opening outbox!");
    s_result_callback(ResultCodeSendingFailed);
    return false;
  }

  write_header();

  return true;
}

static void send_outbox_callback() {
  if(app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error sending outbox!");
    s_result_callback(ResultCodeSendingFailed);
  }
}

static void send_outbox() {
  app_timer_register(DELAY_MS, send_outbox_callback, NULL);
}

/************************************ API *************************************/

void dash_api_get_data(DataType type, DashAPIDataCallback *callback) {
  if(!prepare_outbox()) {
    return;
  }

  const int dummy = 0;
  dict_write_int(s_outbox, RequestTypeGetData, &dummy, sizeof(int), true);
  dict_write_int(s_outbox, AppKeyDataType, &type, sizeof(int), true);

  s_last_get_data_type = type;
  s_last_get_data_cb = callback;
  send_outbox();
}

void dash_api_set_feature(FeatureType type, FeatureState new_state, DashAPIFeatureCallback *callback) {
  if(!prepare_outbox()) {
    return;
  }

  const int dummy = 0;
  dict_write_int(s_outbox, RequestTypeSetFeature, &dummy, sizeof(int), true);
  dict_write_int(s_outbox, AppKeyFeatureType, &type, sizeof(int), true);
  const int state = (int)new_state; // Prevents 2 becoming 119762434
  dict_write_int(s_outbox, AppKeyFeatureState, &state, sizeof(int), true);

  s_last_set_feature_type = type;
  s_last_set_feature_cb = callback;
  send_outbox();
}

void dash_api_get_feature(FeatureType type, DashAPIFeatureCallback *callback) {
  if(!prepare_outbox()) {
    return;
  }

  const int dummy = 0;
  dict_write_int(s_outbox, RequestTypeGetFeature, &dummy, sizeof(int), true);
  dict_write_int(s_outbox, AppKeyFeatureType, &type, sizeof(int), true);

  s_last_get_feature_type = type;
  s_last_get_feature_cb = callback;
  send_outbox();
}

void dash_api_init(char *app_name, DashAPIResultCallback *callback) {
  s_result_callback = callback;
  snprintf(s_app_name, sizeof(s_app_name), "%s", app_name);

  events_app_message_register_inbox_received(inbox_received_handler, NULL);
  events_app_message_request_inbox_size(INBOX_SIZE);
  events_app_message_request_outbox_size(OUTBOX_SIZE);
}

void dash_api_is_available() {
  if(!prepare_outbox()) {
    return;
  }

  const int version = DASH_API_VERSION;
  dict_write_int(s_outbox, AppKeyLibraryVersion, &version, sizeof(int), true);

  send_outbox();
}
