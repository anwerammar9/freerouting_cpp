#pragma once

// Deprecated: the former C++ RouterScoringSettings is Java's RoutingCostSettings. Include
// settings/RoutingCostSettings.h and use RoutingCostSettings directly; this alias only remains so
// the file can be removed without touching unrelated drafts.

#include "settings/RoutingCostSettings.h"

namespace freerouting::settings {

using RouterScoringSettings = RoutingCostSettings;

} // namespace freerouting::settings
