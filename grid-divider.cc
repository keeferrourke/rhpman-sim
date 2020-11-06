#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/mobility-model.h"
#include "ns3/rectangle.h"

#include "grid-divider.h"

namespace rhpman {

ns3::Rectangle GridDivider::asRectangle() const {
    return ns3::Rectangle(
        this->minX()-1, this->maxX()+1, this->minY()-1, this->maxY()+1);
}

std::vector<GridDivider> GridDivider::divideHorizontally(int32_t parts) {
    const double maxX = m_max.first;
    const double minX = m_min.first;

    const double delta = maxX - minX;
    const double size = delta / parts;

    std::vector<GridDivider> dividers(parts);
    double currentMin = minX;
    double currentMax = minX + size;
    for (int i = 0; i < parts; i++) {
        dividers[i] =
            GridDivider(std::pair<double, double>(currentMin, m_min.second),
                std::pair<double, double>(currentMax, m_max.second));
        currentMin = currentMax;
        currentMax += size;
    }
    return dividers;
}

std::vector<GridDivider> GridDivider::divideVertically(int32_t parts) {
    const double maxY = m_max.second;
    const double minY = m_min.second;

    const double delta = maxY - minY;
    const double size = delta / parts;

    std::vector<GridDivider> dividers(parts);
    double currentMin = minY;
    double currentMax = minY + size;
    for (int i = 0; i < parts; i++) {
        dividers[i] =
            GridDivider(std::pair<double, double>(m_min.first, currentMin),
                std::pair<double, double>(m_max.first, currentMax));
        currentMin = currentMax;
        currentMax += size;
    }
    return dividers;
}

std::vector<GridDivider> GridDivider::split(int32_t x, int32_t y) {
    auto strata = this->divideHorizontally(x);
    std::vector<GridDivider> all_grids;
    for (auto stratum : strata) {
        auto grids = stratum.divideVertically(y);
        for (auto grid : grids) {
            all_grids.push_back(grid);
        }
    }
    return all_grids;
}

ns3::Ptr<ns3::GridPositionAllocator> GridDivider::getGridPositionAllocator()
    const {
    auto alloc = ns3::CreateObject<ns3::GridPositionAllocator>();
    std::cout << this->minX() << " : " << this->deltaX() << std::endl;
    std::cout << this->minY() << " : " << this->deltaY() << std::endl;
    alloc->SetMinX(this->minX());
    alloc->SetMinY(this->minY());
    alloc->SetDeltaX(this->deltaX());
    alloc->SetDeltaY(this->deltaY());
    return alloc;
}

ns3::Ptr<ns3::RandomBoxPositionAllocator>
GridDivider::getRandomBoxPositionAllocator() const {
    ns3::Ptr<ns3::UniformRandomVariable> x =
        ns3::CreateObject<ns3::UniformRandomVariable>();
    x->SetAttribute("Min", ns3::DoubleValue(this->minX()));
    x->SetAttribute("Max", ns3::DoubleValue(this->maxX()));

    ns3::Ptr<ns3::UniformRandomVariable> y =
        ns3::CreateObject<ns3::UniformRandomVariable>();
    y->SetAttribute("Min", ns3::DoubleValue(this->minY()));
    y->SetAttribute("Max", ns3::DoubleValue(this->maxY()));

    ns3::Ptr<ns3::ConstantRandomVariable> z =
        ns3::CreateObject<ns3::ConstantRandomVariable>();
    z->SetAttribute("Constant", ns3::DoubleValue(0.0));

    auto alloc = ns3::CreateObject<ns3::RandomBoxPositionAllocator>();
    alloc->SetX(x);
    alloc->SetY(y);
    alloc->SetZ(z);
    return alloc;
}

std::string GridDivider::to_string() const {
    std::stringstream ss;
    ss << "{(" << m_min.first << "," << m_min.second << "),(" << m_max.first
       << "," << m_max.second << ")}";
    return ss.str();
}

}  // namespace rhpman