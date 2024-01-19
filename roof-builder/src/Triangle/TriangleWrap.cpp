#include "TriangleWrap.h"

TriangleWrapper::TriangleWrapper() {}

TriangleWrapper::~TriangleWrapper() {}

void TriangleWrapper::Initialize() {
	m_Out.pointlist = (REAL*)NULL;
	m_Out.pointattributelist = (REAL*)NULL;
	m_Out.pointmarkerlist = (int*)NULL;
	m_Out.trianglelist = (int*)NULL;
	m_Out.triangleattributelist = (REAL*)NULL;
	m_Out.neighborlist = (int*)NULL;
	m_Out.segmentlist = (int*)NULL;
	m_Out.segmentmarkerlist = (int*)NULL;
	m_Out.edgelist = (int*)NULL;
	m_Out.edgemarkerlist = (int*)NULL;
}

void TriangleWrapper::UploadPoints(const std::vector<MyPoint2>& constrainedPoints, const std::vector<MyPoint2>& other) {
	m_In.numberofpoints = constrainedPoints.size() + other.size();
	m_In.numberofsegments = constrainedPoints.size();
	m_In.numberofpointattributes = 0;
	m_In.numberofholes = 0;
	m_In.numberofregions = 0;
	m_In.regionlist = (REAL*)NULL;
	m_In.pointattributelist = (REAL*)NULL;
	m_In.pointlist = (REAL*)malloc(m_In.numberofpoints * 2 * sizeof(REAL));
	m_In.pointmarkerlist = (int*)malloc(m_In.numberofpoints * sizeof(int));
	m_In.segmentlist = (int*)malloc(m_In.numberofsegments * 2 * sizeof(int));
	m_In.segmentmarkerlist = (int*)malloc(m_In.numberofsegments * 2 * sizeof(int));

	// Upload constrained Points
	for (int i = 0; i < constrainedPoints.size(); i++) {
		m_In.pointlist[i * 2] = constrainedPoints.at(i).x;
		m_In.pointlist[i * 2 + 1] = constrainedPoints.at(i).y;
	}

	// Upload interior Points
	for (int i = constrainedPoints.size(), j = 0; i < m_In.numberofpoints; i++, j++) {
		m_In.pointlist[i * 2] = other.at(j).x;
		m_In.pointlist[i * 2 + 1] = other.at(j).y;
	}


	// Define markers
	for (int i = 0; i < constrainedPoints.size(); i++)
		m_In.pointmarkerlist[i] = 2;

	for (int i = constrainedPoints.size(); i < m_In.numberofpoints; i++)
		m_In.pointmarkerlist[i] = 3;

	// Upload segments for outer border (constraints)
	for (int i = 1; i <= m_In.numberofsegments * 2; i++) {
		if (i == m_In.numberofsegments * 2)
			m_In.segmentlist[i - 1] = 0;
		else
			m_In.segmentlist[i - 1] = i / 2;
	}

	for (int i = 0; i < m_In.numberofsegments; i++) {
		m_In.segmentmarkerlist[i] = 2;
	}
}

std::vector<MyTriangle2> TriangleWrapper::Triangulate() {
	char cmd[] = "pz";
	triangulate(cmd, &m_In, &m_Out, (struct triangulateio*)NULL);

	std::vector<MyTriangle2> triangles;

	for (int i = 0; i < m_Out.numberoftriangles; i++) {
		int v1 = m_Out.trianglelist[i * 3];
		int v2 = m_Out.trianglelist[i * 3 + 1];
		int v3 = m_Out.trianglelist[i * 3 + 2];

		float ax = m_Out.pointlist[2 * v1];
		float ay = m_Out.pointlist[2 * v1 + 1];
		float bx = m_Out.pointlist[2 * v2];
		float by = m_Out.pointlist[2 * v2 + 1];
		float cx = m_Out.pointlist[2 * v3];
		float cy = m_Out.pointlist[2 * v3 + 1];

		triangles.push_back({ {ax, ay}, {bx, by}, {cx, cy} });
	}

	deallocateStructs();

	return triangles;
}

void TriangleWrapper::deallocateStructs() {
	free(m_In.pointlist);
	free(m_In.pointmarkerlist);
	free(m_In.regionlist);
	free(m_Out.pointlist);
	free(m_Out.pointattributelist);
	free(m_Out.pointmarkerlist);
	free(m_Out.trianglelist);
	free(m_Out.triangleattributelist);
	free(m_Out.neighborlist);
	free(m_Out.segmentlist);
	free(m_Out.segmentmarkerlist);
	free(m_Out.edgelist);
	free(m_Out.edgemarkerlist);
}
