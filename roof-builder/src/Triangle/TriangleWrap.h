#pragma once
#define _CRT_SECURE_NO_WARNINGS


#include <iostream>
#include <vector>
#include <list>
#include "../MyPoint.h"
extern "C" {
	#include "triangle.h"
}
/*
void report(triangulateio* io, int markers, int reporttriangles, int reportneighbors, int reportsegments,
	int reportedges, int reportnorms) {
	int i, j;

	for (i = 0; i < io->numberofpoints; i++) {
		printf("Point %4d:", i);
		for (j = 0; j < 2; j++) {
			printf("  %.6g", io->pointlist[i * 2 + j]);
		}
		if (io->numberofpointattributes > 0) {
			printf("   attributes");
		}
		for (j = 0; j < io->numberofpointattributes; j++) {
			printf("  %.6g",
				io->pointattributelist[i * io->numberofpointattributes + j]);
		}
		if (markers) {
			printf("   marker %d\n", io->pointmarkerlist[i]);
		}
		else {
			printf("\n");
		}
	}
	printf("\n");

	if (reporttriangles || reportneighbors) {
		for (i = 0; i < io->numberoftriangles; i++) {
			if (reporttriangles) {
				printf("Triangle %4d points:", i);
				for (j = 0; j < io->numberofcorners; j++) {
					printf("  %4d", io->trianglelist[i * io->numberofcorners + j]);
				}
				if (io->numberoftriangleattributes > 0) {
					printf("   attributes");
				}
				for (j = 0; j < io->numberoftriangleattributes; j++) {
					printf("  %.6g", io->triangleattributelist[i *
						io->numberoftriangleattributes + j]);
				}
				printf("\n");
			}
			if (reportneighbors) {
				printf("Triangle %4d neighbors:", i);
				for (j = 0; j < 3; j++) {
					printf("  %4d", io->neighborlist[i * 3 + j]);
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	if (reportsegments) {
		for (i = 0; i < io->numberofsegments; i++) {
			printf("Segment %4d points:", i);
			for (j = 0; j < 2; j++) {
				printf("  %4d", io->segmentlist[i * 2 + j]);
			}
			if (markers) {
				printf("   marker %d\n", io->segmentmarkerlist[i]);
			}
			else {
				printf("\n");
			}
		}
		printf("\n");
	}

	if (reportedges) {
		for (i = 0; i < io->numberofedges; i++) {
			printf("Edge %4d points:", i);
			for (j = 0; j < 2; j++) {
				printf("  %4d", io->edgelist[i * 2 + j]);
			}
			if (reportnorms && (io->edgelist[i * 2 + 1] == -1)) {
				for (j = 0; j < 2; j++) {
					printf("  %.6g", io->normlist[i * 2 + j]);
				}
			}
			if (markers) {
				printf("   marker %d\n", io->edgemarkerlist[i]);
			}
			else {
				printf("\n");
			}
		}
		printf("\n");
	}
}
*/
typedef triangulateio TriangleIO;

class TriangleWrapper {
public:
	TriangleWrapper();
	~TriangleWrapper();
	void Initialize();

	void UploadPoints(const std::vector<MyPoint2>& constrainedPoints, const std::vector<MyPoint2>& other = {}, const std::vector<std::pair<int, int>>& constrainedIndexes = {});
	std::vector<MyTriangle2> Triangulate();
	std::vector<MyTriangle2> TriangulateConstrained();

	std::vector<std::vector<int>> GetIndices() { return m_Indices; };
private:
	void deallocateStructs();

private:
	TriangleIO m_In;
	TriangleIO m_Out;
	std::vector<MyTriangle2> m_Triangles;
	std::vector<std::vector<int>> m_Indices;
};


class UtilsTriangle {
private:
	static int FindNextVert(std::list<std::pair<int, int>>& v, int num) {
		for (auto it = v.begin(); it != v.end(); /* no increment here */) {
			//std::cout << "searching " << num << std::endl;
			//std::cout << "current: " << it->first << " - " << it->second << std::endl;
			if (it->first == num) {
				int temp = it->second;
				v.erase(it);
				return temp;
			}
			else if (it->second == num) {
				int temp = it->first;
				it = v.erase(it);
				return temp;
			}
			else {
				++it; // only increment if we didn't erase
			}
		}

		return -1;
	}

public:
	static int FindPrimaryVert(std::list<std::pair<int, int>>& v, int num, int t) {
		int pos = num;
		bool found = false;

		for (auto it = v.begin(); !found && it != v.end(); /* no increment here */) {
			//std::cout << "searching " << pos << std::endl;
			//std::cout << "current: " << it->first << " - " << it->second << std::endl;
			if (it->first == pos) {
				//std::cout << "found " << pos << " with " << it->second << std::endl;
				pos = it->second;

				// remove this entry from v
				it = v.erase(it);
				if (pos < t) {
					found = true;
					return pos;
				}
			}
			else if (it->second == pos) {
				//std::cout << "found " << pos << " with " << it->first << std::endl;
				pos = it->first;

				// remove this entry from v
				it = v.erase(it);
				if (pos < t) {
					found = true;
					return pos;
				}
			}
			else {
				++it; // only increment if we didn't erase
			}

			if (it == v.end()) {
				it = v.begin();
			}
		}

		return -1;
	}

	
	static std::vector<std::pair<int, int>> Polygonize(std::list<std::pair<int, int>>& list) {
		std::vector<std::pair<int, int>> res;
		int init = list.front().first;
		int temp = init;
		while (!list.empty()) { // no increment here
			int sec = FindNextVert(list, temp);
			res.push_back({ temp, sec });
			temp = sec;
			if (sec == init) { // there are some dirty edges
				return res;
			}
		}

		return res;
	}
};
