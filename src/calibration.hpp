#pragma once

#include <string>
#include <vector>
#include "interceptor/phy_listener.hpp"

namespace wave_native {
namespace core {

class CalibrationMode {
public:
    CalibrationMode() = default;

    // Run the non-linear frequency sweep and return optimal frequency.
    // Also captures optimal physics parameters if desired.
    bool run_sweep(long double& out_omega);

    // Persistence Layer
    static bool save_profile(const std::string& path, long double omega, long double alpha, long double beta, long double delta);
    static bool load_profile(const std::string& path, long double& omega, long double& alpha, long double& beta, long double& delta);
};

} // namespace core
} // namespace wave_native
