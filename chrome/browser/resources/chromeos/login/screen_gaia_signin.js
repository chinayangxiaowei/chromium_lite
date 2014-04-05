// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe signin screen implementation.
 */

cr.define('login', function() {

  /**
   * Creates a new sign in screen div.
   * @constructor
   * @extends {HTMLDivElement}
   */
  var GaiaSigninScreen = cr.ui.define('div');

  /**
   * Registers with Oobe.
   */
  GaiaSigninScreen.register = function() {
    var screen = $('gaia-signin');
    GaiaSigninScreen.decorate(screen);
    Oobe.getInstance().registerScreen(screen);
    window.addEventListener('message',
                            screen.onMessage_.bind(screen), false);
  };

  GaiaSigninScreen.prototype = {
    __proto__: HTMLDivElement.prototype,

    // Authentication extension's start page URL.
    extension_url_: null,

    /** @inheritDoc */
    decorate: function() {
      $('createAccount').onclick = function() {
        chrome.send('createAccount');
      };
      $('guestSignin').onclick = function() {
        chrome.send('launchIncognito');
      };
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return localStrings.getString('signinScreenTitle');
    },

    /**
     * Event handler that is invoked just before the frame is shown.
     * @param data {string} Screen init payload. Url of auth extension start
     *        page.
     */
    onBeforeShow: function(data) {
      console.log('Opening extension: ' + data.startUrl +
                  ', opt_email=' + data.email);
      var frame = $('signin-frame');
      frame.addEventListener('load', function(e) {
        console.log('Frame loaded: ' + data.startUrl);
      });
      frame.contentWindow.location.href = data.startUrl;
      this.extension_url_ = data.startUrl;
      // TODO(xiyuan): Pre-populate Gaia with data.email (if any).

      $('createAccount').hidden = !data.createAccount;
      $('guestSignin').hidden = !data.guestSignin;
    },

    /**
     * Checks if message comes from the loaded authentication extension.
     * @param e {object} Payload of the received HTML5 message.
     * @type {bool}
     */
    isAuthExtMessage_: function(e) {
      return this.extension_url_.indexOf(e.origin) == 0;
    },

    /**
     * Event handler that is invoked when HTML5 message is received.
     * @param e {object} Payload of the received HTML5 message.
     */
    onMessage_: function(e) {
      var msg = e.data;
      if (msg.method == 'completeLogin' && this.isAuthExtMessage_(e)) {
        chrome.send('completeLogin', [msg.email, msg.password] );
      }
    },

    /**
     * Clears input fields and switches to input mode.
     * @param {boolean} takeFocus True to take focus.
     */
    reset: function(takeFocus) {
      // Reload and show the sign-in UI.
      Oobe.showSigninUI();
    }
  };

  return {
    GaiaSigninScreen: GaiaSigninScreen
  };
});
