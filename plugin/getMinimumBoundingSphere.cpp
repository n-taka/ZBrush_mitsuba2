#ifndef GET_MINIMUM_BOUNDING_SPHERE_CPP
#define GET_MINIMUM_BOUNDING_SPHERE_CPP

#include "getMinimumBoundingSphere.h"

void getMinimumBoundingSphere(
	const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> &V,
	const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> &F,
	Eigen::Matrix<float, 1, Eigen::Dynamic> &center,
	float &radius)
{
	Eigen::Matrix<float, 3, Eigen::Dynamic> XP;
	XP.resize(3, V.cols());
	center.resize(1, V.cols());
	radius = 0.0f;

	const auto updateSphereFromXP = [&XP, &center, &radius]()
	{
		const float a = (XP.row(2) - XP.row(1)).norm();
		const float b = (XP.row(0) - XP.row(2)).norm();
		const float c = (XP.row(1) - XP.row(0)).norm();
		const float cosA = (XP.row(1) - XP.row(0)).dot(XP.row(2) - XP.row(0)) / (b * c);
		const float cosB = (XP.row(0) - XP.row(1)).dot(XP.row(2) - XP.row(1)) / (c * a);
		const float cosC = (XP.row(0) - XP.row(2)).dot(XP.row(1) - XP.row(2)) / (a * b);
		center = (a * cosA * XP.row(0) + b * cosB * XP.row(1) + c * cosC * XP.row(2)) / (a * cosA + b * cosB + c * cosC);
		radius = (center - XP.row(0)).norm();
	};
	// initialize pick first 3 points to form a minimum bounding sphere
	XP.row(0) = V.row(0);
	XP.row(1) = V.row(1);
	XP.row(2) = V.row(2);
	updateSphereFromXP();
	for (int v = 3; v < V.rows(); ++v)
	{
		const float distToCenter = (V.row(v) - center).norm();
		if (distToCenter > radius)
		{
			// remove closest vertex
			float distToClosest = std::numeric_limits<float>::max();
			int closestXPIdx = -1;
			for (int xp = 0; xp < XP.rows(); ++xp)
			{
				const float distToNewVert = (XP.row(xp) - V.row(v)).norm();
				if (distToNewVert < distToClosest)
				{
					distToClosest = distToNewVert;
					closestXPIdx = xp;
				}
			}
			if (closestXPIdx > -1)
			{
				// update XP
				XP.row(closestXPIdx) = V.row(v);
				// update center/radius
				updateSphereFromXP();
			}
		}
	}
}

#endif
