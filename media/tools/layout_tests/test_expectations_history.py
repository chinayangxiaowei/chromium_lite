#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Module for the history of the test expectation file."""

import re
import sys
import time
import pysvn

from datetime import datetime
from datetime import timedelta

# Default Webkit SVN location for chromium test expectation file.
# TODO(imasaki): support multiple test expectation files.
DEFAULT_TEST_EXPECTATION_LOCATION = (
    'http://svn.webkit.org/repository/webkit/trunk/'
    'LayoutTests/platform/chromium/test_expectations.txt')


class TestExpectationsHistory:
  """A class to represent history of the test expectation file.

  The history is obtained by calling PySVN.log()/diff() APIs.

  TODO(imasaki): Add more functionalities here like getting some statistics
      about the test expectation file.
  """

  @staticmethod
  def GetDiffBetweenTimes(start, end, testname_list,
                          te_location=DEFAULT_TEST_EXPECTATION_LOCATION):
    """Get difference between time period for the specified test names.

    Given the time period, this method first gets the revision number. Then,
    it gets the diff for each revision. Finally, it keeps the diff relating to
    the test names and returns them along with other information about
    revision.

    Args:
      start: A timestamp specifying start of the time period to be
          looked at.
      end: A timestamp object specifying end of the time period to be
          looked at.
      testname_list: A list of strings representing test names of interest.
      te_location: A location of the test expectation file.

    Returns:
      A list of tuples (old_rev, new_rev, author, date, message, lines). The
          |lines| contains the diff of the tests of interest.
    """
    # Get directory name which is necesary to call PySVN.checkout().
    te_location_dir = te_location[0:te_location.rindex('/')]
    client = pysvn.Client()
    client.checkout(te_location_dir, 'tmp', recurse=False)
    logs = client.log('tmp/test_expectations.txt',
                      revision_start=pysvn.Revision(
                          pysvn.opt_revision_kind.date, start),
                      revision_end=pysvn.Revision(
                          pysvn.opt_revision_kind.date, end))
    result_list = []
    # Find the last revision outside of time period and
    # append it to preserve the last change before entering the time period.
    gobackdays = 1
    while gobackdays < sys.maxint:
      start2 = time.mktime(
          (datetime.fromtimestamp(start) - (
              timedelta(days=gobackdays))).timetuple())
      logs2 = client.log('tmp/test_expectations.txt',
                         revision_start=pysvn.Revision(
                             pysvn.opt_revision_kind.date, start2),
                         revision_end=pysvn.Revision(
                             pysvn.opt_revision_kind.date, start))
      if logs2:
        logs.append(logs2[len(logs2) - 2])
        break
      gobackdays *= 2

    for i in xrange(len(logs) - 1):
      # PySVN.log() returns logs in reverse chronological order.
      new_rev = logs[i].revision.number
      old_rev = logs[i + 1].revision.number
      # Parsing the actual diff.
      text = client.diff('/tmp', 'tmp/test_expectations.txt',
                         revision1=pysvn.Revision(
                             pysvn.opt_revision_kind.number, old_rev),
                         revision2=pysvn.Revision(
                             pysvn.opt_revision_kind.number, new_rev))
      lines = text.split('\n')
      target_lines = []
      for line in lines:
        for testname in testname_list:
          matches = re.findall(testname, line)
          if matches:
            if line[0] == '+' or line[0] == '-':
              target_lines.append(line)
      if target_lines:
        # Needs to convert to normal date string for presentation.
        result_list.append((
            old_rev, new_rev, logs[i].author,
            datetime.fromtimestamp(logs[i].date).strftime('%Y-%m-%d %H:%M:%S'),
            logs[i].message, target_lines))
    return result_list
