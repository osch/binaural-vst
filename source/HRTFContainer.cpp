#pragma warning(push)
#include "delaunay/delaunay.h"
#include "HRTFContainer.h"


HRTFContainer::HRTFContainer()
{
	jassert(hrirReadIndex.is_lock_free());
	hrirReadIndex = 0;
	hrir_[0] = {};
	hrir_[1] = {};
}

HRTFContainer::~HRTFContainer()
{
}

void HRTFContainer::updateHRIR(double azimuth, double elevation)
{
	if (!triangulation_)
		return;

	const auto& triangles = triangulation_->getTriangles();
	// Iterate through all the faces of the triangulation
 	for (auto& triangle : triangles)
	{
		const auto A = triangle.p1;
		const auto B = triangle.p2;
		const auto C = triangle.p3;

		const double T[] = {A.x - C.x, A.y - C.y, B.x - C.x, B.y - C.y};
		double invT[] = {T[3], -T[1], -T[2], T[0]};
		const auto det = T[0] * T[3] - T[1] * T[2];
		jassert(det != 0 && "Bad triangulation!");
		for (auto i = 0; i < 4; ++i)
			invT[i] /= det;
		const double X[] = {azimuth - C.x, elevation - C.y};

		// Barycentric coordinates of point X
		const auto g1 = static_cast<float>(invT[0] * X[0] + invT[2] * X[1]);
		const auto g2 = static_cast<float>(invT[1] * X[0] + invT[3] * X[1]);
		const auto g3 = 1 - g1 - g2;

		// If any of the barycentric coordinate is negative, the point
		// does not lay inside the triangle, so continue the loop.
		if (g1 < 0 || g2 < 0 || g3 < 0)
			continue;
		auto& irA = hrirDict_[static_cast<int>(A.x)][getElvIndex(std::lround(A.y))];
		auto& irB = hrirDict_[static_cast<int>(B.x)][getElvIndex(std::lround(B.y))];
		auto& irC = hrirDict_[static_cast<int>(C.x)][getElvIndex(std::lround(C.y))];
		const auto hrirWriteIndex = hrirReadIndex ^ 1;
		auto& hrirBuffer = hrir_[hrirWriteIndex];
		for (auto i = 0u; i < hrirBuffer.HRIR_SIZE; ++i)
		{
			hrirBuffer.leftEarIR[i] = g1 * irA.leftEarIR[i] + g2 * irB.leftEarIR[i] + g3 * irC.leftEarIR[i];
			hrirBuffer.rightEarIR[i] = g1 * irA.rightEarIR[i] + g2 * irB.rightEarIR[i] + g3 * irC.rightEarIR[i];
		}
		hrirReadIndex = hrirWriteIndex;
		return;
	}
}

const HRIRBuffer& HRTFContainer::hrir() const
{
	return hrir_[hrirReadIndex];
}

void HRTFContainer::loadHrir(InputStream& istream)
{
	{
		std::vector<Vec2f> points;
		int azimuths[] = {-90, -80, -65, -55, -45, -40, -35, -30, -25, -20,
			-15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 65, 80, 90};
		for (auto& azm : azimuths)
		{
			hrirDict_.insert(std::make_pair(azm, std::array<HRIRBuffer, 52>()));
			// -90 deg
			istream.read(hrirDict_[azm][0].leftEarIR.data(), 200 * sizeof(float));
			istream.read(hrirDict_[azm][0].rightEarIR.data(), 200 * sizeof(float));
			points.push_back({static_cast<float>(azm), -90.f});
			// 50 elevations
			for (int i = 1; i < 51; ++i)
			{
				istream.read(hrirDict_[azm][i].leftEarIR.data(), 200 * sizeof(float));
				istream.read(hrirDict_[azm][i].rightEarIR.data(), 200 * sizeof(float));
				points.push_back({static_cast<float>(azm), -45.f + 5.625f * (i - 1)});
			}
			// 270 deg
			istream.read(hrirDict_[azm][51].leftEarIR.data(), 200 * sizeof(float));
			istream.read(hrirDict_[azm][51].rightEarIR.data(), 200 * sizeof(float));
			points.push_back({static_cast<float>(azm), 270});
		}
		triangulation_ = new Delaunay();
		triangulation_->triangulate(points);
	}
}

int HRTFContainer::getElvIndex(int elv)
{
	if (elv == -90)
		return 0;
	else if (elv == 270)
		return 51;
	else
		return std::lroundf((elv + 45) / 5.625f);
}
