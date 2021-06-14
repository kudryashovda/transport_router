#define _USE_MATH_DEFINES

#include "geo.h"

#include <cmath>

namespace geo {

    double ComputeDistance(Coordinates from, Coordinates to) {
        using namespace std;

        // if coords are equal, formula returns nan!
        constexpr double EPSILON = 1e-6;
        if (std::abs(from.lat - to.lat) < EPSILON && std::abs(from.lng - to.lng) < EPSILON) {
            return 0.0;
        }

        constexpr double dr = M_PI / 180.0;
        constexpr double earth_radius_meters = 6371000;

        return acos(sin(from.lat * dr) * sin(to.lat * dr) + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr)) * earth_radius_meters;
    }

} // namespace geo