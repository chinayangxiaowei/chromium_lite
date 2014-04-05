// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A simple virtual keyboard implementation.
 */

// TODO(mazda): Support more virtual keyboards like French VK, German VK, etc.
var KEYBOARDS = {
  'us': {
    'definition': KEYS_US,
    'aspect': 3.15,
    // No canvas.
  },
  'handwriting-vk': {
    'definition': KEYS_HANDWRITING_VK,
    'aspect': 2.1,
    // TODO(yusukes): Stop special-casing canvas when mazda's i18n keyboard
    // code is ready.
    'canvas': null
  }
};

/**
 * The keyboard layout name currently in use.
 * @type {string}
 */
var currentKeyboardLayout = 'us';

/**
 * The ratio of the row height to the font size.
 * @type {number}
 */
const kFontSizeRatio = 3.5;

/**
 * Return the id attribute of the keyboard element for the given layout.
 * @param {string} layout The keyboard layout.
 * @return {string} The id attribute of the keyboard element.
 */
function getKeyboardId(layout) {
  return 'keyboard_' + layout;
}

/**
 * Return the aspect ratio of the current keyboard.
 * @param {string} layout The keyboard layout.
 * @return {number} The aspect ratio of the current keyboard.
 */
function getKeyboardAspect() {
  return KEYBOARDS[currentKeyboardLayout]['aspect'];
}

/**
 * Calculate the height of the keyboard based on the size of the page.
 * @return {number} The height of the keyboard in pixels.
 */
function getKeyboardHeight() {
  var x = window.innerWidth;
  var y = window.innerHeight - ((imeui && imeui.visible) ? IME_HEIGHT : 0);
  return (x > getKeyboardAspect() * y) ?
      y : Math.floor(x / getKeyboardAspect());
}

/**
 * Set the keyboard mode.
 * @param {string} mode The new mode.
 * @return {void}
 */
function setMode(mode) {
  var rows = KEYBOARDS[currentKeyboardLayout]['rows'];
  for (var i = 0; i < rows.length; ++i) {
    rows[i].showMode(mode);
  }
}

/**
 * Create a DOM of the keyboard rows for the given keyboard layout.
 * Do nothing if the DOM is already created.
 * @param {string} layout The keyboard layout for which rows are created.
 * @param {Element} The DOM Element to which rows are appended.
 * @return {void}
 */
function initRows(layout, element) {
  var keyboard = KEYBOARDS[layout];
  if ('rows' in keyboard) {
    return;
  }
  var def = keyboard['definition'];
  var rows = [];
  for (var i = 0; i < def.length; ++i) {
    rows.push(new Row(i, def[i]));
  }
  keyboard['rows'] = rows;

  // A div element which holds rows for the layout.
  var rowsDiv = document.createElement('div');
  rowsDiv.className = 'rows';
  for (var i = 0; i < rows.length; ++i) {
    rowsDiv.appendChild(rows[i].makeDOM());
    rows[i].showMode(currentMode);
  }
  keyboard['rowsDiv'] = rowsDiv;
  element.appendChild(rowsDiv);
}

/**
 * Create a DOM of the handwriting canvas for the given keyboard layout.
 * Do nothing if the DOM is already created or the layout doesn't have canvas.
 * @param {string} layout The keyboard layout for which canvas is created.
 * @param {Element} The DOM Element to which canvas is appended.
 * @return {void}
 */
function initHandwritingCanvas(layout, element) {
  var keyboard = KEYBOARDS[layout];
  if (!('canvas' in keyboard) || keyboard['canvas']) {
    return;
  }
  var canvas = new HandwritingCanvas();
  canvas.className = 'handwritingcanvas';
  var border = 1;
  var marginTop = 5;
  var canvasHeight = getKeyboardHeight() - 2 * border - marginTop;
  canvas.resize(canvasHeight);
  keyboard['canvas'] = canvas;
  element.appendChild(canvas);
}

/**
 * Create a DOM of the keyboard for the given keyboard layout.
 * Do nothing if the DOM is already created.
 * @param {string} layout The keyboard layout for which keyboard is created.
 * @param {Element} The DOM Element to which keyboard is appended.
 * @return {void}
 */
function initKeyboard(layout, element) {
  var keyboard = KEYBOARDS[layout];
  if (!keyboard || keyboard['keyboardDiv']) {
    return;
  }
  var keyboardDiv = document.createElement('div');
  keyboardDiv.id = getKeyboardId(layout);
  keyboardDiv.className = 'nodisplay';
  initRows(layout, keyboardDiv);
  initHandwritingCanvas(layout, keyboardDiv);
  keyboard['keyboardDiv'] = keyboardDiv;
  element.appendChild(keyboardDiv);
}

/**
 * Resize the keyboard according to the new window size.
 * @return {void}
 */
window.onresize = function() {
  var keyboardDiv = document.getElementById(getKeyboardId(
      currentKeyboardLayout));
  var height = getKeyboardHeight();
  keyboardDiv.style.height = height + 'px';
  var mainDiv = document.getElementById('main');
  mainDiv.style.width = Math.floor(getKeyboardAspect() * height) + 'px';
  var rowsLength = KEYBOARDS[currentKeyboardLayout]['rows'].length;
  keyboardDiv.style.fontSize = (height / kFontSizeRatio / rowsLength) + 'px';
  updateIme();
}

/**
 * Init the keyboard.
 * @return {void}
 */
window.onload = function() {
  var body = document.getElementById('b');

  var mainDiv = document.createElement('div');
  mainDiv.className = 'main';
  mainDiv.id = 'main';
  body.appendChild(mainDiv);

  initIme(mainDiv);
  initKeyboard(currentKeyboardLayout, mainDiv);

  window.onhashchange();

  // Restore the keyboard to the default state when it is hidden.
  // Ref: dvcs.w3.org/hg/webperf/raw-file/tip/specs/PageVisibility/Overview.html
  document.addEventListener("webkitvisibilitychange", function() {
    if (document.webkitHidden) {
      currentMode = SHIFT_MODE;
      setMode(currentMode);
    }
  }, false);
}
// TODO(bryeung): would be nice to leave less gutter (without causing
// rendering issues with floated divs wrapping at some sizes).

/**
 * Switch the keyboard layout based on the current URL hash.
 * @return {void}
 */
window.onhashchange = function() {
  var old_layout = currentKeyboardLayout;
  var new_layout = location.hash.replace(/^#/, "");
  if (old_layout == new_layout) {
    return;
  }

  if (KEYBOARDS[new_layout] === undefined) {
    // Unsupported layout.
    new_layout = "us";
  }
  currentKeyboardLayout = new_layout;

  var mainDiv = document.getElementById('main');
  initKeyboard(currentKeyboardLayout, mainDiv);

  [new_layout, old_layout].forEach(function(layout) {
      var visible = (layout == new_layout);
      var keyboardDiv = KEYBOARDS[layout]['keyboardDiv'];
      keyboardDiv.className = visible ? 'keyboard' : 'nodisplay';
      var canvas = KEYBOARDS[layout]['canvas'];
      if (canvas !== undefined) {
        if (!visible) {
          canvas.clear();
        }
      }
      if (visible) {
        window.onresize();
      }
    });
}
