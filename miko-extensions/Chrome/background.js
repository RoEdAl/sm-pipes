const prefix = "Send to SuperMemo: ";

let isSrvListening = false,
	port = null;

function log( message ) {
	console.log( message );
}

function onNativeMessage( message ) {
	if ( message.hasOwnProperty( 'ntrnl' ) ) {
		if ( message.ntrnl === 'connected' ) {
			isSrvListening = true;
			enableBrowserActionForAllTabs();
		} else {
			isSrvListening = false;
			disableBrowserActionForAllTabs();
		}
	}
	
	log( message );
}

function onDisconnected() {
	log( { event: 'disconnected' } );
	
	port = null;
};

function disableBrowserActionForAllTabs() {
	if ( !isSrvListening ) {
		chrome.tabs.query( {}, function ( tabs ) {
			for ( var i = 0, j = tabs.length; i < j; i++ ) {
				var iTabId = tabs[i].id;
				
				chrome.browserAction.disable( iTabId );
				chrome.browserAction.setIcon( { path: 'icons/disabled/sm-32.png', tabId: iTabId } );
			}
			// log( tabs[i] );
		} );
	}
}

function enableBrowserActionForAllTabs() {
	if ( isSrvListening ) {
		chrome.tabs.query( {}, function ( tabs ) {
			for ( var i = 0, j = tabs.length; i < j; i++ ) {
				var iTabId = tabs[i].id;
				
				chrome.browserAction.enable( iTabId );
				chrome.browserAction.setIcon( { path: 'icons/sm-32.png', tabId: iTabId } );
			}
			// log( tabs[i] );
		} );
	}
}

function onTabCreated( tab ) {
	console.log( tab );
	
	if ( !isSrvListening ) {
		chrome.browserAction.disable();
		chrome.browserAction.setIcon( { path: 'icons/disabled/sm-32.png' } );
	}
}

document.addEventListener( 'DOMContentLoaded', function () {
	var hostName = 'com.supermemory.nativemsgproxy';
	
	port = chrome.runtime.connectNative( hostName );
	port.onMessage.addListener( onNativeMessage );
	port.onDisconnect.addListener( onDisconnected );
	chrome.tabs.onCreated.addListener( onTabCreated );
	disableBrowserActionForAllTabs();
} );

chrome.browserAction.onClicked.addListener( function( tab ) {
	var msg = {
			cmd: "url",
			val: tab.url
		};

	if ( port != null ) {
		port.postMessage( msg );
	}
    
	console.log( prefix + tab.url );
} );