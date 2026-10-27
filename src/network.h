#pragma once

namespace net {

// Connect to WiFi. Uses the BSSID/channel cached in RTC memory from the previous
// wakeup to skip the channel scan, falls back to a normal connect if that fails.
bool connectWifi();

// Disconnect and switch the radio off.
void shutdown();

}  // namespace net
