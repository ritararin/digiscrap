#include <iostream>
#include <string>
#include <filesystem>

#include "core/framebuffer.h"
#include "core/mat3.h"
#include "core/scene.h"
#include "core/font.h"

#include "lines/lines.h"
#include "polygons/polygons.h"
#include "clipping/clipping.h"
#include "colour/colour.h"
#include "circles/circles.h"
#include "curves/curves.h"
#include "animation/demo_sequences.h"
#include "stickers/demo_drag.h"
#include "opengl/window.h"
#include "opengl/shader.h"
#include "opengl/renderer.h"
#include "opengl/texture_renderer.h"
#include <ctime>
#include <cstdio>
#include "textures/image_loader.h"
#include "textures/font_atlas.h"
#include "calendar/calendar_gl.h"


// ---------- shared palette (taken from the project deck) ----------
namespace pal {
    const Color brown      {154, 106,  67};
    const Color dark       { 77,  74,  63};
    const Color cream      {249, 228, 192};
    const Color paper      {252, 241, 217};
    const Color white      {255, 252, 245};
    const Color mustard    {228, 165,  69};
    const Color honey      {249, 205, 125};
    const Color green      {110, 145, 121};
    const Color deepGreen  { 84, 118,  96};
    const Color sage       {173, 170, 134};
    const Color terracotta {196,  92,  70};
    const Color shadow     {214, 186, 140};   // soft shadow on cream paper
    const Color tapeA      {228, 165,  69, 200};
    const Color tapeB      {110, 145, 121, 200};
}

static const bool kDebug = false;   // true = show selection boxes and clip window


// ---------- helpers ----------

static Polygon rectPoly(double x0, double y0, double x1, double y1) {
    return {
        {x0, y0},
        {x1, y0},
        {x1, y1},
        {x0, y1}
    };
}


static void fillRect(
    Framebuffer& f,
    double x0,
    double y0,
    double x1,
    double y1,
    Color c
) {
    fillPolygon(f, rectPoly(x0, y0, x1, y1), c);
}


static void drawOutline(
    Framebuffer& f,
    const Polygon& p,
    Color c
) {
    for (size_t i = 0; i < p.size(); i++) {
        Vec2 a = p[i];
        Vec2 b = p[(i + 1) % p.size()];

        drawLineBresenham(
            f,
            (int)a.x,
            (int)a.y,
            (int)b.x,
            (int)b.y,
            c
        );
    }
}


static void drawRect(
    Framebuffer& f,
    int x0,
    int y0,
    int x1,
    int y1,
    Color c
) {
    drawLineBresenham(f, x0, y0, x1, y0, c);
    drawLineBresenham(f, x1, y0, x1, y1, c);
    drawLineBresenham(f, x1, y1, x0, y1, c);
    drawLineBresenham(f, x0, y1, x0, y0, c);
}


// Rotate (degrees) and scale about the polygon's own centre, then move it
static Polygon placed(
    const Polygon& p,
    double deg,
    double s,
    Vec2 move
) {
    return transformPolygon(
        p,
        Mat3::about(
            polygonCentroid(p),
            deg * M_PI / 180.0,
            s,
            move
        )
    );
}


// Offset copy of a polygon, used as a drop shadow
static void dropShadow(
    Framebuffer& f,
    Polygon p,
    double dx = 5,
    double dy = 6
) {
    for (Vec2& v : p) {
        v.x += dx;
        v.y += dy;
    }

    fillPolygon(f, p, pal::shadow);
}


// Every sticker goes through this so they all look the same style
static void sticker(
    Framebuffer& f,
    const Polygon& p,
    Color fill,
    Color edge
) {
    dropShadow(f, p);
    fillPolygon(f, p, fill);
    drawOutline(f, p, edge);
}


