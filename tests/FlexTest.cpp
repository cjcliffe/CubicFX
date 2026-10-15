#include <cmath>
#include <cstdio>

#include "ui/Flex.h"

static int failures = 0;

static void check(bool ok, const char *name) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", name);
    } else {
        std::printf("ok %s\n", name);
    }
}

static bool feq(float a, float b) {
    return std::fabs(a - b) < 0.001f;
}

int main() {
    {
        Flex col;
        col.dir = FlexDir::Column;
        col.bounds = Rectangle{ 16, 12, 628, 612 };
        col.gap = 6;
        col.addFixed(34);
        col.addFixed(24);
        col.addFixed(144);
        col.layout();
        check(feq(col.at(0).x, 16) && feq(col.at(0).y, 12) && feq(col.at(0).width, 628) && feq(col.at(0).height, 34), "col fixed 0");
        check(feq(col.at(1).y, 52) && feq(col.at(1).height, 24), "col fixed 1");
        check(feq(col.at(2).y, 82) && feq(col.at(2).height, 144), "col fixed 2");
        check(feq(col.cursor(), 82 + 144 + 6), "col cursor");
    }
    {
        Flex col;
        col.dir = FlexDir::Column;
        col.bounds = Rectangle{ 0, 0, 100, 120 };
        col.gap = 10;
        col.addFixed(20);
        col.addGrow(1);
        col.addGrow(3);
        col.layout();
        check(feq(col.at(0).height, 20), "grow fixed part");
        check(feq(col.at(1).height, 20) && feq(col.at(2).height, 60), "grow split 1:3");
        check(feq(col.at(1).y, 30) && feq(col.at(2).y, 60), "grow positions");
    }
    {
        Flex row;
        row.dir = FlexDir::Row;
        row.bounds = Rectangle{ 16, 0, 628, 30 };
        row.gap = 8;
        row.addFixed(310);
        row.addFixed(310);
        row.layout();
        check(feq(row.at(0).x, 16) && feq(row.at(0).width, 310), "row 0");
        check(feq(row.at(1).x, 334) && feq(row.at(1).width, 310), "row 1");
        check(feq(row.at(0).height, 30), "row full height");
    }
    {
        Flex col;
        col.dir = FlexDir::Column;
        col.bounds = Rectangle{ 0, 0, 100, 0 };
        col.gap = 6;
        col.addFixed(10);
        col.addFixed(20);
        col.layout();
        check(feq(col.at(0).y, 0) && feq(col.at(1).y, 16), "auto height stack");
    }
    {
        Flex row;
        row.dir = FlexDir::Row;
        row.bounds = Rectangle{ 0, 0, 200, 40 };
        row.gap = 0;
        row.padX = 10;
        row.padY = 5;
        row.addFixed(50);
        row.addGrow(1);
        row.layout();
        check(feq(row.at(0).x, 10) && feq(row.at(0).width, 50) && feq(row.at(0).height, 30), "pad fixed");
        check(feq(row.at(1).x, 60) && feq(row.at(1).width, 130), "pad grow");
    }
    if (failures == 0) {
        std::printf("ALL PASS\n");
        return 0;
    }
    std::printf("%d FAILURES\n", failures);
    return 1;
}
