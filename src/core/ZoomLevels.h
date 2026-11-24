#pragma once

#include <algorithm>
#include <array>

namespace thatsapp {

inline constexpr std::array<double, 7> kZoomLevels = {0.8, 0.9, 1.0, 1.1, 1.25, 1.35, 1.5};
inline constexpr double kDefaultZoom = 1.0;

inline double clampZoom(double zoom)
{
    return std::clamp(zoom, kZoomLevels.front(), kZoomLevels.back());
}

inline double nextZoomIn(double current)
{
    for (double factor : kZoomLevels) {
        if (factor > current + 0.001) {
            return factor;
        }
    }
    return kZoomLevels.back();
}

inline double nextZoomOut(double current)
{
    for (auto it = kZoomLevels.rbegin(); it != kZoomLevels.rend(); ++it) {
        if (*it < current - 0.001) {
            return *it;
        }
    }
    return kZoomLevels.front();
}

} // namespace thatsapp
