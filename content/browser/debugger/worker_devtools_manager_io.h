// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEBUGGER_WORKER_DEVTOOLS_MANAGER_IO_H_
#define CONTENT_BROWSER_DEBUGGER_WORKER_DEVTOOLS_MANAGER_IO_H_
#pragma once

#include "base/basictypes.h"
#include "base/memory/singleton.h"
#include "base/scoped_ptr.h"

namespace IPC {
class Message;
}
class DevToolsClientHost;
class WorkerDevToolsMessageFilter;

// All methods are supposed to be called on the IO thread.
class WorkerDevToolsManagerIO {
 public:
  // Returns the WorkerDevToolsManagerIO singleton.
  static WorkerDevToolsManagerIO* GetInstance();

  // This method is called on the UI thread by the devtools handler.
  static bool ForwardToWorkerDevToolsAgentOnUIThread(
      DevToolsClientHost* from,
      const IPC::Message& message);

  void OpenDevToolsForWorker(int worker_process_id, int worker_route_id);
  void WorkerDevToolsClientClosing(int worker_process_host_id,
                                   int worker_route_id);

  void ForwardToDevToolsClient(int worker_process_id,
                               int worker_route_id,
                               const IPC::Message& message);
  void WorkerProcessDestroying(int worker_process_host_id);

 private:
  friend struct DefaultSingletonTraits<WorkerDevToolsManagerIO>;

  WorkerDevToolsManagerIO();
  ~WorkerDevToolsManagerIO();
  static void ForwardToWorkerDevToolsAgentOnIOThread(
      int worker_process_host_id,
      int worker_route_id,
      const IPC::Message& message);
  void ForwardToWorkerDevToolsAgent(int worker_process_host_id,
                                    int worker_route_id,
                                    const IPC::Message& message);
  class InspectedWorkersList;
  scoped_ptr<InspectedWorkersList> inspected_workers_;

  DISALLOW_COPY_AND_ASSIGN(WorkerDevToolsManagerIO);
};

#endif  // CONTENT_BROWSER_DEBUGGER_WORKER_DEVTOOLS_MANAGER_IO_H_
