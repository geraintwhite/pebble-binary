void save_setting(DictionaryIterator *iter, int KEY) {
  Tuple *data = dict_find(iter, KEY);
  if (data) {
    persist_write_int(KEY, data->value->uint8);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saving value for %d, %d", KEY, data->value->uint8);
  }
}

bool load_setting(int KEY, bool VAL) {
  bool val = persist_exists(KEY) ? persist_read_int(KEY) : VAL;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading value for %d, %d", KEY, val);
  return val;
}
