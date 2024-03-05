#pragma once
#include <vector>
#include "MyPoint.h"
#include "Writer.h"

class Exporter {
public:
    static int ExportStl(MyMesh mesh, std::string& fileName) {
        std::ostringstream os;
        os.imbue(std::locale::classic());
        os << std::fixed << std::setprecision(3);
        os << "solid\n";
        for (const auto& t : mesh.triangles) {
            os << "outer loop\n";
            os << "vertex " << t.p1 << "\n";
            os << "vertex " << t.p2 << "\n";
            os << "vertex " << t.p3 << "\n";
            os << "endloop\n";
        }
        os << "endsolid\n";

        return Writer::Write(fileName, os.str());
    }

    static int ExportStl(std::vector<MyMesh> meshes, std::string& fileName) {
        std::ostringstream os;
        os.imbue(std::locale::classic());
        os << std::fixed << std::setprecision(3);
        for (MyMesh m : meshes) {
            os << "solid\n";
            for (const auto& t : m.triangles) {
                os << "outer loop\n";
                os << "vertex " << t.p1 << "\n";
                os << "vertex " << t.p2 << "\n";
                os << "vertex " << t.p3 << "\n";
                os << "endloop\n";
            }
            os << "endsolid\n";
        }

        return Writer::Write(fileName, os.str());
    }
};