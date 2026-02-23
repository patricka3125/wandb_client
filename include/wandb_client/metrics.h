#pragma once

#include "wandb_client/run.h"

#include <chrono>
#include <string>

namespace wandb {

/// RAII timer that measures wall-clock elapsed time for a code section
/// and logs the duration (in milliseconds) to a wandb run.
///
/// Usage:
///   {
///       TrainingTimer timer(run, "forward_pass_ms");
///       // ... code to measure ...
///   }  // destructor logs elapsed time via run.log()
///
/// Or with manual stop:
///   TrainingTimer timer(run, "training_epoch_ms");
///   // ... code ...
///   timer.stop();  // logs immediately
///   // destructor is a no-op
class TrainingTimer {
public:
  /// Starts the timer. The label is used as the metric key when logging.
  TrainingTimer(Run &run, const std::string &label);

  /// Stops the timer (if still running) and logs elapsed_ms via run.log().
  ~TrainingTimer();

  // Non-copyable, non-movable.
  TrainingTimer(const TrainingTimer &) = delete;
  TrainingTimer &operator=(const TrainingTimer &) = delete;

  /// Returns the elapsed time in milliseconds since construction.
  /// Can be called while the timer is still running.
  double elapsed_ms() const;

  /// Manually stops the timer and logs the duration.
  /// Safe to call multiple times — subsequent calls are no-ops.
  void stop();

private:
  Run &run_;
  std::string label_;
  std::chrono::steady_clock::time_point start_;
  bool stopped_ = false;
};

} // namespace wandb
