// Work out the endpoint to use, for dev you can change to point at a remote ESP
// and run the HTML/JS from file, no need to upload to the ESP to test

var baseHost = window.location.hostname;
//var baseHost = 'emonesp.local';
//var baseHost = '192.168.4.1';
//var baseHost = '172.16.0.52';
var baseEndpoint = 'http://' + baseHost;

var statusupdate = false;
var selected_network_ssid = "";
var lastmode = "";
var ipaddress = "";

// Convert string to number, divide by scale, return result
// as a string with specified precision
function scaleString(string, scale, precision) {
  var tmpval = parseInt(string) / scale;
  return tmpval.toFixed(precision);
}

function isNumber(val) {
     return (val != undefined && val != null && val.toString().length > 0 && val.toString().match(/[^0-9\.\-]/g) == null);
}

function BaseViewModel(defaults, remoteUrl, mappings) {
  if(mappings === undefined){
   mappings = {};
  }
  var self = this;
  self.remoteUrl = remoteUrl;

  // Observable properties
  ko.mapping.fromJS(defaults, mappings, self);
  self.fetching = ko.observable(false);
}

BaseViewModel.prototype.update = function (after) {
  if(after === undefined){
   after = function () { };
  }
  var self = this;
  self.fetching(true);
  $.get(self.remoteUrl, function (data) {
    ko.mapping.fromJS(data, self);
  }, 'json').always(function () {
    self.fetching(false);
    after();
  });
};


