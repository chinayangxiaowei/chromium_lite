// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// Namespace object for the utilities.
function ImageUtil() {}

// Performance trace.
ImageUtil.trace = (function() {
  function PerformanceTrace() {
    this.lines_ = {};
    this.timers_ = {};
    this.container_ = null;
  }

  PerformanceTrace.prototype.report_ = function(key, value) {
    if (!this.container_) {
      this.container_ = document.getElementById('debug-output');
      if (!this.container_) return;
    }
    if (!(key in this.lines_)) {
      var div = this.lines_[key] = document.createElement('div');
      this.container_.appendChild(div);
    }
    this.lines_[key].textContent = key + ': ' + value;
  };

  PerformanceTrace.prototype.resetTimer = function(key) {
    this.timers_[key] = Date.now();
  }

  PerformanceTrace.prototype.reportTimer = function(key) {
    this.report_(key, (Date.now() - this.timers_[key]) + 'ms');
  };

  return new PerformanceTrace();
})();


ImageUtil.clip = function(min, value, max) {
  return Math.max(min, Math.min(max, value));
};

ImageUtil.between = function(min, value, max) {
  return (value - min) * (value - max) <= 0;
};

/**
 * Computes the function for every integer value between 0 and max and stores
 * the results. Rounds and clips the results to fit the [0..255] range.
 * Used to speed up pixel manipulations.
 * @param {Function} func Function returning a number.
 * @param {Number} max Maximum argument value (inclusive).
 * @return {Array<Number>} Computed results
 */

ImageUtil.precomputeByteFunction = function(func, max) {
  var results = [];
  for (var arg = 0; arg <= max; arg ++) {
    results.push(Math.max(0, Math.min(0xFF, Math.round(func(arg)))));
  }
  return results;
}

/**
 * Rectangle class.
 */

/**
 * Rectange constructor takes 0, 1, 2 or 4 arguments.
 * Supports following variants:
 *   new Rect(left, top, width, height)
 *   new Rect(width, height)
 *   new Rect(rect)         // anything with left, top, width, height properties
 *   new Rect(bounds)       // anything with left, top, right, bottom properties
 *   new Rect(canvas|image) // anything with width and height properties.
 *   new Rect()             // empty rectangle.
 * @constructor
 */
function Rect(args) {
  switch (arguments.length) {
    case 4:
      this.left = arguments[0];
      this.top = arguments[1];
      this.width = arguments[2];
      this.height = arguments[3];
      return;

    case 2:
      this.left = 0;
      this.top = 0;
      this.width = arguments[0];
      this.height = arguments[1];
      return;

    case 1: {
      var source = arguments[0];
      if (source.hasOwnProperty('left') && source.hasOwnProperty('top')) {
        this.left = source.left;
        this.top = source.top;
        if (source.hasOwnProperty('right') && source.hasOwnProperty('bottom')) {
          this.width = source.right - source.left;
          this.height = source.bottom - source.top;
          return;
        }
      } else {
        this.left = 0;
        this.top = 0;
      }
      if (source.hasOwnProperty('width') && source.hasOwnProperty('height')) {
        this.width = source.width;
        this.height = source.height;
        return;
      }
      break; // Fall through to the error message.
    }

    case 0:
      this.left = 0;
      this.top = 0;
      this.width = 0;
      this.height = 0;
      return;
  }
  console.error('Invalid Rect constructor arguments:',
       Array.apply(null, arguments));
}

/**
 * @return {Rect} A rectangle with every dimension scaled.
 */
Rect.prototype.scale = function(factor) {
  return new Rect(
      this.left * factor,
      this.top * factor,
      this.width * factor,
      this.height * factor);
};

/**
 * @return {Rect} A rectangle shifted by (dx,dy), same size.
 */
Rect.prototype.shift = function(dx, dy) {
  return new Rect(this.left + dx, this.top + dy, this.width, this.height);
};

/**
 * @return {Rect} A rectangle with left==x and top==y, same size.
 */
Rect.prototype.moveTo = function(x, y) {
  return new Rect(x, y, this.width, this.height);
};

/**
 * @return {Rect} A rectangle inflated by (dx, dy), same center.
 */
Rect.prototype.inflate = function(dx, dy) {
  return new Rect(
      this.left - dx, this.top - dx, this.width + 2 * dx, this.height + 2 * dy);
};

/**
 * @return {Boolean} True if the point lies inside the rectange.
 */
Rect.prototype.inside = function(x, y) {
  return this.left <= x && x < this.left + this.width &&
         this.top <= y && y < this.top + this.height;
};

/*
 * Useful shortcuts for drawing (static functions).
 */

/**
 * Draws the image in context with appropriate scaling.
 */
Rect.drawImage = function(context, image, dstRect, srcRect) {
  context.drawImage(image,
      srcRect.left, srcRect.top, srcRect.width, srcRect.height,
      dstRect.left, dstRect.top, dstRect.width, dstRect.height);
};

/**
 * Strokes the rectangle.
 */
Rect.stroke = function(context, rect) {
  context.strokeRect(rect.left, rect.top, rect.width, rect.height);
};

/**
 * Fills the space between the two rectangles.
 */
Rect.fillBetween= function(context, inner, outer) {
  var inner_right = inner.left + inner.width;
  var inner_bottom = inner.top + inner.height;
  var outer_right = outer.left + outer.width;
  var outer_bottom = outer.top + outer.height;
  if (inner.top > outer.top) {
    context.fillRect(
        outer.left, outer.top, outer.width, inner.top - outer.top);
  }
  if (inner.left > outer.left) {
    context.fillRect(
        outer.left, inner.top, inner.left - outer.left, inner.height);
  }
  if (inner.width < outer_right) {
    context.fillRect(
        inner_right, inner.top, outer_right - inner_right, inner.height);
  }
  if (inner.height < outer_bottom) {
    context.fillRect(
        outer.left, inner_bottom, outer.width, outer_bottom - inner_bottom);
  }
};

/**
 * Circle class.
 */

function Circle(x, y, R) {
  this.x = x;
  this.y = y;
  this.squaredR = R * R;
};

Circle.prototype.inside = function(x, y) {
  x -= this.x;
  y -= this.y;
  return x * x + y * y <= this.squaredR;
};
