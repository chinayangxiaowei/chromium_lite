// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options', function() {
  /** @const */ var DictionaryWordsList =
      options.dictionary_words.DictionaryWordsList;
  /** @const */ var Page = cr.ui.pageManager.Page;
  /** @const */ var PageManager = cr.ui.pageManager.PageManager;

  /**
   * Adding and removing words in custom spelling dictionary.
   * @constructor
   * @extends {cr.ui.pageManager.Page}
   */
  function EditDictionaryOverlay() {
    Page.call(this, 'editDictionary',
              loadTimeData.getString('languageDictionaryOverlayPage'),
              'language-dictionary-overlay-page');
  }

  cr.addSingletonGetter(EditDictionaryOverlay);

  EditDictionaryOverlay.prototype = {
    __proto__: Page.prototype,

    /**
     * A list of words in the dictionary.
     * @type {options.dictionary_words.DictionaryWordsList}
     * @private
     */
    wordList_: null,

    /**
     * The input field for searching for words in the dictionary.
     * @type {HTMLElement}
     * @private
     */
    searchField_: null,

    /**
     * The paragraph of text that indicates that search returned no results.
     * @type {HTMLElement}
     * @private
     */
    noMatchesLabel_: null,

    /** @override */
    initializePage: function() {
      Page.prototype.initializePage.call(this);

      var wordList = $('language-dictionary-overlay-word-list');
      DictionaryWordsList.decorate(wordList);
      this.wordList_ = assertInstanceof(wordList, DictionaryWordsList);
      this.wordList_.onWordListChanged = function() {
        this.onWordListChanged_();
      }.bind(this);

      this.searchField_ = $('language-dictionary-overlay-search-field');
      this.searchField_.onsearch = function(e) {
        this.wordList_.search(e.currentTarget.value);
      }.bind(this);
      this.searchField_.onkeydown = function(e) {
        // Don't propagate enter key events. Otherwise the default button will
        // activate.
        if (e.keyIdentifier == 'Enter')
          e.stopPropagation();
      };

      this.noMatchesLabel_ = getRequiredElement(
          'language-dictionary-overlay-no-matches');

      $('language-dictionary-overlay-done-button').onclick = function(e) {
        PageManager.closeOverlay();
      };
    },

    /**
     * Refresh the dictionary words when the page is displayed.
     * @override
     */
    didShowPage: function() {
      chrome.send('refreshDictionaryWords');
    },

    /**
     * Update the view based on the changes in the word list.
     * @private
     */
    onWordListChanged_: function() {
      if (this.searchField_.value.length > 0 && this.wordList_.empty) {
        this.noMatchesLabel_.hidden = false;
        this.wordList_.classList.add('no-search-matches');
      } else {
        this.noMatchesLabel_.hidden = true;
        this.wordList_.classList.remove('no-search-matches');
      }
    },
  };

  EditDictionaryOverlay.setWordList = function(entries) {
    EditDictionaryOverlay.getInstance().wordList_.setWordList(entries);
  };

  EditDictionaryOverlay.updateWords = function(add_words, remove_words) {
    EditDictionaryOverlay.getInstance().wordList_.addWords(add_words);
    EditDictionaryOverlay.getInstance().wordList_.removeWords(remove_words);
  };

  EditDictionaryOverlay.getWordListForTesting = function() {
    return EditDictionaryOverlay.getInstance().wordList_;
  };

  return {
    EditDictionaryOverlay: EditDictionaryOverlay
  };
});