function StatusViewModel() {
  var self = this;

  BaseViewModel.call(self, {
    "mode": "ERR",
    "networks": [],
    "rssi": [],
    "srssi": "",
    "ipaddress": "",
    "packets_sent": "",
    "packets_success": "",
    "emoncms_connected": "",
    "mqtt_connected": "",
    "free_heap": ""
  }, baseEndpoint + '/status');

  // Some devired values
  self.isWifiClient = ko.pureComputed(function () {
    return ("STA" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.isWifiAccessPoint = ko.pureComputed(function () {
    return ("AP" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.fullMode = ko.pureComputed(function () {
    switch (self.mode()) {
      case "AP":
        return "Access Point (AP)";
      case "STA":
        return "Client (STA)";
      case "STA+AP":
        return "Client + Access Point (STA+AP)";
    }

    return "Unknown (" + self.mode() + ")";
  });
}
StatusViewModel.prototype = Object.create(BaseViewModel.prototype);
StatusViewModel.prototype.constructor = StatusViewModel;

function ConfigViewModel() {
  var self = this;
  BaseViewModel.call(self, {
    "espflash":"",
    "version":"",
    "ssid":"",
    "pass":"",
    "emoncms_server":"emoncms.org",
    "emoncms_path":"/emoncms",
    "emoncms_node":"",
    "emoncms_apikey":"",
    "emoncms_fingerprint":"",
    "mqtt_server":"",
    "mqtt_topic":"",
    "mqtt_feed_prefix":"",
    "mqtt_user":"",
    "mqtt_pass":"",
    "www_username":"",
    "www_password":"",
    "voltage_cal":"",
    "voltage2_cal":"",
    "gain1_cal":"1","gain2_cal":"1","gain3_cal":"1","gain4_cal":"1","gain5_cal":"1","gain6_cal":"1",
    "gain7_cal":"1","gain8_cal":"1","gain9_cal":"1","gain10_cal":"1","gain11_cal":"1","gain12_cal":"1",
    "gain13_cal":"1","gain14_cal":"1","gain15_cal":"1","gain16_cal":"1","gain17_cal":"1","gain18_cal":"1",
    "gain19_cal":"1","gain20_cal":"1","gain21_cal":"1","gain22_cal":"1","gain23_cal":"1","gain24_cal":"1",
    "gain25_cal":"1","gain26_cal":"1","gain27_cal":"1","gain28_cal":"1","gain29_cal":"1","gain30_cal":"1",
    "gain31_cal":"1","gain32_cal":"1","gain33_cal":"1","gain34_cal":"1","gain35_cal":"1","gain36_cal":"1",
    "gain37_cal":"1","gain38_cal":"1","gain39_cal":"1","gain40_cal":"1","gain41_cal":"1","gain42_cal":"1",
    "ct1_cal":"","cur1_mul":"1.00","pow1_mul":"1.00","ct2_cal":"","cur2_mul":"1.00","pow2_mul":"1.00",
    "ct3_cal":"","cur3_mul":"1.00","pow3_mul":"1.00","ct4_cal":"","cur4_mul":"1.00","pow4_mul":"1.00",
    "ct5_cal":"","cur5_mul":"1.00","pow5_mul":"1.00","ct6_cal":"","cur6_mul":"1.00","pow6_mul":"1.00",
    "ct7_cal":"","cur7_mul":"1.00","pow7_mul":"1.00","ct8_cal":"","cur8_mul":"1.00","pow8_mul":"1.00",
    "ct9_cal":"","cur9_mul":"1.00","pow9_mul":"1.00","ct10_cal":"","cur10_mul":"1.00","pow10_mul":"1.00",
    "ct11_cal":"","cur11_mul":"1.00","pow11_mul":"1.00","ct12_cal":"","cur12_mul":"1.00","pow12_mul":"1.00",
    "ct13_cal":"","cur13_mul":"1.00","pow13_mul":"1.00","ct14_cal":"","cur14_mul":"1.00","pow14_mul":"1.00",
    "ct15_cal":"","cur15_mul":"1.00","pow15_mul":"1.00","ct16_cal":"","cur16_mul":"1.00","pow16_mul":"1.00",
    "ct17_cal":"","cur17_mul":"1.00","pow17_mul":"1.00","ct18_cal":"","cur18_mul":"1.00","pow18_mul":"1.00",
    "ct19_cal":"","cur19_mul":"1.00","pow19_mul":"1.00","ct20_cal":"","cur20_mul":"1.00","pow20_mul":"1.00",
    "ct21_cal":"","cur21_mul":"1.00","pow21_mul":"1.00","ct22_cal":"","cur22_mul":"1.00","pow22_mul":"1.00",
    "ct23_cal":"","cur23_mul":"1.00","pow23_mul":"1.00","ct24_cal":"","cur24_mul":"1.00","pow24_mul":"1.00",
    "ct25_cal":"","cur25_mul":"1.00","pow25_mul":"1.00","ct26_cal":"","cur26_mul":"1.00","pow26_mul":"1.00",
    "ct27_cal":"","cur27_mul":"1.00","pow27_mul":"1.00","ct28_cal":"","cur28_mul":"1.00","pow28_mul":"1.00",
    "ct29_cal":"","cur29_mul":"1.00","pow29_mul":"1.00","ct30_cal":"","cur30_mul":"1.00","pow30_mul":"1.00",
    "ct31_cal":"","cur31_mul":"1.00","pow31_mul":"1.00","ct32_cal":"","cur32_mul":"1.00","pow32_mul":"1.00",
    "ct33_cal":"","cur33_mul":"1.00","pow33_mul":"1.00","ct34_cal":"","cur34_mul":"1.00","pow34_mul":"1.00",
    "ct35_cal":"","cur35_mul":"1.00","pow35_mul":"1.00","ct36_cal":"","cur36_mul":"1.00","pow36_mul":"1.00",
    "ct37_cal":"","cur37_mul":"1.00","pow37_mul":"1.00","ct38_cal":"","cur38_mul":"1.00","pow38_mul":"1.00",
    "ct39_cal":"","cur39_mul":"1.00","pow39_mul":"1.00","ct40_cal":"","cur40_mul":"1.00","pow40_mul":"1.00",
    "ct41_cal":"","cur41_mul":"1.00","pow41_mul":"1.00","ct42_cal":"","cur42_mul":"1.00","pow42_mul":"1.00",
    "freq_cal":""
    }, baseEndpoint + '/config');
}
ConfigViewModel.prototype = Object.create(BaseViewModel.prototype);
ConfigViewModel.prototype.constructor = ConfigViewModel;

function SensorConfigViewModel(baseconfig, baselast) {
  var self = this;
  self.config = baseconfig;
  self.last = baselast;

  self.getValue = function(prefix, postfix) {
    var lastValue = null;
    var value = "";
    for (i = 0; i < self.selectedSensors().length; i++) {
      obs = self.config[`${prefix}${self.selectedSensors()[i].substr(2)}_${postfix}`];
      if (obs == null) {
        return "";
      }
      value = obs();
      if (lastValue != null && lastValue != value) {
        return "";
      }
      lastValue = value;
    }
    return value;
  };
    self.setValue = function(prefix, postfix, value) {
      self.selectedSensors().forEach(function(sensor,_,_) {
        self.config[`${prefix}${sensor.substr(2)}_${postfix}`] = ko.observable(value);
      });
    };
    self.selectedCt = ko.pureComputed({
      read: function() { return self.getValue("ct", "cal") },
      write: function(value) { self.setValue("ct", "cal", value) }
    }).extend({ notify: 'always' });
    self.selectedGain = ko.pureComputed({
      read: function() { return self.getValue("gain", "cal") },
      write: function(value) { self.setValue("gain", "cal", value) }
    }).extend({ notify: 'always' });
    self.selectedCurMul = ko.pureComputed({
      read: function() { return self.getValue("cur", "mul") },
      write: function(value) { self.setValue("cur", "mul", value) }
    }).extend({ notify: 'always' });
    self.selectedPowMul = ko.pureComputed({
      read: function() { return self.getValue("pow", "mul") },
      write: function(value) { return self.setValue("pow", "mul", value) }
    }).extend({ notify: 'always' });

    self.selectedSensors = ko.observableArray(["CT1"]);
    self.sensors = ko.observableArray();

    self.update = function(after) {
      var vals = [];
      self.last.values().forEach(function(input,_,_) {
        if (input.key().substring(0, 2) == "CT") {
          vals.push(input.key());
        }
      });
      self.sensors = ko.observableArray(vals);
      after();
   }
}

function LastValuesViewModel() {
  var self = this;
  self.remoteUrl = baseEndpoint + '/lastvalues';

  // Observable properties
  self.fetching = ko.observable(false);
  self.values = ko.mapping.fromJS([]);

  self.update = function (after) {
    if(after === undefined){
     after = function () { };
    }
    self.fetching(true);
    $.get(self.remoteUrl, function (data) {
      // Transform the data into something a bit easier to handle as a binding
      var namevaluepairs = data.split(",");
      var vals = [];
      for (var z in namevaluepairs) {
        var namevalue = namevaluepairs[z].split(":");
        var units = "";
        if (namevalue[0].indexOf("CT") === 0) units = "A";
		if (namevalue[0].indexOf("totI") === 0) units = "A";
		if (namevalue[0].indexOf("V") === 0) units = "V";
		if (namevalue[0].indexOf("W") === 0) units = "W";
		if (namevalue[0].indexOf("AW") === 0) units = "W";
        if (namevalue[0].indexOf("T") === 0) units = String.fromCharCode(176)+"C";
        vals.push({key: namevalue[0], value: namevalue[1]+units});
      }
      ko.mapping.fromJS(vals, self.values);
    }, 'text').always(function () {
      self.fetching(false);
      after();
    });
  };
}

function EmonEspViewModel() {
  var self = this;

  self.config = new ConfigViewModel();
  self.status = new StatusViewModel();
  self.last = new LastValuesViewModel();
  self.sensorconfig = new SensorConfigViewModel(self.config, self.last);

  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);

  var updateTimer = null;
  var updateTime = 1 * 1000;

  // Upgrade URL
  self.upgradeUrl = ko.observable('about:blank');

  // -----------------------------------------------------------------------
  // Initialise the app
  // -----------------------------------------------------------------------
  self.start = function () {
    self.updating(true);
    self.config.update(function () {
      self.status.update(function () {
        self.last.update(function () {
          self.sensorconfig.update(function () {
            ko.applyBindings(self);
            self.initialised(true);
            updateTimer = setTimeout(self.update, updateTime);
            self.upgradeUrl(baseEndpoint + '/update');
            self.updating(false);
          });
        });
      });
    });
  };

  // -----------------------------------------------------------------------
  // Get the updated state from the ESP
  // -----------------------------------------------------------------------
  self.update = function () {
    if (self.updating()) {
      return;
    }
    self.updating(true);
    if (null !== updateTimer) {
      clearTimeout(updateTimer);
      updateTimer = null;
    }
    self.status.update(function () {
      self.last.update(function () {
        updateTimer = setTimeout(self.update, updateTime);
        self.updating(false);
      });
    });
  };

  self.wifiConnecting = ko.observable(false);
  self.status.mode.subscribe(function (newValue) {
    if(newValue === "STA+AP" || newValue === "STA") {
      self.wifiConnecting(false);
    }
  });

  // -----------------------------------------------------------------------
  // Event: WiFi Connect
  // -----------------------------------------------------------------------
  self.saveNetworkFetching = ko.observable(false);
  self.saveNetworkSuccess = ko.observable(false);
  self.saveNetwork = function () {
    if (self.config.ssid() === "") {
      alert("Please select network");
    } else {
      self.saveNetworkFetching(true);
      self.saveNetworkSuccess(false);
      $.post(baseEndpoint + "/savenetwork", { ssid: self.config.ssid(), pass: self.config.pass() }, function (data) {
          self.saveNetworkSuccess(true);
          self.wifiConnecting(true);
        }).fail(function () {
          alert("Failed to save WiFi config");
        }).always(function () {
          self.saveNetworkFetching(false);
        });
    }
  };

  // -----------------------------------------------------------------------
  // Event: Admin save
  // -----------------------------------------------------------------------
  self.saveAdminFetching = ko.observable(false);
  self.saveAdminSuccess = ko.observable(false);
  self.saveAdmin = function () {
	var adminsave = {
		user: self.config.www_username(),
		pass: self.config.www_password()
	};
	
	if (adminsave.user.length > 16 || adminsave.pass.length > 16) {
		alert("Please enter a username and password that is 16 characters or less");
	} else {
    self.saveAdminFetching(true);
    self.saveAdminSuccess(false);
    $.post(baseEndpoint + "/saveadmin", adminsave, function (data) {
      self.saveAdminSuccess(true);
    }).fail(function () {
      alert("Failed to save Admin config");
    }).always(function () {
      self.saveAdminFetching(false);
    });
   }
  };

  // -----------------------------------------------------------------------
  // Event: Emoncms save
  // -----------------------------------------------------------------------
  self.saveEmonCmsFetching = ko.observable(false);
  self.saveEmonCmsSuccess = ko.observable(false);
  self.saveEmonCms = function () {
    var emoncms = {
      server: self.config.emoncms_server(),
      path: self.config.emoncms_path(),
      apikey: self.config.emoncms_apikey(),
      node: self.config.emoncms_node(),
      fingerprint: self.config.emoncms_fingerprint()
    };

    if (emoncms.server === "" || emoncms.node === "") {
      alert("Please enter EmonCMS server and node");
    } else if (emoncms.apikey.length != 32) {
      alert("Please enter a valid Emoncms apikey");
    } else if (emoncms.fingerprint !== "" && emoncms.fingerprint.length != 59) {
      alert("Please enter a valid SSL SHA-1 fingerprint");
    } else {
      self.saveEmonCmsFetching(true);
      self.saveEmonCmsSuccess(false);
      $.post(baseEndpoint + "/saveemoncms", emoncms, function (data) {
        self.saveEmonCmsSuccess(true);
      }).fail(function () {
        alert("Failed to save EmonCMS config");
      }).always(function () {
        self.saveEmonCmsFetching(false);
      });
    }
  };

  // -----------------------------------------------------------------------
  // Event: MQTT save
  // -----------------------------------------------------------------------
  self.saveMqttFetching = ko.observable(false);
  self.saveMqttSuccess = ko.observable(false);
  self.saveMqtt = function () {
    var mqtt = {
      server: self.config.mqtt_server(),
      topic: self.config.mqtt_topic(),
      prefix: self.config.mqtt_feed_prefix(),
      user: self.config.mqtt_user(),
      pass: self.config.mqtt_pass()
    };

	  self.saveMqttFetching(true);
	  self.saveMqttSuccess(false);
	  $.post(baseEndpoint + "/savemqtt", mqtt, function (data) {
		self.saveMqttSuccess(true);
	  }).fail(function () {
		alert("Failed to save MQTT config");
	  }).always(function () {
		self.saveMqttFetching(false);
	  });
  };
  // -----------------------------------------------------------------------
  // Event: Calibration save
  // -----------------------------------------------------------------------
  self.saveCalFetching = ko.observable(false);
  self.saveCalSuccess = ko.observable(false);
  self.saveCal = function () {
    for (i = 1; i <= 6*7; i ++) {
      if (!isNumber(self.config[`ct${i}_cal`]()) || !isNumber(self.config[`cur${i}_mul`]()) || !isNumber(self.config[`pow${i}_mul`]())) {
        alert("Please enter a number for calibration values");
        return;
      } else if (self.config[`ct${i}_cal`] > 65535) {
        alert("Please enter calibration settings less than 65535");
        return;
      }
      if (!isNumber(self.config[`gain${i}_cal`]()) || (self.config[`gain${i}_cal`]() != 1 && self.config[`gain${i}_cal`]() != 2 && self.config[`gain${i}_cal`]() != 4)) {
        alert("Please enter gain settings of 1, 2, or 4");
        return;
      }
    }
    if (!isNumber(self.config.voltage_cal()) || !isNumber(self.config.voltage2_cal()) || !isNumber(self.config.freq_cal())) {
      alert("Please enter a number for calibration values");
      return;
    }
    if (self.config.voltage_cal() > 65535 || self.config.voltage2_cal() > 65535 || self.config.freq_cal() > 65535) {
      alert("Please enter calibration settings less than 65535");
      return;
    }
    self.saveCalFetching(true);
    self.saveCalSuccess(false);
    $.post(baseEndpoint + "/savecal", self.config, function (data) {
      self.saveCalSuccess(true);
    }).fail(function () {
      alert("Failed to save calibration config");
    }).always(function () {
      self.saveCalFetching(false);
    });
  };
}

$(function () {
  // Activates knockout.js
  var emonesp = new EmonEspViewModel();
  emonesp.start();
});

// -----------------------------------------------------------------------
// Event: Turn off Access Point
// -----------------------------------------------------------------------
document.getElementById("apoff").addEventListener("click", function (e) {

  var r = new XMLHttpRequest();
  r.open("POST", "apoff", true);
  r.onreadystatechange = function () {
    if (r.readyState != 4 || r.status != 200)
      return;
    var str = r.responseText;
    console.log(str);
    document.getElementById("apoff").style.display = 'none';
    if (ipaddress !== "")
      window.location = "http://" + ipaddress;
  };
  r.send();
});

// -----------------------------------------------------------------------
// Event: Reset config and reboot
// -----------------------------------------------------------------------
document.getElementById("reset").addEventListener("click", function (e) {

  if (confirm("CAUTION: Do you really want to Factory Reset? All setting and config will be lost.")) {
    var r = new XMLHttpRequest();
    r.open("POST", "reset", true);
    r.onreadystatechange = function () {
      if (r.readyState != 4 || r.status != 200)
        return;
      var str = r.responseText;
      console.log(str);
      if (str !== 0)
        document.getElementById("reset").innerHTML = "Resetting...";
    };
    r.send();
  }
});

// -----------------------------------------------------------------------
// Event: Restart
// -----------------------------------------------------------------------
document.getElementById("restart").addEventListener("click", function (e) {

  if (confirm("Restart emonESP? Current config will be saved, takes approximately 10s.")) {
    var r = new XMLHttpRequest();
    r.open("POST", "restart", true);
    r.onreadystatechange = function () {
      if (r.readyState != 4 || r.status != 200)
        return;
      var str = r.responseText;
      console.log(str);
      if (str !== 0)
        document.getElementById("restart").innerHTML = "Restarting";
    };
    r.send();
  }
});

// -----------------------------------------------------------------------
// Event:Upload Firmware
// -----------------------------------------------------------------------
document.getElementById("upload").addEventListener("click", function(e) {
  window.location.href='/upload';
});
