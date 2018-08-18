//
// background.js
//

const

send_to_sm = "Send to SuperMemo",
cannot_send_to_sm = "Trying to connect to SuperMemo…",

enabled_icon_set = {
    16: "icons/sm-16.png",
    32: "icons/sm-32.png",
    48: "icons/sm-48.png",
    64: "icons/sm-64.png"
},
disabled_icon_set = {
    16: "icons/disabled/sm-16.png",
    32: "icons/disabled/sm-32.png",
    48: "icons/disabled/sm-48.png",
    64: "icons/disabled/sm-64.png"
};

/*
    Disable button at startup.
*/
browser.browserAction.disable();

let

update_active_tab = (connected) => {
    if (connected) {
        browser.browserAction.enable();
        browser.browserAction.setIcon({path: enabled_icon_set});
        browser.browserAction.setTitle({title: send_to_sm});
    } else {
        browser.browserAction.disable();
        browser.browserAction.setIcon({path: disabled_icon_set});
        browser.browserAction.setTitle({title: cannot_send_to_sm});
    }
},

message_handler = (response) => {
    console.log("sm-pipes < " + JSON.stringify(response));

    if (!response.hasOwnProperty("ntrnl") || typeof response.ntrnl !== "string") return;

    switch (response.ntrnl) {
        case "connected":
        update_active_tab(true);
        break;

        case "disconnected":
        update_active_tab(false);
        break;
    }
},

disconnect_handler = () => {
    console.log("sm-pipes : disconnected");
},

connect_native = function (name, on_message, on_disconnect) {
    let port = browser.runtime.connectNative(name);
    port.onMessage.addListener(on_message);
    port.onDisconnect.addListener(on_disconnect);
    return port;
},

port = connect_native(
    "com.supermemory.nativemsgproxy",
    message_handler,
    disconnect_handler
),

send_msg = (cmd, val) => {
    let msg = {"cmd": cmd, "val": val};
    console.log("sm-pipes > " + JSON.stringify(msg));
    port.postMessage(msg);
};

/*
    On a click on the browser send URL of current tab
*/
browser.browserAction.onClicked.addListener((tab) => {
    send_msg("url", tab.url);
});
