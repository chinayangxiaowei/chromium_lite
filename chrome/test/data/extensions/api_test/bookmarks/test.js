// bookmarks api test
// browser_tests.exe --gtest_filter=ExtensionApiTest.Bookmarks

// This is global state that is maintained across tests as a reference
// to compare against what's fetched from the browser (using compareTrees).
// TODO(erikkay) It would be better if each test was self-contained and
// didn't depend on global state.
var expected = [
  {"children": [
      {children:[], id:"1", parentId:"0", index:0, title:"Bookmarks bar"},
      {children:[], id:"2", parentId:"0", index:1, title:"Other bookmarks"}
    ],
   id:"0", title:""
  }
];

function bookmarksBar() { return expected[0].children[0]; }
function otherBookmarks() { return expected[0].children[1]; }

// Some variables that are used across multiple tests.
var node1 = {parentId:"1", title:"Foo bar baz",
             url:"http://www.example.com/hello"};
var node2 = {parentId:"1", title:"foo quux",
             url:"http://www.example.com/bar"};
var node3 = {parentId:"1", title:"bar baz",
             url:"http://www.google.com/hello/quux"};
var quota_node1 = {parentId:"1", title:"Dave",
                   url:"http://www.dmband.com/"};
var quota_node2 = {parentId:"1", title:"UW",
                   url:"http://www.uwaterloo.ca/"};
var quota_node3 = {parentId:"1", title:"Whistler",
                   url:"http://www.whistlerblackcomb.com/"};

var pass = chrome.test.callbackPass;
var fail = chrome.test.callbackFail;

function compareNode(left, right) {
  //chrome.test.log("compareNode()");
  //chrome.test.log(JSON.stringify(left, null, 2));
  //chrome.test.log(JSON.stringify(right, null, 2));
  // TODO(erikkay): do some comparison of dateAdded
  if (left.id != right.id)
    return "id mismatch: " + left.id + " != " + right.id;
  if (left.title != right.title) {
    // TODO(erikkay): This resource dependency still isn't working reliably.
    // See bug 19866.
    // return "title mismatch: " + left.title + " != " + right.title;
    chrome.test.log("title mismatch: " + left.title + " != " + right.title);
  }
  if (left.url != right.url)
    return "url mismatch: " + left.url + " != " + right.url;
  if (left.index != right.index)
    return "index mismatch: " + left.index + " != " + right.index;
  return true;
}

function compareTrees(left, right, verbose) {
  if (verbose) {
    chrome.test.log(JSON.stringify(left) || "<null>");
    chrome.test.log(JSON.stringify(right) || "<null>");
  }
  if (left == null && right == null) {
    return true;
  }
  if (left == null || right == null)
    return left + " !+ " + right;
  if (left.length != right.length)
    return "count mismatch: " + left.length + " != " + right.length;
  for (var i = 0; i < left.length; i++) {
    var result = compareNode(left[i], right[i]);
    if (result !== true) {
      chrome.test.log(result);
      chrome.test.log(JSON.stringify(left, null, 2));
      chrome.test.log(JSON.stringify(right, null, 2));
      return result;
    }
    result = compareTrees(left[i].children, right[i].children);
    if (result !== true)
      return result;
  }
  return true;
}

// |expectedParent| is a child within the global |expected| that each node in
// the array|nodes| should be added to.  |callback| will be called when all of
// the nodes have been successfully created.
function createNodes(expectedParent, nodes, callback) {
  function createNext() {
    if (nodes.length) {
      var node = nodes.shift();
      chrome.bookmarks.create(node, function(results) {
        node.id = results.id;
        node.index = results.index;
        expectedParent.children.push(node);

        // Create each node a minimum of 1ms apart.  This ensures that each
        // node will have a unique timestamp.
        setTimeout(createNext, 1);
      });
    } else {
      callback();
    }
  }
  createNext();
}