// Draw with the team's existing rasterisers, then blend the temporary layer.
// This keeps setPixel's replacement semantics unchanged for other modules.
static void blendLayer(
    Framebuffer& destination,
    const std::function<void(Framebuffer&)>& draw
) {
    Framebuffer layer(
        destination.width,
        destination.height,
        {0, 0, 0, 0}
    );

    draw(layer);

    for (size_t index = 0; index < layer.pixels.size(); index++) {
        if (layer.pixels[index].a != 0) {
            destination.pixels[index] =
                alphaBlend(
                    layer.pixels[index],
                    destination.pixels[index]
                );
        }
    }
}


// Measure the font's letter/digit widths, including character spacing.
static int textW(
    const std::string& s,
    int scale
) {
    return textWidth(s, scale);
}


// ---------- day page ----------

void buildDayPage(Framebuffer& fb) {

    Scene s;


    // 1. Notebook background and paper card
    s.add("background", [](Framebuffer& f) {

        f.clear(pal::brown);

        // Binder holes
        for (int y = 60; y < 580; y += 70) {
            fillCircle(
                f,
                28,
                y,
                11,
                pal::dark
            );
        }

        Polygon card =
            rectPoly(
                70,
                30,
                770,
                570
            );

        dropShadow(
            f,
            card,
            6,
            7
        );

        fillPolygon(
            f,
            card,
            pal::cream
        );
    });


    // 2. Polaroid with a procedural photo
    s.add("polaroid", [](Framebuffer& f) {

        Polygon frame =
            rectPoly(
                120,
                80,
                480,
                430
            );

        dropShadow(
            f,
            frame
        );

        fillPolygon(
            f,
            frame,
            pal::white
        );


        // Warm green -> mustard gradient
        for (int y = 100; y < 360; y++) {

            for (int x = 140; x < 460; x++) {

                double t =
                    ((x - 140) + (y - 100))
                    / 580.0;

                f.setPixel(
                    x,
                    y,
                    {
                        (uint8_t)(110 + t * 118),
                        (uint8_t)(145 + t * 20),
                        (uint8_t)(121 - t * 52)
                    }
                );
            }
        }


        // Sun
        fillCircle(
            f,
            395,
            160,
            30,
            pal::honey
        );


        // Hills
        fillPolygon(
            f,
            {
                {140, 360},
                {140, 295},
                {225, 250},
                {320, 305},
                {395, 265},
                {460, 300},
                {460, 360}
            },
            pal::deepGreen
        );


        applyFilter(
            f,
            140,
            100,
            460,
            360,
            sepia
        );


        drawRect(
            f,
            140,
            100,
            460,
            360,
            pal::dark
        );
    });


    // 3. Washi tape: source-over blending reveals
    // the photo and frame beneath.
    s.add("tape", [](Framebuffer& f) {

        blendLayer(
            f,
            [](Framebuffer& layer) {

                fillPolygon(
                    layer,
                    placed(
                        rectPoly(
                            -45,
                            -13,
                            45,
                            13
                        ),
                        -35,
                        1.0,
                        {140, 92}
                    ),
                    pal::tapeA
                );
            }
        );


        blendLayer(
            f,
            [](Framebuffer& layer) {

                fillPolygon(
                    layer,
                    placed(
                        rectPoly(
                            -45,
                            -13,
                            45,
                            13
                        ),
                        35,
                        1.0,
                        {462, 92}
                    ),
                    pal::tapeB
                );
            }
        );
    });


    // 4. STICKERS:

    // Regular polygon star
    s.add("star", [](Framebuffer& f) {

        sticker(
            f,
            placed(
                makeStar(
                    {0, 0},
                    50,
                    22
                ),
                15,
                1.2,
                {490, 125}
            ),
            pal::mustard,
            {150, 100, 30}
        );
    });

    // CURVES: Cubic Bezier heart:

    s.add("bezier-heart", [](Framebuffer& f) {

        Polygon p =
            placed(
                makeBezierHeart(
                    {0, 0},
                    90
                ),
                -20,
                1.0,
                {200, 320}
            );


        sticker(
            f,
            p,
            pal::terracotta,
            {120, 50, 35}
        );


        if (kDebug) {

            Vec2 mn;
            Vec2 mx;

            boundingBox(
                p,
                mn,
                mx
            );


            drawRect(
                f,
                (int)mn.x - 4,
                (int)mn.y - 4,
                (int)mx.x + 4,
                (int)mx.y + 4,
                pal::dark
            );


            std::cout
                << "pointInPolygon(Bezier heart, centre) = "
                << pointInPolygon(
                    p,
                    polygonCentroid(p)
                )
                << " (expect 1)\n"

                << "pointInPolygon(Bezier heart, far) = "
                << pointInPolygon(
                    p,
                    {700, 50}
                )
                << " (expect 0)\n";
        }
    });


    // Hexagon sticker
    s.add("hexagon", [](Framebuffer& f) {

        sticker(
            f,
            placed(
                makeRegularPolygon(
                    {0, 0},
                    40,
                    6
                ),
                10,
                1.0,
                {610, 130}
            ),
            pal::sage,
            pal::dark
        );
    });


    // Square sticker
    s.add("square", [](Framebuffer& f) {

        sticker(
            f,
            placed(
                makeSquare(
                    {0, 0},
                    64
                ),
                20,
                1.0,
                {560, 290}
            ),
            tint(
                pal::honey,
                pal::green,
                0.55
            ),
            {170, 120, 40}
        );
    });


    // Circle sticker
    s.add("circle", [](Framebuffer& f) {

        // Shadow
        fillCircle(
            f,
            703,
            246,
            36,
            pal::shadow
        );

        fillCircle(
            f,
            698,
            240,
            36,
            pal::green
        );

        drawCircleMidpoint(
            f,
            698,
            240,
            36,
            pal::deepGreen
        );
    });
    s.add("ellipse", [](Framebuffer& f) {
        fillEllipse(f, 695, 146, 42, 26, pal::shadow);        // shadow
        fillEllipse(f, 690, 140, 42, 26, pal::honey);
        drawEllipseMidpoint(f, 690, 140, 42, 26, pal::brown);
    });


    // CURVES: Catmull-Rom cloud:
   
    s.add("cloud", [](Framebuffer& f) {

        Polygon cloud =
            makeCloud(
                {635, 330},
                120,
                70
            );


        sticker(
            f,
            cloud,
            pal::white,
            pal::brown
        );
    });


    // CURVES: Curvy star generated using Catmull-Rom:

    s.add("curvy-star", [](Framebuffer& f) {

        Polygon star =
            makeCurvyStar(
                {520, 485},
                38,
                20,
                5,
                10
            );


        sticker(
            f,
            star,
            pal::honey,
            pal::brown
        );
    });

    // 5. Date stamp:

    s.add("date", [](Framebuffer& f) {

        Polygon stamp =
            makeDateStamp(
                {620, 400},
                280,
                64,
                12
            );


        sticker(
            f,
            stamp,
            pal::white,
            pal::brown
        );


        std::string d =
            "05-10-2026";


        drawText(
            f,
            620 - textW(d, 4) / 2,
            390,
            d,
            4,
            pal::dark
        );
    });


    // 6. Sticker clipped by the card edge:

    s.add("clipped", [](Framebuffer& f) {

        Polygon big =
            placed(
                makeStar(
                    {0, 0},
                    85,
                    38
                ),
                8,
                1.0,
                {735, 540}
            );


        Polygon cut =
            clipPolygon(
                big,
                70,
                30,
                770,
                570
            );


        fillPolygon(
            f,
            cut,
            pal::terracotta
        );


        drawOutline(
            f,
            cut,
            {120, 50, 35}
        );


        if (kDebug) {

            drawRect(
                f,
                70,
                30,
                770,
                570,
                {90, 90, 90}
            );
        }
    });


    s.render(fb);
}


