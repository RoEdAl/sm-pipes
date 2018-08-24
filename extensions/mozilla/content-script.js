//
// Mozilla
//
// content-script.js
//

let
    port = browser.runtime.connect({ name: "supermemo-content-script" }),

    on_message = (m) => {
        console.log("sm-content-script < " + JSON.stringify(m));
        switch (m.cmd) {
            case "page":
                let msg = {
                    "cmd": m.cmd,
                    "val": {
                        "content_type": document.contentType,
                        "url": document.URL,
                        "title": document.title,
                        "doc": document.documentElement.outerHTML
                    }
                };
                port.postMessage(msg);
                break;
        }
    };

port.onMessage.addListener(on_message);
port.postMessage({ greeting: true });
