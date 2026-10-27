#include <math.h>
#include <unity.h>

#include "battery_curve.h"
#include "tank_geometry.h"

void setUp() {}
void tearDown() {}

namespace {

tank::Config base(tank::Shape shape) {
    tank::Config c{};
    c.shape = shape;
    c.heightCm = 150.0f;
    c.lengthCm = 200.0f;
    c.widthCm = 100.0f;
    c.diameterCm = 100.0f;
    c.count = 1;
    c.sensorOffsetCm = 0.0f;
    return c;
}

}  // namespace

void test_rectangular() {
    tank::Config c = base(tank::Shape::Rectangular);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3000.0f, tank::capacityLiters(c));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1000.0f, tank::litersAtHeight(c, 50.0f));
}

void test_vertical_cylinder() {
    tank::Config c = base(tank::Shape::VerticalCylinder);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1178.1f, tank::capacityLiters(c));  // pi * 50^2 * 150
}

void test_horizontal_cylinder() {
    tank::Config c = base(tank::Shape::HorizontalCylinder);
    const float full = 3.14159265f * 50.0f * 50.0f * 200.0f / 1000.0f;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, full, tank::capacityLiters(c));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, full / 2.0f, tank::litersAtHeight(c, 50.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, tank::litersAtHeight(c, 0.0f));
    // A lying cylinder holds less than a quarter in its lowest quarter of height.
    TEST_ASSERT_TRUE(tank::litersAtHeight(c, 25.0f) < full / 4.0f);
}

void test_table() {
    static const tank::TablePoint table[] = {{10.0f, 100.0f}, {100.0f, 1000.0f}};
    tank::Config c = base(tank::Shape::Table);
    c.table = table;
    c.tableSize = 2;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, tank::innerHeightCm(c));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, tank::litersAtHeight(c, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 550.0f, tank::litersAtHeight(c, 55.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, tank::litersAtHeight(c, 120.0f));
}

void test_level_from_distance_with_offset_and_count() {
    tank::Config c = base(tank::Shape::Rectangular);
    c.sensorOffsetCm = 30.0f;
    c.count = 2;
    tank::Level l = tank::levelFromDistance(c, 80.0f);  // 150 + 30 - 80 = 100 cm
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, l.fillHeightCm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 4000.0f, l.liters);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 6000.0f, l.capacityLiters);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 66.67f, l.percent);

    // Clamped at empty and full.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, tank::levelFromDistance(c, 500.0f).liters);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, tank::levelFromDistance(c, 10.0f).percent);
}

void test_battery_curve() {
    TEST_ASSERT_EQUAL_FLOAT(100.0f, battery::lipoPercent(4.25f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, battery::lipoPercent(3.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, battery::lipoPercent(3.83f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 95.0f, battery::lipoPercent(4.15f));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rectangular);
    RUN_TEST(test_vertical_cylinder);
    RUN_TEST(test_horizontal_cylinder);
    RUN_TEST(test_table);
    RUN_TEST(test_level_from_distance_with_offset_and_count);
    RUN_TEST(test_battery_curve);
    return UNITY_END();
}
