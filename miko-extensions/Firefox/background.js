const prefix = "Send to SuperMemo: ";

var isSrvListening = false,
	port = null;

function log( message ) {
	console.log( message );
};

var onNativeMessage = ( message ) => {
	debugger;
	
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
};

var onDisconnected = () => {
	log( { event: 'disconnected' } );
	
	port = null;
};

function disableBrowserActionForAllTabs() {	
	if ( !isSrvListening ) {
		browser.tabs.query( {} ).then( ( tabs ) => {
			// debugger;
			
			for ( var i = 0, j = tabs.length; i < j; i++ ) {
				var iTabId = tabs[i].id;
				
				browser.browserAction.disable( iTabId );
				browser.browserAction.setIcon( { path: 'icons/disabled/sm-32.png', tabId: iTabId } );
			}
			// log( tabs[i] );
		} );
	}
}

function enableBrowserActionForAllTabs() {
	if ( isSrvListening ) {
		browser.tabs.query( {} ).then( ( tabs ) => {
			// debugger;
			
			for ( var i = 0, j = tabs.length; i < j; i++ ) {
				var iTabId = tabs[i].id;
				
				browser.browserAction.enable( iTabId );
				browser.browserAction.setIcon( { path: 'icons/sm-32.png', tabId: iTabId } );
			}
			// log( tabs[i] );
		} );
	}
}

function onTabCreated( tab ) {
	console.log( tab );
	
	if ( !isSrvListening ) {
		browser.browserAction.disable();
		browser.browserAction.setIcon( { path: 'icons/disabled/sm-32.png' } );
	}
};

document.addEventListener( 'DOMContentLoaded', function () {
	var hostName = 'com.supermemory.nativemsgproxy';
	
	port = browser.runtime.connectNative( hostName );
	port.onMessage.addListener( onNativeMessage );
	port.onDisconnect.addListener( onDisconnected );
	browser.tabs.onCreated.addListener( onTabCreated );
	disableBrowserActionForAllTabs();
} );

browser.browserAction.onClicked.addListener( function( tab ) {
	var msg = {
			cmd: "url",
			val: tab.url
		};

	if ( port != null ) {
		port.postMessage( msg );
	}
    
	log( prefix + tab.url );
} );