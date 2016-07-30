#include "pebble-dash-api.h"

#include <pebble-events/pebble-events.h>

#define INBOX_SIZE  256
#define OUTBOX_SIZE 256
#define DELAY_MS    200   // Enable opening AppMessage and an API query in the same event loop
#define TIMEOUT_MS  10000 // 10s for the Android app to respond, or it is assumed MIA

typedef enum {
  RequestTypeGetData = 24784,
  RequestTypeSetFeature = 24785,
  RequestTypeGetFeature = 24786,
  RequestTypeError = 24787,
  RequestTypeIsAvailable = 24788
} RequestType;

typedef enum {
  AppKeyFeatureType = 47836,
  AppKeyFeatureState = 47837,
  AppKeyDataType = 47838,
  AppKeyDataValue = 47839,
  AppKeyUsesDashAPI = 47840,
  AppKeyAppName = 47841,
  AppKeyErrorCode = 47842,
  AppKeyLibraryVersion = 47843
} AppKey;

static DashAPIDataCallback *s_last_get_data_cb;
static DashAPIFeatureCallback *s_last_set_feature_cb, *s_last_get_feature_cb;
static DashAPIErrorCallback *s_error_callback;

static AppTimer *s_timeout_timer;
static FeatureType s_last_get_feature_type, s_last_set_feature_type;
static DataType s_last_get_data_type;
static DictionaryIterator *s_outbox;
static char s_app_name[32];
static bool s_in_flight, s_initialized;

/********************************* Internal ***********************************/

static bool data_type_is_valid(DataType type) {
  bool valid = type >= DataTypeBatteryPercent && type <= DataTypeNextCalendarEventTwoLine;
  if(!valid) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: DataType is not valid: %d", type);
  }
  return valid;
}

static bool feature_type_is_valid(FeatureType type) {
  bool valid = type >= FeatureTypeWifi && type <= FeatureTypeAutoBrightness;
  if(!valid) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: FeatureType is not valid: %d", type);
  }
  return valid;
}

static bool feature_state_is_valid(FeatureState state) {
  bool valid = state >= FeatureStateOff && state <= FeatureStateRingerSilent;
  if(!valid) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: FeatureState is not valid: %d" + state);
  }
  return valid;
}

static void cancel_timeout() {
  if(s_timeout_timer) {
    app_timer_cancel(s_timeout_timer);
    s_timeout_timer = NULL;
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
 *   RequestTypeError
 *     AppKeyLibraryVersion
 *
 * (To Android):
 * HEADER:
 *   AppKeyUsesDashAPI
 *   AppKeyAppName
 *   AppKeyLibraryVersion
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
 *   RequestTypeError
 *     AppKeyErrorCode    - ErrorCodeNoPermissions | ErrorCodeWrongVersion
 */
static void inbox_received_handler(DictionaryIterator *inbox, void *context) {
  cancel_timeout(); // Response was quick enough
  s_in_flight = false;

  // Get data response
  if(dict_find(inbox, RequestTypeGetData)) {
    DataValue value;

    int type = dict_find(inbox, AppKeyDataType)->value->int32;
    switch(type) {
      // Data type will be integer
      case DataTypeBatteryPercent:
      case DataTypeGSMStrength:
      case DataTypeStoragePercentUsed:
      case DataTypeUnreadSMSCount:
        value.integer_value = dict_find(inbox, AppKeyDataValue)->value->int32;
        if(s_last_get_data_cb) {
          s_last_get_data_cb(type, value);
        }
        break;

      // Data type will be string
      case DataTypeWifiNetworkName:
      case DataTypeGSMOperatorName:
      case DataTypeStorageFreeGBString:
      case DataTypeNextCalendarEventOneLine:
      case DataTypeNextCalendarEventTwoLine:
        value.string_value = malloc(INBOX_SIZE);
        strcpy(value.string_value, dict_find(inbox, AppKeyDataValue)->value->cstring);
        if(s_last_get_data_cb) {
          s_last_get_data_cb(type, value);
        }
        free(value.string_value);
        break;

      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Unknown DataType! %d", type);
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
  else if(dict_find(inbox, RequestTypeError)) {
    int code = dict_find(inbox, AppKeyErrorCode)->value->int32;
    switch(code) {
      case ErrorCodeNoPermissions:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Permission for this app has not been granted within the Dash API Android app!");
        break;
      case ErrorCodeWrongVersion:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: An incompatible version of the Dash API Android app is installed!");
        break;
    }
    s_error_callback(code);
  }

  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Unknown message type");
  }
}

static void write_header() {
  const int dummy = 0;
  dict_write_int(s_outbox, AppKeyUsesDashAPI, &dummy, sizeof(int), true);
  dict_write_cstring(s_outbox, AppKeyAppName, s_app_name);
  const char *version = DASH_API_VERSION;
  dict_write_cstring(s_outbox, AppKeyLibraryVersion, version);
}

static bool prepare_outbox() {
  if(!s_initialized) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_init() not yet called.");
    s_error_callback(ErrorCodeSendingFailed);
    return false;
  }

  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Bluetooth is disconnected!");
    s_error_callback(ErrorCodeSendingFailed);
    return false;
  }

  if(s_in_flight) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_get_data() failed - request already in progress!");
    s_error_callback(ErrorCodeSendingFailed);
    return false;
  }

  AppMessageResult result = app_message_outbox_begin(&s_outbox);
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error opening outbox!");
    s_error_callback(ErrorCodeSendingFailed);
    return false;
  }

  write_header();

  return true;
}

