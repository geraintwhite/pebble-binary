var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var url = 'http://api.openweathermap.org/data/2.5/weather' +
      '?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=11c8a2c1236b923c156aebf3e4419037';

  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      var conditions = json.weather[0].main;      
      
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  getWeather();
});

Pebble.addEventListener('appmessage', function() {
  console.log('AppMessage received!');
  getWeather();
});

Pebble.addEventListener('showConfiguration', function(e) {
  var url = 'http://pebble.geraintwhite.co.uk/?options=batteryPercentage+showDate+invertColours+bluetoothVibrate+hourlyVibrate+showWeather';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + JSON.stringify(config));

  var data = {
    'BATTERY_PERCENTAGE': config.batteryPercentage ? 1 : 0,
    'SHOW_DATE': config.showDate ? 1 : 0,
    'INVERT_COLOURS': config.invertColours ? 1 : 0,
    'BLUETOOTH_VIBRATE': config.bluetoothVibrate ? 1 : 0,
    'HOURLY_VIBRATE': config.hourlyVibrate ? 1 : 0,
    'SHOW_WEATHER': config.showWeather ? 1 : 0,
  };

  Pebble.sendAppMessage(data, function() {
    console.log('Sent config data to Pebble');
  }, function() {
    console.log('Failed to send config data!');
  });
});
