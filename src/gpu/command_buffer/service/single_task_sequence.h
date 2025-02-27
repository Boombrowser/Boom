// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SINGLE_TASK_SEQUENCE_H_
#define GPU_COMMAND_BUFFER_SERVICE_SINGLE_TASK_SEQUENCE_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/sequence_id.h"
#include "gpu/command_buffer/service/task_graph.h"
#include "gpu/gpu_gles2_export.h"

namespace gpu {
// Represents a single task execution sequence. Tasks posted to a sequence are
// run in order. Tasks across sequences should be synchronized using sync
// tokens. Destroying the sequence will drop tasks which haven't been executed
// yet.
class GPU_GLES2_EXPORT SingleTaskSequence {
 public:
  virtual ~SingleTaskSequence() {}

  // Returns identifier used for identifying sync tokens with this sequence,
  // and for scheduling.
  virtual SequenceId GetSequenceId() = 0;

  // Returns true if sequence should yield while running its current task.
  virtual bool ShouldYield() = 0;

  // Schedule a task with provided sync token dependencies and release.
  // For scheduling from viz thread, due to limitations in Android WebView,
  // ScheduleTask is only available to be called inside initialization,
  // teardown, and DrawAndSwap.
  // |report_callback| will be called on the same thread and before |task| runs.
  virtual void ScheduleTask(
      gpu::TaskCallback task,
      std::vector<SyncToken> sync_token_fences,
      const SyncToken& release,
      ReportingCallback report_callback = ReportingCallback()) = 0;
  virtual void ScheduleTask(
      base::OnceClosure task,
      std::vector<SyncToken> sync_token_fences,
      const SyncToken& release,
      ReportingCallback report_callback = ReportingCallback()) = 0;

  // If |ScheduleGpuTask| is available, then this is equivalent to
  // ScheduleGpuTask. Otherwise, the |task|, |sync_token_fences| and |release|
  // are retained and run when |ScheduleGpuTask| becomes available. Either case,
  // tasks in |ScheduleTask| and |ScheduleOrRetainTask| are sequenced by the
  // call order; calling this instead of |ScheduleTask| can only delay but not
  // reorder tasks.
  // |report_callback| will be called on the same thread and before |task| runs.
  virtual void ScheduleOrRetainTask(
      base::OnceClosure task,
      std::vector<SyncToken> sync_token_fences,
      const SyncToken& release,
      ReportingCallback report_callback = ReportingCallback()) = 0;

  // Continue running the current task after yielding execution.
  virtual void ContinueTask(gpu::TaskCallback task) = 0;
  virtual void ContinueTask(base::OnceClosure task) = 0;

  // Creates a SyncPointClientState associated with the sequence.
  [[nodiscard]] virtual ScopedSyncPointClientState CreateSyncPointClientState(
      CommandBufferNamespace namespace_id,
      CommandBufferId command_buffer_id) = 0;
};
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SINGLE_TASK_SEQUENCE_H_
