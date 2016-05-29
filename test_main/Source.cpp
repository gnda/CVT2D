#include "..\CVT2D\CVT2D.h"

#include <iostream>
#include <fstream>
#include <random>

using namespace std;

template< typename T >
Line ConstructHalfPlane(T p0, T p1)
{
	auto A = p0.y() - p1.y();
	auto B = p1.x() - p0.x();
	auto C = p0.x()*p1.y() - p1.x()*p0.y();
	return Line(CGAL::to_double(A), CGAL::to_double(B), CGAL::to_double(C));
}

bool LoadBoundary(const char *fileName, Nef_polyhedron &boundaryNef)
{
	ifstream input(fileName);
	if (!input)
	{
		cout << "Cannot open boundary file!" << endl;
		return false;
	}

	// loading boundary information
	TPolygon_2 outLine;
	list< TPolygon_2 > partitionedOuter, partitionedInner;
	list< TPolygon_2 > boundaryOuter, boundaryInner;
	string curLine;
	bool isOuter = true;
	while (getline(input, curLine))
	{
		if (curLine[0] == '#')
		{
			if (!outLine.is_empty())
				if (isOuter) boundaryOuter.push_back(outLine);
				else boundaryInner.push_back(outLine);
				outLine.clear();
				isOuter = curLine == "#Outer";
		}
		else
		{
			double x, y;
			stringstream sin;
			sin << curLine; sin >> x >> y;
			outLine.push_back(TPoint_2(x, y));
		}
	}
	if (!outLine.is_empty())
	{
		if (isOuter) boundaryOuter.push_back(outLine);
		else boundaryInner.push_back(outLine);
	}

	// partitioning...
	cout << "Partitioning..." << endl;
	for (auto &Outer : boundaryOuter)
	{
		Traits partition_traits;
		Validity_traits validity_traits;
		list< TPolygon_2 > curPartition;
		CGAL::optimal_convex_partition_2(Outer.vertices_begin(), Outer.vertices_end(), back_inserter(curPartition), partition_traits);
		assert(CGAL::partition_is_valid_2(Outer.vertices_begin(), Outer.vertices_end(), curPartition.begin(), curPartition.end(), validity_traits));
		partitionedOuter.insert(partitionedOuter.end(), curPartition.begin(), curPartition.end());
	}
	for (auto &Inner : boundaryInner)
	{
		Traits partition_traits;
		Validity_traits validity_traits;
		list< TPolygon_2 > curPartition;
		CGAL::optimal_convex_partition_2(Inner.vertices_begin(), Inner.vertices_end(), back_inserter(curPartition), partition_traits);
		assert(CGAL::partition_is_valid_2(Inner.vertices_begin(), Inner.vertices_end(), curPartition.begin(), curPartition.end(), validity_traits));
		partitionedInner.insert(partitionedInner.end(), curPartition.begin(), curPartition.end());
	}
	cout << "Partition result: " << endl;
	cout << "Outer: " << partitionedOuter.size() << ", Inner: " << partitionedInner.size() << endl;

	boundaryNef = Nef_polyhedron(Nef_polyhedron::EMPTY);
	for (auto &Outer : partitionedOuter)
	{
		auto curPolygon = Nef_polyhedron(Nef_polyhedron::COMPLETE);
		for (auto v = Outer.vertices_begin(); v != Outer.vertices_end(); ++v)
		{
			auto vNext = v; ++vNext;
			if (vNext == Outer.vertices_end()) vNext = Outer.vertices_begin();
			curPolygon *= Nef_polyhedron(ConstructHalfPlane(*v, *vNext));
		}
		boundaryNef += curPolygon;
	}
	for (auto &Inner : partitionedInner)
	{
		auto curPolygon = Nef_polyhedron(Nef_polyhedron::COMPLETE);
		for (auto v = Inner.vertices_begin(); v != Inner.vertices_end(); ++v)
		{
			auto vNext = v; ++vNext;
			if (vNext == Inner.vertices_end()) vNext = Inner.vertices_begin();
			curPolygon *= Nef_polyhedron(ConstructHalfPlane(*v, *vNext));
		}
		boundaryNef -= curPolygon;
	}

	return true;
}

void GenInitGenerators(vector< Point_2 > &generators, int Ng)
{
	default_random_engine gen;
	uniform_real_distribution<double> dist(0.0, 200.0);
	generators.clear();
	for (int i = 0; i < Ng; ++i)
	{
		generators.push_back(Point_2(dist(gen), dist(gen)));
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		cout << "USAGE: [.exe] [.boundary] [#generator]" << endl;
		return -1;
	}

	Nef_polyhedron boundaryNef;
	if (!LoadBoundary(argv[1], boundaryNef))
	{
		cout << "Cannot load boundary file!" << endl;
		return -2;
	}
	int Ng = stoi(argv[2]);
	vector< Point_2 > generators;
	GenInitGenerators(generators, Ng);

	CCVT2D cvt;
	cvt.AssignBoundary(boundaryNef);
	cvt.AssignGeneratorNum(Ng);
	cvt.AssignInitGenerators(generators);

	cvt.SetMaxIteration(1000);
	cvt.SetMinMove(1e-7);

	cvt.Execute();

	ofstream output("finalState.txt");
	cvt.PrintGenerators(output);
	output.close();
	return 0;
}