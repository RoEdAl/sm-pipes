//
// background.js
//

let

message_handler = (response) => {
  console.log("sm-pipes: Received: " + JSON.stringify(response));
},

disconnect_handler = () => {
  console.log("sm-pipes: Disconnected");
},

connect_native = function (name, on_message, on_disconnect) {
    let port = browser.runtime.connectNative(name);
    port.onMessage.addListener(on_message);
    port.onDisconnect.addListener(on_disconnect);
    return port;
},

port = connect_native("com.supermemory.nativemsgproxy", message_handler, disconnect_handler),

send_msg = (cmd, val) => {
    let msg = {
        "cmd": cmd,
        "val": val
    };
    console.log("sm-pipes: Sending: " + JSON.stringify(msg));
    port.postMessage(msg);
};

/*
    On a click on the browser send URL of current tab
*/
browser.browserAction.onClicked.addListener(() => {
  browser.tabs.query({currentWindow: true, active: true}).then((tabs) => {
      send_msg("url", tabs[0].url);
  });
});
