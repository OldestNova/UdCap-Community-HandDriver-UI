#pragma once

#include <atomic>

namespace AppRestart {
inline std::atomic_bool requested{false};

inline void request() { requested.store(true); }
inline bool consume() { return requested.exchange(false); }
}
