#ifndef __grid_divider_h
#define __grid_divider_h

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ns3/rectangle.h"
#include "ns3/position-allocator.h"

namespace rhpman {

/// A helper class to divide out 2D cartesian grids into subgrids.
/// Coordinates are represented by std::pairs of doubles arranged by (x,y).
///
/// This is intended to help create `ns3::GridPositionAllocator`s.
class GridDivider {
   public:
    GridDivider() {}

    GridDivider(std::pair<double, double> min, std::pair<double, double> max)
        : m_min(min), m_max(max) {}

    double minX() const { return m_min.first; }
    double maxX() const { return m_max.first; }
    double minY() const { return m_min.second; }
    double maxY() const { return m_max.second; }
    double deltaX() const { return m_max.first - m_min.first; }
    double deltaY() const { return m_max.second - m_min.second; }
    void setMin(std::pair<double, double> value) { m_min = value; }
    void setMax(std::pair<double, double> value) { m_max = value; }

    ns3::Rectangle asRectangle() const;

    std::vector<GridDivider> divideHorizontally(int32_t parts);
    std::vector<GridDivider> divideVertically(int32_t parts);
    std::vector<GridDivider> split(int32_t x, int32_t y);

    /// Get a Grid Position Allocator which is compatible with constant position mobility models.
    ns3::Ptr<ns3::GridPositionAllocator> getGridPositionAllocator() const;
    /// Get a Random Box Position Allocation which is compatible with random walk mobility models.
    ns3::Ptr<ns3::RandomBoxPositionAllocator> getRandomBoxPositionAllocator() const;

    std::string to_string() const;

    friend std::ostream& operator<<(std::ostream& os, const GridDivider& gd) {
        return os << gd.to_string();
    }

   private:
    std::pair<double, double> m_min;
    std::pair<double, double> m_max;
};

}  // namespace rhpman

#endif