//
// content-script.js
//

let
    port = do {
        if (BROWSER == "Mozilla")
            browser.runtime.connect({ name: "supermemo-content-script" })
        else if (BROWSER == "Chrome")
            chrome.runtime.connect({ name: "supermemo-content-script" })
    },

    on_message = (m) => {
        console.log("sm-content-script < " + JSON.stringify(m));
        switch (m.cmd) {
            case "page": {
                let msg = {
                    "cmd": m.cmd,
                    "val": {
                        "content_type": document.contentType,
                        "url": document.URL,
                        "title": document.title,
                        "page": document.documentElement.outerHTML
                    }
                };
                port.postMessage(msg);
                break;
            }

            case "selection": {
                let
                    selection = window.getSelection(),
                    range = selection.getRangeAt(0);
                if (!range) break;
                let div = document.createElement('div');
                div.appendChild(range.cloneContents());
                let msg = {
                    "cmd": m.cmd,
                    "val": {
                        "url": document.URL,
                        "title": document.title,
                        "selection": div.innerHTML,
                        "selection_text": m.selection_text
                    }
                };
                port.postMessage(msg);
                break;
            }
        }
    };

port.onMessage.addListener(on_message);
port.postMessage({greeting: true});