// Calls getTree and verifies that the results match the global |expected|
// state.  Assigns over expected in the end since getTree has more data
// (e.g. createdDate) which tests may want to depend on.
function verifyTreeIsExpected(callback) {
  chrome.bookmarks.getTree(pass(function(results) {
    chrome.test.assertTrue(compareTrees(expected, results),
                           "getTree() result != expected");
    expected = results;
    callback();
  }));
}

function run() {
chrome.test.runTests([
  function getTree() {
    verifyTreeIsExpected(pass());
  },

  function get() {
    chrome.bookmarks.get("1", pass(function(results) {
      chrome.test.assertTrue(compareNode(results[0], expected[0].children[0]));
    }));
    chrome.bookmarks.get("42", fail("Can't find bookmark for id."));
  },

  function getArray() {
    chrome.bookmarks.get(["1", "2"], pass(function(results) {
      chrome.test.assertTrue(compareNode(results[0], expected[0].children[0]),
                             "get() result != expected");
      chrome.test.assertTrue(compareNode(results[1], expected[0].children[1]),
                             "get() result != expected");
    }));
  },

  function getChildren() {
    chrome.bookmarks.getChildren("0", pass(function(results) {
      chrome.test.assertTrue(compareNode(results[0], expected[0].children[0]),
                             "getChildren() result != expected");
      chrome.test.assertTrue(compareNode(results[1], expected[0].children[1]),
                             "getChildren() result != expected");
    }));
  },

  function create() {
    var node = {parentId:"1", title:"google", url:"http://www.google.com/"};
    chrome.test.listenOnce(chrome.bookmarks.onCreated, function(id, created) {
      node.id = created.id;
      node.index = 0;
      chrome.test.assertEq(id, node.id);
      chrome.test.assertTrue(compareNode(node, created));
    });
    chrome.bookmarks.create(node, pass(function(results) {
      node.id = results.id;  // since we couldn't know this going in
      node.index = 0;
      chrome.test.assertTrue(compareNode(node, results),
                             "created node != source");
      expected[0].children[0].children.push(node);
    }));
  },

  function createFolder() {
    var node = {parentId:"1", title:"foo bar"};  // folder
    chrome.test.listenOnce(chrome.bookmarks.onCreated, function(id, created) {
      node.id = created.id;
      node.index = 1;
      node.children = [];
      chrome.test.assertTrue(compareNode(node, created));
    });
    chrome.bookmarks.create(node, pass(function(results) {
      node.id = results.id;  // since we couldn't know this going in
      node.index = 1;
      node.children = [];
      chrome.test.assertTrue(compareNode(node, results),
                             "created node != source");
      expected[0].children[0].children.push(node);
    }));
  },

  function moveSetup() {
    createNodes(bookmarksBar(), [node1, node2, node3], pass(function() {
      verifyTreeIsExpected(pass());
    }));
  },

  function move() {
    // Move node1, node2, and node3 from their current location (the bookmark
    // bar) to be under the "foo bar" folder (created in createFolder()).
    // Then move that folder to be under the "other bookmarks" folder.
    var folder = expected[0].children[0].children[1];
    var old_node1 = expected[0].children[0].children[2];
    chrome.test.listenOnce(chrome.bookmarks.onMoved, function(id, moveInfo) {
      chrome.test.assertEq(node1.id, id);
      chrome.test.assertEq(moveInfo.parentId, folder.id);
      chrome.test.assertEq(moveInfo.index, 0);
      chrome.test.assertEq(moveInfo.oldParentId, old_node1.parentId);
      chrome.test.assertEq(moveInfo.oldIndex, old_node1.index);
    });
    chrome.bookmarks.move(node1.id, {parentId:folder.id},
                          pass(function(results) {
      chrome.test.assertEq(results.parentId, folder.id);
      node1.parentId = results.parentId;
      node1.index = 0;
    }));
    chrome.bookmarks.move(node2.id, {parentId:folder.id},
                          pass(function(results) {
      chrome.test.assertEq(results.parentId, folder.id);
      node2.parentId = results.parentId;
      node2.index = 1;
    }));
    // Insert node3 at index 1 rather than at the end.  This should result in
    // an order of node1, node3, node2.
    chrome.bookmarks.move(node3.id, {parentId:folder.id, index:1},
                          pass(function(results) {
      chrome.test.assertEq(results.parentId, folder.id);
      chrome.test.assertEq(results.index, 1);
      node3.parentId = results.parentId;
      node3.index = 1;
      node2.index = 2;

      // update expected to match
      expected[0].children[0].children.pop();
      expected[0].children[0].children.pop();
      expected[0].children[0].children.pop();
      folder.children.push(node1);
      folder.children.push(node3);
      folder.children.push(node2);

      verifyTreeIsExpected(pass());
    }));

    // Move folder (and its children) to be a child of Other Bookmarks.
    var other = expected[0].children[1];
    chrome.bookmarks.move(folder.id, {parentId:other.id},
                          pass(function(results) {
      chrome.test.assertEq(results.parentId, other.id);
      folder.parentId = results.parentId;

      var folder2 = expected[0].children[0].children.pop();
      folder2.parentId = other.parentId;
      folder2.index = 0;
      expected[0].children[1].children.push(folder2);

      verifyTreeIsExpected(pass());
    }));
  },

  function search() {
    chrome.bookmarks.search("baz bar", pass(function(results) {
      // matches node1 & node3
      chrome.test.assertEq(2, results.length);
    }));
    chrome.bookmarks.search("www hello", pass(function(results) {
      // matches node1 & node3
      chrome.test.assertEq(2, results.length);
    }));
    chrome.bookmarks.search("bar example",
                            pass(function(results) {
      // matches node2
      chrome.test.assertEq(1, results.length);
    }));
    chrome.bookmarks.search("foo bar", pass(function(results) {
      // matches node1
      chrome.test.assertEq(1, results.length);
    }));
    chrome.bookmarks.search("quux", pass(function(results) {
      // matches node2 & node3
      chrome.test.assertEq(2, results.length);
    }));
  },

  function update() {
    var title = "hello world";
    chrome.test.listenOnce(chrome.bookmarks.onChanged, function(id, changes) {
      chrome.test.assertEq(title, changes.title);
    });
    chrome.bookmarks.update(node1.id, {"title": title}, pass(function(results) {
      chrome.test.assertEq(title, results.title);
    }));

    var url = "http://example.com/hello";
    chrome.bookmarks.update(node1.id, {"url": url}, pass(function(results) {
      // Make sure that leaving out the title does not set the title to empty.
      chrome.test.assertEq(title, results.title);
      chrome.test.assertEq(url, results.url);

      // Empty or invalid URLs should not change the URL.
      var bad_url = "";
      chrome.bookmarks.update(node1.id, {"url": bad_url},
        pass(function(results) {
          chrome.bookmarks.get(node1.id, pass(function(results) {
            chrome.test.assertEq(url, results[0].url);
            chrome.test.log("URL UNCHANGED");
          }));
        })
      );

      // Invalid URLs also generate an error.
      bad_url = "I am not an URL";
      chrome.bookmarks.update(node1.id, {"url": bad_url}, fail("Invalid URL.",
        function(results) {
          chrome.bookmarks.get(node1.id, pass(function(results) {
            chrome.test.assertEq(url, results[0].url);
            chrome.test.log("URL UNCHANGED");
          }));
        })
      );
    }));
  },

  function remove() {
    var parentId = node1.parentId;
    chrome.test.listenOnce(chrome.bookmarks.onRemoved,
                           function(id, removeInfo) {
      chrome.test.assertEq(id, node1.id);
      chrome.test.assertEq(removeInfo.parentId, parentId);
      chrome.test.assertEq(removeInfo.index, node1.index);
    });
    chrome.bookmarks.remove(node1.id, pass(function() {
      // Update expected to match.
      // We removed node1, which means that the index of the other two nodes
      // changes as well.
      expected[0].children[1].children[0].children.shift();
      expected[0].children[1].children[0].children[0].index = 0;
      expected[0].children[1].children[0].children[1].index = 1;

      verifyTreeIsExpected(pass());
    }));
  },

  function removeTree() {
    var parentId = node2.parentId;
    var folder = expected[0].children[1].children[0];
    chrome.test.listenOnce(chrome.bookmarks.onRemoved,
                           function(id, removeInfo) {
      chrome.test.assertEq(id, folder.id);
      chrome.test.assertEq(removeInfo.parentId, folder.parentId);
      chrome.test.assertEq(removeInfo.index, folder.index);
    });
    chrome.bookmarks.removeTree(parentId, pass(function(){
      // Update expected to match.
      expected[0].children[1].children.shift();

      verifyTreeIsExpected(pass());
    }));
  },

  function quotaLimitedCreate() {
    var node = {parentId:"1", title:"quotacreate", url:"http://www.quota.com/"};
    for (i = 0; i < 100; i++) {
      chrome.bookmarks.create(node, pass(function(results) {
        expected[0].children[0].children.push(results);
      }));
    }
    chrome.bookmarks.create(node,
                            fail("This request exceeds available quota."));

    chrome.test.resetQuota();

    // Also, test that > 100 creations of different items is fine.
    for (i = 0; i < 101; i++) {
      var changer = {parentId:"1", title:"" + i, url:"http://www.quota.com/"};
      chrome.bookmarks.create(changer, pass(function(results) {
        expected[0].children[0].children.push(results);
      }));
    }
  },

  function quotaSetup() {
    createNodes(bookmarksBar(),
                [quota_node1, quota_node2, quota_node3],
                pass(function() {
      verifyTreeIsExpected(pass());
    }));
  },

  function quotaLimitedUpdate() {
    var title = "hello, world!";
    for (i = 0; i < 100; i++) {
      chrome.bookmarks.update(quota_node1.id, {"title": title},
          pass(function(results) {
                 chrome.test.assertEq(title, results.title);
               }
      ));
    }
    chrome.bookmarks.update(quota_node1.id, {"title": title},
        fail("This request exceeds available quota."));

    chrome.test.resetQuota();
  },

  function getRecentSetup() {
    // Clean up tree
    ["1", "2"].forEach(function(id) {
      chrome.bookmarks.getChildren(id, pass(function(children) {
        children.forEach(function(child, i) {
          chrome.bookmarks.removeTree(child.id, pass(function() {}));

          // When we have removed the last child we can continue.
          if (i == children.length - 1)
            afterRemove();
        });
      }));
    });

    function afterRemove() {
      // Once done add 3 nodes
      chrome.bookmarks.getTree(pass(function(results) {
        chrome.test.assertEq(0, results[0].children[0].children.length);
        chrome.test.assertEq(0, results[0].children[1].children.length);
        expected = results;

        // Reset the nodes
        node1 = {parentId:"1", title:"Foo bar baz",
                 url:"http://www.example.com/hello"};
        node2 = {parentId:"1", title:"foo quux",
                 url:"http://www.example.com/bar"};
        node3 = {parentId:"1", title:"bar baz",
                  url:"http://www.google.com/hello/quux"};
        createNodes(bookmarksBar(), [node1, node2, node3], pass(function() {
          verifyTreeIsExpected(pass());
        }));
      }));
    }
  },

  function getRecent() {
    var failed = false;
    try {
      chrome.bookmarks.getRecent(0, function() {});
    } catch (ex) {
      failed = true;
    }
    chrome.test.assertTrue(failed, "Calling with 0 should fail");

    chrome.bookmarks.getRecent(10000, pass(function(results) {
      chrome.test.assertEq(3, results.length,
                           "Should have gotten all recent bookmarks");
    }));

    chrome.bookmarks.getRecent(2, pass(function(results) {
      chrome.test.assertEq(2, results.length,
                           "Should only get the last 2 bookmarks");

      chrome.test.assertTrue(compareNode(node3, results[0]));
      chrome.test.assertTrue(compareNode(node2, results[1]));
    }));
  }
]);
}

run();
