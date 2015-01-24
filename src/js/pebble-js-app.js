var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var xhrPost = function (url, data) {
  var xhr = new XMLHttpRequest();
  xhr.open("POST", url);
  xhr.send(data);
};


function pullTime() {
  // Construct URL
  var url = "http://localhost:1337/";
  console.log("url is" + url);
  // Send request to website
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with time info
      var json = JSON.parse(responseText);

      
      var ticks = json.main.ticks; 
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TICKS": ticks,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
      
    }      
  );
   
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial time
    pullTime();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    pullTime();
  }    
);