static void timeout_handler(void *context) {
  s_timeout_timer = NULL;

  APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Timed out!");
  s_error_callback(ErrorCodeUnavailable);
}

static void send_outbox_callback() {
  if(app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: Error sending outbox!");
    s_error_callback(ErrorCodeSendingFailed);
  }

  // Begin timeout timer
  cancel_timeout();
  s_timeout_timer = app_timer_register(TIMEOUT_MS, timeout_handler, NULL);

  s_in_flight = true;
}

static void send_outbox() {
  app_timer_register(DELAY_MS, send_outbox_callback, NULL);
}

/************************************ API *************************************/

void dash_api_get_data(DataType type, DashAPIDataCallback *callback) {
  if(!prepare_outbox()) {
    return;
  }

  if(!data_type_is_valid(type)) {
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

  if(!feature_type_is_valid(type)) {
    return;
  }

  if(!feature_state_is_valid(new_state)) {
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

  if(!feature_type_is_valid(type)) {
    return;
  }

  const int dummy = 0;
  dict_write_int(s_outbox, RequestTypeGetFeature, &dummy, sizeof(int), true);
  dict_write_int(s_outbox, AppKeyFeatureType, &type, sizeof(int), true);

  s_last_get_feature_type = type;
  s_last_get_feature_cb = callback;
  send_outbox();
}

void dash_api_init(char *app_name, DashAPIErrorCallback *callback) {
  s_error_callback = callback;
  snprintf(s_app_name, sizeof(s_app_name), "%s", app_name);

  events_app_message_register_inbox_received(inbox_received_handler, NULL);
  events_app_message_request_inbox_size(INBOX_SIZE);
  events_app_message_request_outbox_size(OUTBOX_SIZE);

  s_initialized = true;
}

void dash_api_check_is_available() {
  if(!prepare_outbox()) {
    return;
  }

  const int dummy = 0;
  dict_write_int(s_outbox, RequestTypeIsAvailable, &dummy, sizeof(int), true);

  send_outbox();
}

char* dash_api_error_code_to_string(ErrorCode code) {
  switch(code) {
    case ErrorCodeSuccess:       return "The request was successful.";
    case ErrorCodeSendingFailed: return "The request failed to send.";
    case ErrorCodeUnavailable:   return "The Dash API Android app is not installed, or the request timed out.";
    case ErrorCodeNoPermissions: return "This app does not have write permission turned on in the Dash API Android app.";
    case ErrorCodeWrongVersion:  return "An incompatible version of the Dash API Android app is installed.";
    default: {
      static char s_err_buff[32];
      snprintf(s_err_buff, sizeof(s_err_buff), "Unknown error (code %d)", code);
      return &s_err_buff[0];
    } break;
  }
}

void dash_api_fake_get_data_response(DataType type, int integer_value, char *string_value) {
  if(!s_initialized) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_init() not yet called.");
  }

  if(!s_in_flight) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: There was no request in flight.");
    return;
  }

  cancel_timeout();

  DataValue value;
  value.integer_value = integer_value;
  value.string_value = malloc(INBOX_SIZE);
  if(string_value) {
    strcpy(value.string_value, string_value);
  }
  if(s_last_get_data_cb) {
    s_last_get_data_cb(type, value);
  }
  free(value.string_value);
}

void dash_api_fake_set_feature_response(FeatureType type, FeatureState new_state) {
  if(!s_initialized) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_init() not yet called.");
  }

  if(!s_in_flight) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: There was no request in flight.");
    return;
  }

  cancel_timeout();

  if(s_last_set_feature_cb) {
    s_last_set_feature_cb(type, new_state);
  }
}

void dash_api_fake_get_feature_response(FeatureType type, FeatureState new_state) {
  if(!s_initialized) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_init() not yet called.");
  }

  if(!s_in_flight) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: There was no request in flight.");
    return;
  }

  cancel_timeout();

  if(s_last_get_feature_cb) {
    s_last_get_feature_cb(type, new_state);
  }
}

void dash_api_fake_error(ErrorCode code) {
  if(!s_initialized) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: dash_api_init() not yet called.");
  }

  if(!s_in_flight) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dash API: There was no request in flight.");
    return;
  }

  cancel_timeout();

  if(s_error_callback) {
    s_error_callback(code);
  }
}
