#pragma once

#include "instrumentgraph.hh"

#include <memory>
#include <vector>

using Instruments = std::vector<std::unique_ptr<InstrumentGraph>>;
