#include "wandb_client/metrics.h"

namespace wandb {

TrainingTimer::TrainingTimer(Run &run, const std::string &label)
    : run_(run), label_(label), start_(std::chrono::steady_clock::now()) {}

TrainingTimer::~TrainingTimer() {
  if (!stopped_) {
    try {
      stop();
    } catch (...) {
      // Suppress exceptions in destructor.
    }
  }
}

double TrainingTimer::elapsed_ms() const {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(now - start_).count();
}

void TrainingTimer::stop() {
  if (stopped_) {
    return;
  }

  double ms = elapsed_ms();
  run_.log({{label_, ms}});
  stopped_ = true;
}

} // namespace wandb
