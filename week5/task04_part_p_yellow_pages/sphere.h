#pragma once

#include "functional"

namespace Sphere {
    double ConvertDegreesToRadians(double degrees);

    struct Point {
        double latitude;
        double longitude;

        static Point FromDegrees(double latitude, double longitude);

        bool operator==(const Point& rhs) const {
            return this->latitude == rhs.latitude && this->longitude == rhs.longitude;
        }
    };

    struct PointHash
    {
        size_t operator() (const Point &point) const
        {
            return std::hash<double>()(point.latitude) ^ std::hash<double>()(point.longitude);
        }
    };

    double Distance(Point lhs, Point rhs);
}
