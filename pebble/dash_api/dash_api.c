#include "dash_api.h"

#define INBOX_SIZE  128
#define OUTBOX_SIZE 256
#define DELAY_MS    200   // Enable opening AppMessage and an API query in the same event loop

typedef enum {
  RequestTypeGetData = 24784,
  RequestTypeSetFeature = 24785,
  RequestTypeGetFeature = 24786
} RequestType;

typedef enum {
  AppKeyFeatureType = 47836,
  AppKeyFeatureState = 47837,
  AppKeyDataType = 47838,
  AppKeyDataValue = 47839,
  AppKeyUsesDashAPI = 47840
} AppKey;

static DashAPIDataCallback *s_last_get_data_cb;
static DashAPIFeatureCallback *s_last_set_feature_cb, *s_last_get_feature_cb;
static FeatureType s_last_get_feature_type, s_last_set_feature_type;
static DataType s_last_get_data_type;

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
 * (inbound):
 *   RequestTypeGetData
 *     AppKeyDataType      - DataType
 *   RequestTypeSetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 *   RequestTypeGetFeature
 *     AppKeyFeatureType   - FeatureType
 *
 * (outbound):
 *   RequestTypeGetData
 *     AppKeyDataType      - DataType
 *     AppKeyDataValue        - DataValue
 *   RequestTypeSetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 *   RequestTypeGetFeature
 *     AppKeyFeatureType   - FeatureType
 *     AppKeyFeatureState  - FeatureState
 */
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  if(!in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: no callbacks");
    return;  // Nothing expected, ignore
  }

  // Get data response
  if(dict_find(iter, RequestTypeGetData)) {
    DataValue value;
    value.integer_value = 0;

    int type = dict_find(iter, AppKeyDataType)->value->int32;
    switch(type) {
      // Data type will be integer
      case DataTypeBatteryPercent:
      case DataTypeGSMStrength:
      case DataTypeStorageFreePercent:
      case DataTypeStorageFreeGBMajor:
      case DataTypeStorageFreeGBMinor:
        value.integer_value = dict_find(iter, AppKeyDataValue)->value->int32;
        break;

      // Data type will be string
      case DataTypeWifiNetworkName:
      case DataTypeGSMOperatorName:
        strcpy(value.string_value, dict_find(iter, AppKeyDataValue)->value->cstring);
        break;
    }

    s_last_get_data_cb(type, value, true);
  }

  // Set feature response
  else if(dict_find(iter, RequestTypeSetFeature)) {
    int type = dict_find(iter, AppKeyFeatureType)->value->int32;
    int state = dict_find(iter, AppKeyFeatureState)->value->int32;
    s_last_set_feature_cb(type, state, true);
  }

  // Get feature response
  else if(dict_find(iter, RequestTypeGetFeature)) {
    int type = dict_find(iter, AppKeyFeatureType)->value->int32;
    int state = dict_find(iter, AppKeyFeatureState)->value->int32;
    s_last_get_feature_cb(type, state, true);
  } 

  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Unknown message type");
  }

  clear_callbacks();
}

/************************************ API *************************************/

static void get_data_handler(void *context) {
  DataValue fail_value;

  if(app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error sending outbox!");
    s_last_get_data_cb(s_last_get_data_type, fail_value, false);
  }
}

void dash_api_get_data(DataType type, DashAPIDataCallback *callback) {
  DataValue fail_value;
  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Bluetooth is disconnected!");
  }


  if(in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_get_data() failed - request already in progress!");
    callback(type, fail_value, false);
    return;
  }

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error opening outbox!");
    callback(type, fail_value, false);
    return;
  }

  const int dummy = 0;
  dict_write_int(iter, AppKeyUsesDashAPI, &dummy, sizeof(int), true);
  dict_write_int(iter, RequestTypeGetData, &dummy, sizeof(int), true);
  dict_write_int(iter, AppKeyDataType, &type, sizeof(int), true);

  s_last_get_data_type = type;
  s_last_get_data_cb = callback;
  app_timer_register(DELAY_MS, get_data_handler, NULL);
}

static void set_feature_handler(void *context) {
  if(app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error sending outbox!");
    s_last_set_feature_cb(s_last_set_feature_type, FeatureStateUnknown, false);
  }
}

void dash_api_set_feature(FeatureType type, FeatureState new_state, DashAPIFeatureCallback *callback) {
  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Bluetooth is disconnected!");
  }

  if(in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_set_feature() failed - request already in progress!");
    callback(type, FeatureStateUnknown, false);
    return;
  }

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error opening outbox!");
    callback(type, FeatureStateUnknown, false);
    return;
  }

  const int dummy = 0;
  dict_write_int(iter, AppKeyUsesDashAPI, &dummy, sizeof(int), true);
  dict_write_int(iter, RequestTypeSetFeature, &dummy, sizeof(int), true);
  dict_write_int(iter, AppKeyFeatureType, &type, sizeof(int), true);
  dict_write_int(iter, AppKeyFeatureState, &new_state, sizeof(int), true);

  s_last_set_feature_type = type;
  s_last_set_feature_cb = callback;
  app_timer_register(DELAY_MS, set_feature_handler, NULL);
}

static void get_feature_handler(void *context) {
  if(app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error sending outbox!");
    s_last_get_feature_cb(s_last_get_feature_type, FeatureStateUnknown, false);
  }
}

void dash_api_get_feature(FeatureType type, DashAPIFeatureCallback *callback) {
  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Bluetooth is disconnected!");
  }

  if(in_flight()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_get_feature() failed - request already in progress!");
    callback(type, FeatureStateUnknown, false);
    return;
  }

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error opening outbox!");
    callback(type, FeatureStateUnknown, false);
    return;
  }

  const int dummy = 0;
  dict_write_int(iter, AppKeyUsesDashAPI, &dummy, sizeof(int), true);
  dict_write_int(iter, RequestTypeGetFeature, &dummy, sizeof(int), true);
  dict_write_int(iter, AppKeyFeatureType, &type, sizeof(int), true);

  s_last_get_feature_type = type;
  s_last_get_feature_cb = callback;
  app_timer_register(DELAY_MS, get_feature_handler, NULL);
}

void dash_api_init_appmessage() {
  app_message_deregister_callbacks();
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
}
