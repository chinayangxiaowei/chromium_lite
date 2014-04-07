// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_WIN_WTS_SESSION_PROCESS_LAUNCHER_H_
#define REMOTING_HOST_WIN_WTS_SESSION_PROCESS_LAUNCHER_H_

#include <windows.h>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/process.h"
#include "base/time.h"
#include "base/timer.h"
#include "base/win/scoped_handle.h"
#include "ipc/ipc_channel.h"
#include "remoting/base/stoppable.h"
#include "remoting/host/win/worker_process_launcher.h"
#include "remoting/host/win/wts_console_observer.h"
#include "remoting/host/worker_process_ipc_delegate.h"

namespace base {
class SingleThreadTaskRunner;
} // namespace base

namespace remoting {

class SasInjector;
class WtsConsoleMonitor;
class WtsSessionProcessLauncherImpl;

class WtsSessionProcessLauncher
    : public Stoppable,
      public WorkerProcessIpcDelegate,
      public WtsConsoleObserver {
 public:
  // Constructs a WtsSessionProcessLauncher object. |stopped_callback| and
  // |main_message_loop| are passed to the undelying |Stoppable| implementation.
  // All interaction with |monitor| should happen on |main_message_loop|.
  // |ipc_message_loop| must be an I/O message loop.
  WtsSessionProcessLauncher(
      const base::Closure& stopped_callback,
      WtsConsoleMonitor* monitor,
      scoped_refptr<base::SingleThreadTaskRunner> main_message_loop,
      scoped_refptr<base::SingleThreadTaskRunner> ipc_message_loop);

  virtual ~WtsSessionProcessLauncher();

  // WorkerProcessIpcDelegate implementation.
  virtual void OnChannelConnected() OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // WtsConsoleObserver implementation.
  virtual void OnSessionAttached(uint32 session_id) OVERRIDE;
  virtual void OnSessionDetached() OVERRIDE;

 protected:
  // Stoppable implementation.
  virtual void DoStop() OVERRIDE;

  // Sends Secure Attention Sequence to the console session.
  void OnSendSasToConsole();

 private:
  // Attempts to launch the host process in the current console session.
  // Schedules next launch attempt if creation of the process fails for any
  // reason.
  void LaunchProcess();

  // Called when the launcher reports the process to be stopped.
  void OnLauncherStopped();

  // |true| if this object is currently attached to the console session.
  bool attached_;

  // Path to the host binary to launch.
  FilePath host_binary_;

  // OS version dependent WorkerProcessLauncher::Delegate implementation
  // instance for the currently-running child process.
  scoped_refptr<WtsSessionProcessLauncherImpl> impl_;

  // WorkerProcessLauncher::Delegate implementation that will be used to launch
  // the new child process after a session switch.
  scoped_refptr<WtsSessionProcessLauncherImpl> new_impl_;

  // Time of the last launch attempt.
  base::Time launch_time_;

  // Current backoff delay.
  base::TimeDelta launch_backoff_;

  // Timer used to schedule the next attempt to launch the process.
  base::OneShotTimer<WtsSessionProcessLauncher> timer_;

  // The main service message loop.
  scoped_refptr<base::SingleThreadTaskRunner> main_message_loop_;

  // Message loop used by the IPC channel.
  scoped_refptr<base::SingleThreadTaskRunner> ipc_message_loop_;

  // This pointer is used to unsubscribe from session attach and detach events.
  WtsConsoleMonitor* monitor_;

  scoped_ptr<WorkerProcessLauncher> launcher_;

  scoped_ptr<SasInjector> sas_injector_;

  DISALLOW_COPY_AND_ASSIGN(WtsSessionProcessLauncher);
};

}  // namespace remoting

#endif  // REMOTING_HOST_WIN_WTS_SESSION_PROCESS_LAUNCHER_H_
