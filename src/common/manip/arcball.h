// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#ifndef VSNRAY_COMMON_MANIP_ARCBALL_H
#define VSNRAY_COMMON_MANIP_ARCBALL_H 1

#include <visionaray/math/math.h>

namespace visionaray
{

class arcball
{
public:

    arcball();
    explicit arcball(float r);

    vec3 project(int x, int y, recti const& viewport);

    float radius;
    vec3 down_pos;
    quat rotation;
    quat down_rotation;

};

} // visionaray

#endif // VSNRAY_COMMON_MANIP_ARCBALL_H
