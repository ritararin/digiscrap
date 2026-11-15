#include "stickers/demo_triangulation.h"

#include <iostream>
#include <vector>

#include "core/framebuffer.h"
#include "animation/timeline.h"
#include "animation/exporter.h"

#include "stickers/sticker.h"
#include "shapes/scrapbook_shapes.h"
#include "polygons/polygons.h"
#include "calendar/calendar_gl.h"
#include "lines/lines.h"

// Palette:

static const Color background {250, 244, 230};

static const Color gold       {240, 190,  70};
static const Color terracotta {200,  95,  75};
static const Color green      {110, 160, 120};

static const Color triA       {245, 190, 100};
static const Color triB       {220, 130, 100};
static const Color triC       {145, 175, 135};

static const Color edge       { 70,  65,  55};

// Draw one triangle:

static void drawTrianglePolygon(
    Framebuffer& fb,
    const Polygon& triangle,
    Color fill
) {
    fillPolygon(
        fb,
        triangle,
        fill
    );

    for (size_t i = 0; i < triangle.size(); i++) {
        Vec2 a = triangle[i];
        Vec2 b = triangle[(i + 1) % triangle.size()];

        drawLineBresenham(
            fb,
            (int)a.x,
            (int)a.y,
            (int)b.x,
            (int)b.y,
            edge
        );
    }
}

// Draw triangulated polygon:

static void drawTriangulated(
    Framebuffer& fb,
    const Polygon& polygon,
    double t
) {
    std::vector<Polygon> triangles =
        triangulateConcave(polygon);

    if (triangles.empty())
        return;

    // Reveal triangles progressively through the animation.
    size_t visible =
        static_cast<size_t>(
            t * triangles.size()
        );

    if (t >= 1.0)
        visible = triangles.size();

    for (size_t i = 0; i < visible; i++) {
        Color c;

        switch (i % 3) {
            case 0:
                c = triA;
                break;

            case 1:
                c = triB;
                break;

            default:
                c = triC;
                break;
        }

        drawTrianglePolygon(
            fb,
            triangles[i],
            c
        );
    }
}

// Demo frame:

static void renderTriangulationDemo(
    Framebuffer& fb,
    double t
) {
    fb.clear(background);

    // LEFT: original scrapbook stickers:

    Polygon speech =
        makeSpeechBubble(
            {170, 150},
            210,
            90
        );

    Polygon burst =
        makeBurstBadge(
            {170, 330},
            72,
            45,
            10
        );

    Polygon ticket =
        makeTicket(
            {170, 500},
            220,
            90
        );


    Sticker speechSticker =
        makeSticker(
            speech,
            {0, 0},
            gold,
            edge,
            1
        );

    Sticker burstSticker =
        makeSticker(
            burst,
            {0, 0},
            terracotta,
            edge,
            2
        );

    Sticker ticketSticker =
        makeSticker(
            ticket,
            {0, 0},
            green,
            edge,
            3
        );


    StickerScene originalScene;

    originalScene.add(speechSticker);
    originalScene.add(burstSticker);
    originalScene.add(ticketSticker);

    originalScene.sortByZ();
    originalScene.render(fb);

    // RIGHT: same shapes triangulated:

    Polygon speechRight =
        makeSpeechBubble(
            {560, 150},
            210,
            90
        );

    Polygon burstRight =
        makeBurstBadge(
            {560, 330},
            72,
            45,
            10
        );

    Polygon ticketRight =
        makeTicket(
            {560, 500},
            220,
            90
        );


    drawTriangulated(
        fb,
        speechRight,
        t
    );

    drawTriangulated(
        fb,
        burstRight,
        t
    );

    drawTriangulated(
        fb,
        ticketRight,
        t
    );
}


// Export demo:

void runTriangulationDemo() {
    Timeline timeline;

    ExportResult result =
        exportAnimation(
            "triangulation_demo",
            timeline,
            800,
            600,
            background,
            renderTriangulationDemo
        );

    if (result.ok) {
        std::cout
            << "Wrote "
            << result.framesWritten
            << " triangulation frames and "
            << result.contactSheetPath
            << "\n";
    }
    else {
        std::cout
            << "FAILED to export triangulation demo\n";
    }
}