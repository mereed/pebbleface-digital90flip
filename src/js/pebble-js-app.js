var mConfig = {};

Pebble.addEventListener("ready", function(e) {
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {

  //console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("bluetoothvibe", parseInt(config.bluetoothvibe)); 
  localStorage.setItem("hourlyvibe", parseInt(config.hourlyvibe)); 
  //localStorage.setItem("hidebt", parseInt(config.hidebt)); 
  localStorage.setItem("flip", parseInt(config.flip));  
  localStorage.setItem("hideday", parseInt(config.hideday));
  localStorage.setItem("hidetimeformat", parseInt(config.hidetimeformat)); 
  
  loadLocalData();

}
function loadLocalData() {
  
	mConfig.bluetoothvibe = parseInt(localStorage.getItem("bluetoothvibe"));
	mConfig.hourlyvibe = parseInt(localStorage.getItem("hourlyvibe"));
	//mConfig.hidebt = parseInt(localStorage.getItem("hidebt"));
	mConfig.flip = parseInt(localStorage.getItem("flip"));
	mConfig.hideday = parseInt(localStorage.getItem("hideday"));
	mConfig.hidetimeformat = parseInt(localStorage.getItem("hidetimeformat"));
	mConfig.configureUrl = "http://www.themapman.com/pebblewatch/bd590-2.html";
	

	if(isNaN(mConfig.bluetoothvibe)) {
		mConfig.bluetoothvibe = 0;
	}
	if(isNaN(mConfig.hourlyvibe)) {
		mConfig.hourlyvibe = 0;
	}
//	if(isNaN(mConfig.hidebt)) {
//		mConfig.hidebt = 0;
//	}
	if(isNaN(mConfig.flip)) {
		mConfig.flip = 0;
	} 
	if(isNaN(mConfig.hideday)) {
		mConfig.hideday = 0;
	} 
    if(isNaN(mConfig.hidetimeformat)) {
		mConfig.hidetimeformat = 0;
	}

  //console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble() {
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "bluetoothvibe":parseInt(mConfig.bluetoothvibe), 
    "hourlyvibe":parseInt(mConfig.hourlyvibe),
    //"hidebt":parseInt(mConfig.hidebt), 
    "flip":parseInt(mConfig.flip),
    "hideday":parseInt(mConfig.hideday),
	"hidetimeformat":parseInt(mConfig.hidetimeformat),
  });    
}