// ---------- calendar ----------

void buildCalendar(Framebuffer& fb) {

    const int x0 = 70;
    const int y0 = 160;

    const int cw = 94;
    const int ch = 68;

    const int hdr = 34;

    // 1 Oct 2026 is a Thursday
    // Sunday = column 0
    const int startCol = 4;

    const int today = 5;


    fb.clear(
        pal::brown
    );


    // Binder holes
    for (int y = 60; y < 580; y += 70) {

        fillCircle(
            fb,
            28,
            y,
            11,
            pal::dark
        );
    }


    Polygon card =
        rectPoly(
            55,
            30,
            770,
            570
        );


    dropShadow(
        fb,
        card,
        6,
        7
    );


    fillPolygon(
        fb,
        card,
        pal::cream
    );

    // Title and weekday header:

    std::string title =
        "2026";


    drawText(
        fb,
        412 - textW(title, 5) / 2,
        70,
        title,
        5,
        pal::dark
    );


    const char* days[7] = {
        "S",
        "M",
        "T",
        "W",
        "T",
        "F",
        "S"
    };


    for (int c = 0; c < 7; c++) {

        fillRect(
            fb,
            x0 + c * cw,
            y0 - hdr,
            x0 + (c + 1) * cw,
            y0,
            pal::green
        );


        drawText(
            fb,
            x0 + c * cw + cw / 2 - 10,
            y0 - hdr + 7,
            days[c],
            3,
            pal::paper
        );
    }

    // Cell backgrounds:

    for (int r = 0; r < 5; r++) {

        for (int c = 0; c < 7; c++) {

            fillRect(
                fb,
                x0 + c * cw,
                y0 + r * ch,
                x0 + (c + 1) * cw,
                y0 + (r + 1) * ch,
                (c == 0 || c == 6)
                    ? Color{247, 222, 178}
                    : pal::paper
            );
        }
    }

    // Grid lines using Bresenham:

    for (int i = 0; i <= 7; i++) {

        drawLineBresenham(
            fb,
            x0 + i * cw,
            y0 - hdr,
            x0 + i * cw,
            y0 + 5 * ch,
            pal::brown
        );
    }


    for (int j = 0; j <= 5; j++) {

        drawLineBresenham(
            fb,
            x0,
            y0 + j * ch,
            x0 + 7 * cw,
            y0 + j * ch,
            pal::brown
        );
    }


    drawLineBresenham(
        fb,
        x0,
        y0 - hdr,
        x0 + 7 * cw,
        y0 - hdr,
        pal::brown
    );


    // Calendar days:

    for (int d = 1; d <= 31; d++) {

        int col =
            (startCol + d - 1)
            % 7;


        int row =
            (startCol + d - 1)
            / 7;


        int cx =
            x0 + col * cw;


        int cy =
            y0 + row * ch;


        Vec2 mid{
            cx + cw / 2.0,
            cy + ch / 2.0
        };


        std::string num =
            std::to_string(d);


        // Highlighted current day
        if (d == today) {

            fillPolygon(
                fb,
                placed(
                    makeStar(
                        {0, 0},
                        33,
                        15
                    ),
                    0,
                    1.0,
                    {
                        mid.x,
                        mid.y + 2
                    }
                ),
                pal::mustard
            );


            drawText(
                fb,
                (int)mid.x - textW(num, 3) / 2,
                (int)mid.y - 8,
                num,
                3,
                pal::dark
            );


            continue;
        }


        drawText(
            fb,
            cx + 8,
            cy + 8,
            num,
            3,
            pal::dark
        );

        // Little stickers on days that "have a page saved":

        Vec2 corner{
            cx + cw - 22.0,
            cy + ch - 20.0
        };


        // Bezier heart stickers
        if (d == 1 || d == 4) {

            fillPolygon(
                fb,
                placed(
                    makeBezierHeart(
                        {0, 0},
                        22
                    ),
                    0,
                    1.0,
                    corner
                ),
                pal::terracotta
            );
        }


        // Circle
        if (d == 2) {

            fillCircle(
                fb,
                (int)corner.x,
                (int)corner.y,
                9,
                pal::green
            );
        }


        // Ordinary polygon star
        if (d == 3) {

            fillPolygon(
                fb,
                placed(
                    makeStar(
                        {0, 0},
                        12,
                        5
                    ),
                    0,
                    1.0,
                    corner
                ),
                pal::mustard
            );
        }
    }
}


