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
  BaseViewModel.call(this, {
    "ssid": "",
    "pass": "",
    "emoncms_server": "emoncms.org",
    "emoncms_path": "/emoncms",
    "emoncms_apikey": "",
    "emoncms_node": "",
    "emoncms_fingerprint": "",
    "mqtt_server": "",
    "mqtt_topic": "",
    "mqtt_feed_prefix": "",
    "mqtt_user": "",
    "mqtt_pass": "",
    "www_username": "",
    "www_password": "",
	"voltage_cal": "",
	"voltage2_cal": "",
	"ct1_cal": "",
	"ct2_cal": "",
	"ct3_cal": "",
	"ct4_cal": "",
	"ct5_cal": "",
	"ct6_cal": "",
	"freq_cal": "",
	"gain_cal": "", 
    "espflash": "",
    "version": "0.0.0"
  }, baseEndpoint + '/config');
}
ConfigViewModel.prototype = Object.create(BaseViewModel.prototype);
ConfigViewModel.prototype.constructor = ConfigViewModel;

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
          self.initialised(true);
          updateTimer = setTimeout(self.update, updateTime);
          self.upgradeUrl(baseEndpoint + '/update');
          self.updating(false);
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
	var cal = {
		voltage: self.config.voltage_cal(), 
		voltage2: self.config.voltage2_cal(), 
		ct1: self.config.ct1_cal(), 
		ct2: self.config.ct2_cal(),
		ct3: self.config.ct3_cal(), 
		ct4: self.config.ct4_cal(),
		ct5: self.config.ct5_cal(), 
		ct6: self.config.ct6_cal(),
		freq: self.config.freq_cal(), 
		gain: self.config.gain_cal()
    };
	if (isNaN(cal.voltage) || isNaN(cal.ct1) || isNaN(cal.ct2) || isNaN(cal.freq) || isNaN(cal.gain)) {
		alert("Please enter a number for calibration values");
	} else if (cal.voltage > 65535 || cal.ct1 > 65535 || cal.ct2 > 65535) {
		alert("Please enter calibration settings less than 65535");
    } else {
		self.saveCalFetching(true);
		self.saveCalSuccess(false);
	  $.post(baseEndpoint + "/savecal", cal, function (data) {
		  self.saveCalSuccess(true);
		}).fail(function () {
		  alert("Failed to save calibration config");
		}).always(function () {
		  self.saveCalFetching(false);
		});
	}
  };
}

$(function () {
  // Activates knockout.js
  var emonesp = new EmonEspViewModel();
  ko.applyBindings(emonesp);
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
