//
// Mozilla
//
// background.js
//

const

    send_to_sm = "Send to SuperMemo",
    cannot_send_to_sm = "Connecting to SuperMemo…",
    permanently_disconnected_from_sm = "Permanently disconnected from SuperMemo",

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
    },

    menu_items = [
        {
            id: "send-image-to-supermemo",
            title: "Send image to SuperMemo",
            contexts: ["image"]
        },
        {
            id: "send-link-to-supermemo",
            title: "Send link to SuperMemo",
            contexts: ["link"]
        },
        {
            id: "send-page-to-supermemo",
            title: "Send page to SuperMemo",
            contexts: ["page"]
        },
        {
            id: "send-selection-to-supermemo",
            title: "Send selection to SuperMemo",
            contexts: ["selection"]
        }
    ];

/*
    Disable button at startup.
*/
browser.browserAction.disable();

let

    csPort = [],

    update_active_tab = (connected, title) => {
        if (connected) {
            browser.browserAction.enable();
            browser.browserAction.setIcon({ path: enabled_icon_set });
            browser.browserAction.setTitle({ "title": title ? title : send_to_sm });
            menu_items.forEach((e) => browser.contextMenus.create(e));
        } else {
            browser.contextMenus.removeAll().then(() => {
                browser.browserAction.disable();
                browser.browserAction.setIcon({ path: disabled_icon_set });
                browser.browserAction.setTitle({ "title": title ? title : cannot_send_to_sm });
            });
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

        update_active_tab(false, permanently_disconnected_from_sm)
    },

    connect_native = (name, on_message, on_disconnect) => {
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
        let msg = { "cmd": cmd, "val": val };
        console.log("sm-pipes > " + JSON.stringify(msg));
        port.postMessage(msg);
    },

    on_content_script_message = (msg, p) => {
        if (msg.hasOwnProperty("cmd") && msg.hasOwnProperty("val")) {
            // TODO: modify message
            console.log("sm-pipes > " + JSON.stringify(msg));
            port.postMessage(msg);
        } else {
            console.log("sm-pipes ! " + JSON.stringify(msg) + " TAB:" + p.sender.tab.id);
        }
    },

    cs_disconnected = (p) => {
        if (p.name !== "supermemo-content-script") return;

        console.log("sm-pipes : content-script disconnect TAB:" + p.sender.tab.id);
        p.onMessage.removeListener(on_content_script_message);
        p.onDisconnect.removeListener(cs_disconnected);
        csPort = csPort.filter((e, idx) => idx !== p.sender.tab.id);
    },

    cs_connected = (p) => {
        if (p.name !== "supermemo-content-script") return;

        console.log("sm-pipes : content-script connect TAB:" + p.sender.tab.id);
        csPort[p.sender.tab.id] = p;
        p.onMessage.addListener(on_content_script_message);
        p.onDisconnect.addListener(cs_disconnected);
    },

    download_binary = (url, image_loaded) => {
        let req = new XMLHttpRequest(),
            on_blob_encoded = (e) => {
                image_loaded(e.target.result);
            },
            on_blob_loaded = (e) => {
                let resp = e.target.response;
                if (resp) {
                    let file_reader = new FileReader();
                    file_reader.addEventListener("load", on_blob_encoded);
                    file_reader.readAsDataURL(resp);
                }
            };

        req.onload = on_blob_loaded;
        req.open("GET", url);
        req.responseType = "blob";
        req.mozBackgroundRequest = true;
        req.send();
    },

    on_button_clicked = (tab) => {
        send_msg("url", tab.url);
    },

    on_context_menu = (info, tab) => {
        switch (info.menuItemId) {
            case "send-image-to-supermemo":
                download_binary(info.srcUrl, (encoded_image) => {
                    let sval = {
                        "url": info.srcUrl,
                        "data_url": encoded_image,
                        "page_url": info.pageUrl
                    };
                    if (info.frameUrl) {
                        sval["frame_url"] = info.frameUrl;
                    }
                    if (tab.url) {
                        sval["tab_url"] = tab.url;
                    }
                    if (tab.title) {
                        sval["tab_title"] = tab.title;
                    }
                    send_msg("img", sval);
                });
                break;

            case "send-link-to-supermemo":
                let sval = {
                    "url": info.linkUrl,
                    "text": info.linkText,
                    "page_url": info.pageUrl
                };
                if (info.frameUrl) {
                    sval["frame_url"] = info.frameUrl;
                }
                if (tab.url) {
                    sval["tab_url"] = tab.url;
                }
                if (tab.title) {
                    sval["tab_title"] = tab.title;
                }
                send_msg("lnk", sval);
                break;

            case "send-page-to-supermemo":
                if (csPort[tab.id]) {
                    csPort[tab.id].postMessage({ cmd: "page" });
                }
                break;

            case "send-selection-to-supermemo":
                if (csPort[tab.id]) {
                    csPort[tab.id].postMessage({ cmd: "selection", selection_text: info.selectionText });
                }
                break;
        }
    };

browser.runtime.onConnect.addListener(cs_connected);
browser.browserAction.onClicked.addListener(on_button_clicked);
browser.contextMenus.onClicked.addListener(on_context_menu);