// ---------- entry point ----------

int main(int argc, char* argv[]) {

    std::error_code error;


    std::filesystem::create_directories(
        "output",
        error
    );


    if (error) {

        std::cerr
            << "Cannot create output directory: "
            << error.message()
            << "\n";

        return 1;
    }

    // Build day scrapbook page:

    Framebuffer day(
        800,
        600,
        pal::brown
    );


    buildDayPage(
        day
    );


    if (
        !day.saveBMP(
            "output/day_page.bmp"
        )
        ||
        !day.savePPM(
            "output/day_page.ppm"
        )
    ) {

        std::cerr
            << "Cannot save day page\n";

        return 1;
    }

    // Build calendar:

    Framebuffer cal(
        800,
        600,
        pal::brown
    );


    buildCalendar(
        cal
    );


    if (
        !cal.saveBMP(
            "output/calendar.bmp"
        )
        ||
        !cal.savePPM(
            "output/calendar.ppm"
        )
    ) {

        std::cerr
            << "Cannot save calendar\n";

        return 1;
    }


    std::cout
        << "Wrote output/day_page.* and output/calendar.*\n";

    runDemoSequences();
    runStickerDemos();

    // Check for headless / non-interactive flag (useful for automated ctest / CI)
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless" || arg == "--no-window" || arg == "-h") {
            headless = true;
        }
    }

    if (headless) {
        std::cout << "[DigiScrap] Headless mode requested. Skipping OpenGL window.\n";
        return 0;
    }

    std::cout << "\n=======================================================\n";
    std::cout << "Starting OpenGL 3.3 Interactive Window (800x600)...\n";
    std::cout << "Controls:\n";
    std::cout << "  [1] Coloured Triangle (OpenGL Hardware Core)\n";
    std::cout << "  [2] Scrapbook Day Page (Software Framebuffer Texture Fallback)\n";
    std::cout << "  [3] Calendar Grid (Software Framebuffer Texture Fallback)\n";
    std::cout << "  [4] Hardware Transformed Polygons (Mat3 to 4x4 Affine Shader)\n";
    std::cout << "  [5] Textured Photo + Filters + Tape + Date Stamp (Option 3)\n";
    std::cout << "      [F] next filter  [Up/Down] filter strength   (run with --photo file.bmp|.ppm)\n";
    std::cout << "  [6] Interactive Multi-Day Calendar, Live Animations & Saving (Option 5)\n";
    std::cout << "      Calendar: Click day / [Enter] open page, [Arrows] glide highlight, [</> ] month\n";
    std::cout << "      Day Page: Drag sticker, [A] add sticker, [D] drop anim, [N/P] page turn, [B] back\n";
    std::cout << "  [Space] Cycle modes\n";
    std::cout << "  [ESC] Exit\n";
    std::cout << "=======================================================\n\n";

    GLFWwindow* window = gl::createWindow(800, 600, "DigiScrap - Scrapbook Studio (OpenGL 3.3 Core)");
    if (!window) {
        std::cerr << "Could not create GLFW window (no display available). Exiting cleanly.\n";
        return 0;
    }

    if (!gl::initRenderer()) {
        std::cerr << "Failed to initialise OpenGL renderer.\n";
        gl::destroyWindow(window);
        return 1;
    }

    // ---- Option 3: photo texture + font atlas (CPU loaders are our own code) ----
    std::string photoPath;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--photo") photoPath = argv[i + 1];
    Framebuffer photoImg(1, 1);
    if (photoPath.empty() || !loadImage(photoPath, photoImg)) {
        if (!photoPath.empty()) std::cerr << "Could not load " << photoPath << ", using the demo photo.\n";
        photoImg = makeDemoPhoto(360, 270);
    } else {
        photoImg = resizeBilinear(photoImg, 360, 270);
    }
    gl::initTextureRenderer();
    gl::Texture photoTex = gl::createTexture(photoImg);
    gl::Texture fontTex  = gl::createTexture(buildFontAtlas(), true);   // NEAREST keeps pixels crisp
    // Date stamp text = today's date (computer clock), formatted YYYY-MM-DD
    std::string stampText;
    {
        std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
        stampText = buf;
    }
    FilterParams photoFilter;
    photoFilter.kind = FILTER_SEPIA;
    photoFilter.strength = 1.0f;

    // ---- Option 5: Interactive Multi-Day Calendar, Live Animations & Saving ----
    CalendarScrapbookApp calendarApp;
    calendarApp.init(photoTex, fontTex, "saves", photoPath.empty() ? "demo" : photoPath);

    int currentMode = 5; // Default to Option 5 interactive calendar & scrapbook (1..6 switch modes)
    double elapsedAnim = 0.0;

    gl::WindowCallbacks cb;
    cb.onResize = [](int w, int h) {
        gl::setProjection(w, h);
        gl::setTextureProjection(w, h);
    };

    cb.onKey = [&currentMode, &photoFilter, &calendarApp](int key, int action) {
        if (currentMode == 5 && calendarApp.isEditingDescription()) {
            calendarApp.onKey(key, action);
            return;
        }
        if (action == 1) { // GLFW_PRESS
            if (key == 49)      { currentMode = 0; return; }            // '1'
            else if (key == 50) { currentMode = 1; return; }            // '2'
            else if (key == 51) { currentMode = 2; return; }            // '3'
            else if (key == 52) { currentMode = 3; return; }            // '4'
            else if (key == 53) { currentMode = 4; return; }            // '5'
            else if (key == 54) { currentMode = 5; return; }            // '6'
            else if (key == 32) { currentMode = (currentMode + 1) % 6; return; } // Space
        }
        if (currentMode == 5) {
            calendarApp.onKey(key, action);
            return;
        }
        if (action == 1 || action == 2) { // GLFW_PRESS / REPEAT
            if (key == 265) photoFilter.strength = std::min(1.0f, photoFilter.strength + 0.1f);   // Up
            if (key == 264) photoFilter.strength = std::max(0.0f, photoFilter.strength - 0.1f);   // Down
        }
        if (action == 1) { // GLFW_PRESS
            if (key == 70) photoFilter.kind = (FilterKind)((photoFilter.kind + 1) % FILTER_COUNT); // F
        }
    };

    cb.onMouseButton = [&currentMode, &calendarApp](int button, int action, double mx, double my) {
        if (currentMode == 5) {
            calendarApp.onMouseButton(button, action, mx, my);
        }
    };

    cb.onCursorPos = [&currentMode, &calendarApp](double mx, double my) {
        if (currentMode == 5) {
            calendarApp.onCursorPos(mx, my);
        }
    };

    cb.onChar = [&currentMode, &calendarApp](unsigned int codepoint) {
        if (currentMode == 5) {
            calendarApp.onChar(codepoint);
        }
    };

    cb.onDrop = [&currentMode, &calendarApp](const std::string& filePath) {
        if (currentMode == 5) {
            calendarApp.onDropFile(filePath);
        }
    };

    cb.onUpdate = [&elapsedAnim, &currentMode, &calendarApp](double dt) {
        elapsedAnim += dt;
        if (currentMode == 5) {
            calendarApp.update(dt);
        }
    };

    cb.onRender = [&]() {
        if (currentMode == 0) {
            // Coloured triangle (Task 1 primary deliverable)
            // Rendered via gl::drawTriangle using OpenGL 3.3 Core shaders and ortho projection
            gl::drawTriangle(400.0f, 100.0f,
                             120.0f, 520.0f,
                             680.0f, 520.0f,
                             {230, 50, 90, 255},   // Red / Pink
                             {110, 145, 121, 255}, // Green
                             {249, 205, 125, 255}  // Honey yellow
            );
        } else if (currentMode == 1) {
            // Fallback mode 1: Software Framebuffer as OpenGL Texture (Day page)
            gl::drawFramebufferTexture(day);
        } else if (currentMode == 2) {
            // Fallback mode 2: Software Framebuffer as OpenGL Texture (Calendar)
            gl::drawFramebufferTexture(cal);
        } else if (currentMode == 3) {
            // Hardware Mat3 Affine Transformed Polygons (demonstrates mat3ToMat4 in action)
            Mat3 starTransform = Mat3::translate(400.0, 300.0)
                               * Mat3::rotate(elapsedAnim * 0.8)
                               * Mat3::scale(1.2 + 0.25 * std::sin(elapsedAnim * 2.0),
                                             1.2 + 0.25 * std::sin(elapsedAnim * 2.0));
            Polygon star = makeStar({0, 0}, 90, 40);
            gl::drawPolygon(star, starTransform, {255, 200, 0, 230});
            gl::drawPolyline(star, starTransform, {150, 110, 0, 255});

            Mat3 heartTransform = Mat3::translate(200.0, 300.0)
                                * Mat3::rotate(std::sin(elapsedAnim) * 0.3);
            Polygon heart = makeHeart({0, 0}, 80);
            gl::drawPolygon(heart, heartTransform, {230, 50, 90, 220});
            gl::drawPolyline(heart, heartTransform, {120, 20, 50, 255});
        } else if (currentMode == 4) {
            // Option 3: textured, filtered photo + translucent tape + date stamp.
            // The photo is a base element placed by our own Mat3; the filter maths run in the fragment shader.
            Mat3 photoM = Mat3::translate(260, 90) * Mat3::rotate(-0.05);
            gl::drawTexture(photoTex, photoM, 1.0f, photoFilter);

            // Same photo again as a 60% translucent sticker with a different filter (Final Project: filters on stickers)
            FilterParams grey; grey.kind = FILTER_GRAYSCALE; grey.strength = 1.0f;
            gl::drawTexture(photoTex, Mat3::translate(70, 330) * Mat3::rotate(0.12) * Mat3::scale(0.35, 0.35), 0.6f, grey);

            // Washi tape: translucent pink polygon, straight-alpha source-over blending
            Polygon tape = makeRibbon({0, 0}, 130, 34, 6);
            gl::drawPolygon(tape, Mat3::translate(300, 96) * Mat3::rotate(-0.45), {255, 150, 190, 150});
            gl::drawPolygon(tape, Mat3::translate(600, 330) * Mat3::rotate(0.5),  {140, 210, 190, 150});

            // A sticker and the date stamp
            gl::drawPolygon(makeStar({0, 0}, 50, 22), Mat3::translate(640, 120) * Mat3::rotate(elapsedAnim * 0.6), {255, 200, 0, 230});
            Polygon stamp = makeDateStamp({0, 0}, 200, 50, 8);
            Mat3 stampM = Mat3::translate(560, 500) * Mat3::rotate(-0.06);
            gl::drawPolygon(stamp, stampM, {250, 240, 215, 255});
            gl::drawPolyline(stamp, stampM, {150, 80, 60, 255});
            gl::drawStampText(fontTex, stampText, -88, -9, 3.0f, {150, 50, 40, 255}, stampM);

            // Print the active filter when it changes
            static int lastKind = -1; static float lastStrength = -1;
            if (lastKind != (int)photoFilter.kind || lastStrength != photoFilter.strength) {
                std::cout << "Photo filter: " << filterName(photoFilter.kind) << "  strength " << photoFilter.strength << "\n";
                lastKind = (int)photoFilter.kind; lastStrength = photoFilter.strength;
            }
        } else if (currentMode == 5) {
            // Option 5: Interactive Multi-Day Calendar, Live Animations & Saving
            calendarApp.render();
        }
    };

    gl::runLoop(window, cb);

    calendarApp.shutdown();
    gl::deleteTexture(photoTex);
    gl::deleteTexture(fontTex);
    gl::shutdownTextureRenderer();
    gl::shutdownRenderer();
    gl::destroyWindow(window);
    return 0;
}