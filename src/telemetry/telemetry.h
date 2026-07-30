// telemetry.h — BLE/WiFi streaming interface. STUB until phase 4.
//
// The interface is fixed now so task/logging code can call it without
// caring which transport lands. Phase 4 decides BLE GATT vs WiFi AP stream
// (record the choice in docs/decisions.md) and implements behind this API.

#pragma once

#include "../state/sensor_state.h"

namespace telemetry {

inline void init() { /* phase 4 */ }

// Called by the logger cadence (~1 Hz) with the latest snapshot.
inline void publish(const SensorSnapshot&) { /* phase 4 */ }

inline bool client_connected() { return false; }

} // namespace telemetry
