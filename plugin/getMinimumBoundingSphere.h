#ifndef GET_MINIMUM_BOUNDING_SPHERE_H
#define GET_MINIMUM_BOUNDING_SPHERE_H

#include "Eigen/Core"

void getMinimumBoundingSphere(
    const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> &V,
    const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> &F,
    Eigen::Matrix<float, 1, Eigen::Dynamic> &center,
    float &radius);

#endif
