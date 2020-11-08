/// @file grid-divider.cc
/// @author Keefer Rourke (krourke@uoguelph.ca)
/// @brief
///
/// Copyright (c) 2020 by Keefer Rourke <krourke@uoguelph.ca>
/// Permission to use, copy, modify, and/or distribute this software for any
/// purpose with or without fee is hereby granted, provided that the above
/// copyright notice and this permission notice appear in all copies.
///
/// THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES WITH
/// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
/// AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
/// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
/// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
/// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
/// PERFORMANCE OF THIS SOFTWARE.
///
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/mobility-model.h"
#include "ns3/rectangle.h"

#include "simulation-area.h"

namespace rhpman {

ns3::Rectangle SimulationArea::asRectangle() const {
    return ns3::Rectangle(
        this->minX(), this->maxX(), this->minY(), this->maxY());
}

std::vector<SimulationArea> SimulationArea::divideHorizontally(int32_t parts) {
    const double maxX = m_max.first;
    const double minX = m_min.first;

    const double delta = maxX - minX;
    const double size = delta / parts;

    std::vector<SimulationArea> dividers(parts);
    double currentMin = minX;
    double currentMax = minX + size;
    for (int i = 0; i < parts; i++) {
        dividers[i] =
            SimulationArea(std::pair<double, double>(currentMin, m_min.second),
                std::pair<double, double>(currentMax, m_max.second));
        currentMin = currentMax;
        currentMax += size;
    }
    return dividers;
}

std::vector<SimulationArea> SimulationArea::divideVertically(int32_t parts) {
    const double maxY = m_max.second;
    const double minY = m_min.second;

    const double delta = maxY - minY;
    const double size = delta / parts;

    std::vector<SimulationArea> dividers(parts);
    double currentMin = minY;
    double currentMax = minY + size;
    for (int i = 0; i < parts; i++) {
        dividers[i] =
            SimulationArea(std::pair<double, double>(m_min.first, currentMin),
                std::pair<double, double>(m_max.first, currentMax));
        currentMin = currentMax;
        currentMax += size;
    }
    return dividers;
}

std::vector<SimulationArea> SimulationArea::splitIntoGrid(int32_t x, int32_t y) {
    auto strata = this->divideHorizontally(x);
    std::vector<SimulationArea> grid;
    for (auto stratum : strata) {
        auto boxes = stratum.divideVertically(y);
        for (auto box : boxes) {
            grid.push_back(box);
        }
    }
    return grid;
}

ns3::Ptr<ns3::GridPositionAllocator> SimulationArea::getGridPositionAllocator()
    const {
    auto alloc = ns3::CreateObject<ns3::GridPositionAllocator>();
    alloc->SetMinX(this->minX());
    alloc->SetMinY(this->minY());
    alloc->SetDeltaX(this->deltaX());
    alloc->SetDeltaY(this->deltaY());
    return alloc;
}

ns3::Ptr<ns3::RandomRectanglePositionAllocator>
SimulationArea::getRandomRectanglePositionAllocator() const {
    ns3::Ptr<ns3::UniformRandomVariable> x =
        ns3::CreateObject<ns3::UniformRandomVariable>();
    x->SetAttribute("Min", ns3::DoubleValue(this->minX()));
    x->SetAttribute("Max", ns3::DoubleValue(this->maxX()));

    ns3::Ptr<ns3::UniformRandomVariable> y =
        ns3::CreateObject<ns3::UniformRandomVariable>();
    y->SetAttribute("Min", ns3::DoubleValue(this->minY()));
    y->SetAttribute("Max", ns3::DoubleValue(this->maxY()));

    auto alloc = ns3::CreateObject<ns3::RandomRectanglePositionAllocator>();
    alloc->SetX(x);
    alloc->SetY(y);
    return alloc;
}

std::string SimulationArea::toString() const {
    std::stringstream ss;
    ss << "{(" << m_min.first << "," << m_min.second << "),(" << m_max.first
       << "," << m_max.second << ")}";
    return ss.str();
}

}  // namespace rhpman