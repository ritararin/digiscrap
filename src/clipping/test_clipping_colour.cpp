#include "clipping/clipping.h"
#include "colour/colour.h"
#include "polygons/polygons.h"
#include <cmath>
#include <iostream>
#include <random>

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) std::cout << "PASS: " << name << "\n"; \
    else { std::cout << "FAIL: " << name << "\n"; failures++; } } while (0)

static bool near(double first, double second, double eps = 1e-8) {
    return std::fabs(first - second) <= eps;
}

struct Segment { double x0, y0, x1, y1; };
using LineClipper = bool (*)(double&, double&, double&, double&, double, double, double, double);

static bool sameSegment(const Segment& first, const Segment& second) {
    return near(first.x0, second.x0) && near(first.y0, second.y0)
        && near(first.x1, second.x1) && near(first.y1, second.y1);
}

static void lineCases(LineClipper clipper) {
    Segment inside{2, 3, 8, 9}, result = inside;
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, inside), "line fully inside unchanged");
    Segment outside{-4, 2, -1, 8}; result = outside;
    CHECK(!clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, outside), "line fully outside unchanged on rejection");
    result = {-5, 5, 15, 5};
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, {0, 5, 10, 5}), "line crossing window");
    result = {-2, 1, 1, -2};
    CHECK(!clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10),
          "corner miss without shared outside bit");
    result = {-5, 0, 15, 0};
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, {0, 0, 10, 0}), "line on window edge");
    result = {5, 5, 5, 5};
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, {5, 5, 5, 5}), "degenerate point inside");
    result = {-1, -1, -1, -1};
    CHECK(!clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10),
          "degenerate point outside");
    result = {-1, 1, 1, -1};
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, {0, 0, 0, 0}), "corner touch accepted");
    result = {5, -5, 5, 15};
    CHECK(clipper(result.x0, result.y0, result.x1, result.y1, 0, 0, 10, 10)
          && sameSegment(result, {5, 0, 5, 10}), "vertical crossing");
}

static void randomLines() {
    std::mt19937 generator(20261005);
    std::uniform_real_distribution<double> coordinate(-1000.0, 1000.0);
    std::uniform_real_distribution<double> extent(1.0, 500.0);
    bool agree = true;
    for (int sample = 0; sample < 120000; sample++) {
        double xmin = coordinate(generator), ymin = coordinate(generator);
        double xmax = xmin + extent(generator), ymax = ymin + extent(generator);
        Segment original{coordinate(generator), coordinate(generator),
                         coordinate(generator), coordinate(generator)};
        if (sample % 11 == 0) original.x1 = original.x0;
        if (sample % 13 == 0) original.y1 = original.y0;
        Segment cohen = original, liang = original;
        bool acceptedC = clipLine(cohen.x0, cohen.y0, cohen.x1, cohen.y1, xmin, ymin, xmax, ymax);
        bool acceptedL = clipLineLiangBarsky(liang.x0, liang.y0, liang.x1, liang.y1,
                                           xmin, ymin, xmax, ymax);
        bool bounded = !acceptedC || (cohen.x0 >= xmin - 1e-8 && cohen.x0 <= xmax + 1e-8
            && cohen.x1 >= xmin - 1e-8 && cohen.x1 <= xmax + 1e-8
            && cohen.y0 >= ymin - 1e-8 && cohen.y0 <= ymax + 1e-8
            && cohen.y1 >= ymin - 1e-8 && cohen.y1 <= ymax + 1e-8);
        if (acceptedC != acceptedL || !sameSegment(cohen, liang) || !bounded) {
            std::cout << "Random mismatch at sample " << sample << "\n";
            agree = false;
            break;
        }
    }
    CHECK(agree, "120,000 random lines: visibility, endpoints and window bounds");
}

static void polygonCases() {
    Polygon inside{{2, 2}, {8, 2}, {8, 8}, {2, 8}};
    Polygon result = clipPolygon(inside, 0, 0, 10, 10);
    bool unchanged = result.size() == inside.size();
    for (size_t index = 0; unchanged && index < inside.size(); index++)
        unchanged = near(result[index].x, inside[index].x) && near(result[index].y, inside[index].y);
    CHECK(unchanged, "polygon fully inside preserves vertices and order");
    CHECK(clipPolygon({{-4, 2}, {-1, 2}, {-1, 8}, {-4, 8}}, 0, 0, 10, 10).empty(),
          "polygon fully outside is empty");
    result = clipPolygon({{-5, 0}, {5, 0}, {5, 10}, {-5, 10}}, 0, 0, 10, 10);
    CHECK(polygonArea(result) == 50.0, "half-outside square has exact area 50");
    result = clipPolygon({{-5, -5}, {15, -5}, {15, 15}, {-5, 15}}, 0, 0, 10, 10);
    CHECK(polygonArea(result) == 100.0, "polygon crosses all four boundaries");
    std::reverse(inside.begin(), inside.end());
    CHECK(polygonArea(clipPolygon(inside, 0, 0, 10, 10)) == 36.0,
          "clockwise polygon also clips correctly");
    CHECK(clipPolygon({}, 0, 0, 10, 10).empty(), "empty polygon");
    result = clipPolygon(makeStar({9, 9}, 5, 2), 0, 0, 10, 10);
    bool bounded = !result.empty();
    for (Vec2 vertex : result)
        bounded = bounded && vertex.x >= 0 && vertex.x <= 10 && vertex.y >= 0 && vertex.y <= 10;
    CHECK(bounded, "clipped concave star vertices stay within window");
}

static bool sameColour(Color first, Color second) {
    return first.r == second.r && first.g == second.g && first.b == second.b && first.a == second.a;
}

static void colourCases() {
    Color original{17, 125, 240, 73};
    Color gray = grayscale(original);
    CHECK(gray.r == gray.g && gray.g == gray.b && gray.a == original.a,
        "grayscale equal channels and preserved alpha");
    CHECK(grayscale({255, 0, 0}).r == 76, "grayscale uses weighted luminance");
    Color target{210, 80, 20, 9};
    CHECK(sameColour(tint(original, target, 0), original), "tint strength zero is identity");
    CHECK(sameColour(tint(original, target, 1), {210, 80, 20, 73}),
        "tint strength one replaces RGB, not alpha");
    CHECK(sameColour(tint(original, target, -1), original)
        && sameColour(tint(original, target, 2), {210, 80, 20, 73}), "tint strength clamped");
    CHECK(sameColour(sepia({255, 255, 255, 73}), {255, 255, 239, 73}),
        "sepia matrix clamps bright channels at 255");
    CHECK(sameColour(brightness(original, 300), {255, 255, 255, 73})
        && sameColour(brightness(original, -300), {0, 0, 0, 73}),
        "brightness clamps at both ends");
    CHECK(sameColour(contrast(original, 1), original), "contrast factor one is identity");
    CHECK(sameColour(contrast(original, 0), {128, 128, 128, 73})
        && sameColour(contrast({0, 128, 255, 73}, 3), {0, 128, 255, 73}),
        "contrast midpoint and clamping");
    bool invertIdentity = true;
    for (int value = 0; value < 256; value++) {
      Color sample{static_cast<uint8_t>(value), static_cast<uint8_t>(255 - value), 37, 73};
      invertIdentity = invertIdentity && sameColour(invert(invert(sample)), sample);
    }
    CHECK(invertIdentity, "invert twice is identity for every channel value");
    CHECK(sameColour(alphaBlend({200, 40, 10, 0}, original), original), "transparent source is identity");
    CHECK(sameColour(alphaBlend(target, {99, 88, 77, 0}), target),
        "source over transparent destination preserves straight RGBA");
    CHECK(sameColour(alphaBlend({255, 0, 0, 128}, {0, 0, 255}), {128, 0, 127}),
        "source-over on opaque destination");
    CHECK(sameColour(alphaBlend({1, 2, 3}, original), {1, 2, 3}), "opaque source replaces destination");
    CHECK(sameColour(alphaBlend({255, 0, 0, 128}, {0, 0, 255, 128}), {170, 0, 85, 192}),
        "source-over with two translucent colours");

    Framebuffer image(4, 3, original);
    applyFilter(image, -5, -5, 2, 2, invert);
    bool regionCorrect = true;
    for (int y = 0; y < image.height; y++)
      for (int x = 0; x < image.width; x++)
        regionCorrect = regionCorrect && sameColour(image.pixels[y * image.width + x],
            x < 2 && y < 2 ? invert(original) : original);
    CHECK(regionCorrect, "filter rectangle is clamped and half-open");
    std::vector<Color> snapshot = image.pixels;
    applyFilter(image, 10, 10, 20, 20, invert);
    applyFilter(image, 3, 2, 1, 1, invert);
    applyFilter(image, ColourFilter{});
    bool unchanged = true;
    for (size_t index = 0; index < snapshot.size(); index++)
      unchanged = unchanged && sameColour(snapshot[index], image.pixels[index]);
    CHECK(unchanged, "outside/reversed regions and empty callback do nothing");
    applyFilter(image, grayscale);
    bool wholeImage = true;
    for (Color pixel : image.pixels)
      wholeImage = wholeImage && pixel.r == pixel.g && pixel.g == pixel.b && pixel.a == original.a;
    CHECK(wholeImage, "whole-image filter reaches every pixel and preserves alpha");
}

int main() {
    std::cout << "Cohen-Sutherland\n";
    lineCases(clipLine);
    std::cout << "Liang-Barsky\n";
    lineCases(clipLineLiangBarsky);
    randomLines();
    polygonCases();
    colourCases();
    std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}