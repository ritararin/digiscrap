# DigiScrap — 2D Software Rendering Engine & Digital Scrapbook

A pure C++17 software rendering foundation and digital scrapbook application built from scratch without external graphics or mathematics libraries. All geometric calculations, rasterization, clipping, filling, transformations, curve evaluations, and color blending algorithms are implemented by hand. Output images are generated directly to custom pixel buffers and exported as standard uncompressed 24-bit BMP and Netpbm P6 PPM images.

---

## Project Overview

DigiScrap is a paired computer graphics project designed to demonstrate fundamental 2D raster graphics techniques by writing a complete rendering pipeline from the ground up:
- **No Graphics Libraries**: Drawing is performed entirely by custom code operating on an in-memory pixel buffer (`Framebuffer`). No OpenGL, DirectX, Vulkan, SDL, Cairo, or OpenCV are used.
- **Pure C++17 Implementation**: Standard C++17 features with zero third-party dependencies.
- **Modular Architecture**: Modules for lines, polygons, clipping, circles/ellipses, curves, colour filters, affine transforms, scene management, and bitmap typography.
- **Two Complete Project Demonstrations**:
  1. **A Day Page (`output/day_page.bmp`)**: A layered scrapbook layout with binder backing, card drop shadow, procedural landscape photo, sepia filter, semi-transparent washi tape via source-over alpha blending, rotated stickers (star, heart, square, hexagon, circle, ellipse, ribbon), Catmull-Rom cloud, a date stamp, and a corner star clipped against the card edge.
  2. **A Calendar Month Grid (`output/calendar.bmp`)**: A complete October 2026 calendar page with header labels, weekday column headers, 35-cell grid with tinted weekends, divider lines rasterized via integer Bresenham, a highlighted day (Day 5) with a centered gold star, and mini scrapbook stickers.
- **Checkpoint 2 Animation**: Three short demo animations (`output/anim/`) built on the animation core: stickers flying onto the day page, a calendar highlight moving between days, and a day-page-to-calendar crossfade.

---

## Group Members and Responsibilities

| Member | Student ID | Responsibilities |
| :--- | :--- | :--- |
| **Ian Tuitoek** | 165913 | **Core Architecture & Engine Foundation**: Designed `Framebuffer` pixel buffer, 24-bit uncompressed BMP and Netpbm P6 PPM exporters, 3×3 homogeneous affine matrix system (`Mat3`), `Scene` painter's-order layer manager, 3×5 bitmap font engine, and initial demo pipeline. **CP2 — Animation Core & Integration**: `Transform`, `Timeline`/`lerp`, `Mat3` inverse/shear/reflection, frame exporter with contact sheets, animated `Scene` layers, demo sequences, and merging all `cp2-*` branches. |
| **Rita Palmeris** | 159314 | **Polygons & Scanline Fill**: Polygon vector representation, geometric shape generators (`makeStar`, `makeHeart`, `makeSquare`, `makeRegularPolygon`, `makeDateStamp`), bounding box, Shoelace area, centroid calculation, ray-casting point-in-polygon test, and active-edge-table scanline fill algorithm. |
| **Myra Nyamwanda** | 166982 | **Clipping & Colour Pipeline**: Cohen-Sutherland line clipping (4-bit outcodes), Liang-Barsky parametric line clipping, Sutherland-Hodgman polygon clipping, colour filters (`grayscale`, `sepia`, `tint`, `brightness`, `contrast`, `invert`), RGBA source-over alpha blending, and 120,000-line randomized test harness. |
| **Alvin Eredi** | 168560 | **Lines, Circles, Ellipses & Calendar Grid**: All-octant integer-arithmetic Bresenham line algorithm, Midpoint circle outline and integer scanline circle fill, Midpoint ellipse outline (two-region partition) and scanline fill, ribbon generators (`makeRibbon`, `makeWavyRibbon`), lines/circles test suite, and calendar grid integration. |
| **Judah Ndivo** | 167246 | **Curves & Splines**: Cubic Bézier curve evaluator with Bernstein polynomials (`bezierCubic`), Catmull-Rom spline interpolation with boundary phantom points (`catmullRom`), composite curved shapes (`makeBezierHeart`, `makeCloud`, `makeCurvyStar`), and day page curve integration. |

---

## Technologies & Tools

- **Programming Language**: C++17.
- **Compilers Supported**:
  - GCC / MinGW (`g++` 11+ / 15+)
  - Microsoft Visual Studio C++ (MSVC 2019/2022)
  - Clang / LLVM
- **Build Systems**:
  - **GNU Make** (`Makefile`): Automated cross-platform build supporting Windows (`cmd`/`PowerShell`) and POSIX (`Linux`/`macOS`).
  - **CMake** (v3.10+): Full project configuration, target separation, and CTest integration.
- **Image File Formats**:
  - **BMP**: Hand-crafted 24-bit uncompressed Windows Bitmap (BGR color order, 4-byte row padding, 54-byte headers).
  - **PPM**: Netpbm binary P6 format (RGB raw bytes, 255 max color component).

---

## Graphics Techniques & Algorithms Implemented

### 1. Lines (DDA and Bresenham)
- **Digital Differential Analyzer (DDA)**: Floating-point incremental line rasterization using `std::max(|dx|, |dy|)` steps and `std::lround` rounding.
- **All-Octant Integer Bresenham**: Pure integer line drawing using the two-variable error accumulator formulation ($dx + dy$). Handles all 8 octants, steep and shallow slopes, horizontal, vertical, diagonal, and degenerate single-point segments without floating-point arithmetic or division.

### 2. Polygons (Representation, Transforms, Point-in-Polygon)
- **Representation**: `Polygon` defined as `std::vector<Vec2>`.
- **Affine Transformations**: 2D homogeneous $3 \times 3$ matrices (`Mat3`) supporting translation, non-uniform scaling, rotation, matrix concatenation, and arbitrary pivot transformations (`Mat3::about`).
- **Geometric Analysis**:
  - Bounding box calculation (`boundingBox`).
  - Shoelace formula for signed and absolute polygon area (`polygonArea`).
  - True polygon centroid calculation via weighted triangular decomposition (`polygonCentroid`).
  - Even-odd ray casting point-in-polygon test (`pointInPolygon`).
- **Parametric Shape Generators**: Star, heart, square, regular $n$-gon, date stamp, classic swallowtail ribbon banner (`makeRibbon`), and flowing sinusoidal ribbon (`makeWavyRibbon`).

### 3. Clipping and Filling
- **Parity Scanline Polygon Fill**: Edge Table (ET) sorted by $y_{min}$ combined with an Active Edge List (AEL) updated at pixel row centers ($y + 0.5$) with even-odd span pairing. Fills concave and convex polygons without holes or leaks.
- **Cohen-Sutherland Line Clipping**: 4-bit outcode classification (Left, Right, Below, Above), trivial accept/reject, and iterative edge intersections against rectangular windows.
- **Liang-Barsky Line Clipping**: Parametric segment clipping updating interval $[t_0, t_1]$ against the 4 half-plane boundary constraints.
- **Sutherland-Hodgman Polygon Clipping**: Successive edge clipping against Left, Right, Below, and Above window boundaries, correctly emitting intersection and interior vertices.

### 4. Circles and Ellipses
- **Midpoint Circle Algorithm**: Integer decision parameter $d = 1 - r$ with 8-way symmetry.
- **Midpoint Circle Fill**: Integer scanline filling using 8-way symmetry horizontal spans without floating-point math or `std::sqrt`.
- **Midpoint Ellipse Algorithm**:
  - **Region 1**: Steps primarily along $x$ where slope magnitude $|\frac{dy}{dx}| < 1$ using decision variable $d_1 = r_y^2 - r_x^2 r_y + \frac{1}{4} r_x^2$.
  - **Region 2**: Steps primarily along $y$ where slope magnitude $|\frac{dy}{dx}| \ge 1$ using decision variable $d_2 = r_y^2(x + 0.5)^2 + r_x^2(y - 1)^2 - r_x^2 r_y^2$.
  - 4-way symmetry using 64-bit integer arithmetic (`int64_t`) to prevent numerical overflow. Handles degenerate axes ($r_x = 0$ or $r_y = 0$).
- **Midpoint Ellipse Fill**: Fills solid ellipse horizontal spans at each step along the boundary.

### 5. Curves and Splines
- **Cubic Bézier Curve**: Evaluates $B(t) = (1-t)^3 P_0 + 3(1-t)^2 t P_1 + 3(1-t) t^2 P_2 + t^3 P_3$ sampled into discrete polygonal segments for $t \in [0, 1]$.
- **Catmull-Rom Spline**: Cubic Hermite spline through arbitrary control points with automatic phantom endpoint replication ($P_{-1} = P_0$, $P_{n} = P_{n-1}$) ensuring $C^1$ continuity and passing through all specified points.
- **Composite Curve Shapes**: Dual-Bézier heart, Catmull-Rom cloud, and closed curvy star.

### 6. Colour, Shading & Layering
- **Filters**: Weighted luminance grayscale ($0.299R + 0.587G + 0.114B$), sepia matrix transformation, color tinting, brightness addition, contrast adjustment centered at 128, and color channel inversion.
- **Alpha Blending**: Straight-RGBA source-over compositing ($C_{out} = C_{src} \cdot \alpha_{src} + C_{dst} \cdot \alpha_{dst} (1 - \alpha_{src})$).
- **Region Filtering**: Whole-framebuffer or half-open rectangular subregion filtering.
- **Drop Shadows & Compositing**: Offset shadow polygons and multi-layer rendering order via `Scene`.
- **Typography**: Custom 3×5 bitmap font rendering digits `0-9`, hyphen `-`, and uppercase letters `A-Z`.

### 7. Animation Core (Checkpoint 2)
- **Transform**: position, rotation (degrees), scale and pivot, converted to a 3×3 homogeneous matrix as `T(position) · R · S · T(−pivot)`, so objects rotate and scale about their own pivot.
- **Extended `Mat3`**: shear, reflection (across the x-axis, y-axis or any line through the origin), determinant, an adjugate-based **inverse** (available for hit-testing and zoom), and a matrix-times-point helper.
- **Timeline and lerp**: each frame gets a time `t` from exactly 0 to exactly 1; `lerp` interpolates numbers, `Vec2` and `Color`, and is exact at both ends.
- **Frame exporter**: renders every frame through `renderFrame(fb, t)`, saves numbered BMPs (`output/anim/name_0001.bmp`) and a contact sheet of evenly spaced frames built with a box-filter downscale.
- **Animated Scene layers**: `addAnimated` and `addTimed` layers and `render(fb, t)`.
- **Demo sequences** (`src/animation/demo_sequences.cpp`): stickers flying onto the day page, a calendar highlight moving between days, and a day-page-to-calendar crossfade.

---

## How to Compile and Run

### Prerequisites
A C++17 compliant compiler (`g++` / MinGW, `clang++`, or MSVC).

### Option 1: Using GNU Make (Recommended)

From the project root directory (`digiscrap`):

```bash
# Build all executables (scrapbook demo + test suites)
mingw32-make all       # On Windows (MinGW)
# or: make all         # On Linux / macOS

# Run the test suites
mingw32-make test      # Runs polygons_test, clipping_colour_test, lines_circles_test and animation_test
# or: make test

# Run the scrapbook demo (generates output images and animation frames)
mingw32-make run
# or: make run

# Clean build artifacts
mingw32-make clean
# or: make clean
```

### Option 2: Using CMake & CTest

```bash
# Configure the build directory
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets
cmake --build build --config Release

# Run automated tests via CTest
ctest --test-dir build --output-on-failure -C Release

# Run the demo
./build/Release/scrapbook.exe   # Windows
# or: ./build/scrapbook        # Linux / macOS
```

### Option 3: Direct Compilation with g++

```bash
# Compile and run the demo
g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/main.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp src/animation/exporter.cpp src/animation/demo_sequences.cpp src/stickers/demo_drag.cpp -o scrapbook.exe
./scrapbook.exe

# Compile and run the test suites
g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/polygons/test_polygons.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o polygons_test.exe
./polygons_test.exe

g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/clipping/test_clipping_colour.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o clipping_colour_test.exe
./clipping_colour_test.exe

g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/circles/test_lines_circles.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o lines_circles_test.exe
./lines_circles_test.exe

g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/animation/test_animation_core.cpp src/animation/exporter.cpp -o animation_test.exe
./animation_test.exe

g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/stickers/test_stickers.cpp src/polygons/polygons.cpp src/lines/lines.cpp -o sticker_test.exe
./sticker_test.exe
```

---

## Current Progress

- **All 5 Core Algorithm Groups Implemented**:
  1. Lines: DDA and genuine integer Bresenham (all 8 octants).
  2. Polygons: Full representation, Shoelace area, centroid, ray-casting point-in-polygon, affine matrix transforms, and diverse generators (star, heart, regular polygon, date stamp, classic ribbon, wavy ribbon).
  3. Clipping & Filling: Parity active-edge-table scanline fill, Cohen-Sutherland line clipping, Liang-Barsky line clipping, Sutherland-Hodgman polygon clipping.
  4. Circles & Ellipses: Midpoint circle outline, midpoint circle scanline fill, midpoint ellipse outline (two-region slope partition), midpoint ellipse scanline fill.
  5. Curves: Parametric Cubic Bézier curve evaluation, Catmull-Rom spline interpolation, Bézier heart, Catmull-Rom cloud, and curvy star.
- **Both Demo Images Generated & Validated**:
  - `output/day_page.bmp` and `.ppm`: Complete scrapbook day page with photographic composition, filters, tape blending, rotated stickers, curves, and clipped elements.
  - `output/calendar.bmp` and `.ppm`: Full monthly calendar grid with Bresenham divider lines, weekend tinting, highlighted star date, and mini stickers.
- **100% Automated Test Passing**:
  - `polygons_test`: Validates exact area, centroid, bounding box, ray-casting inside/outside, ribbons, and scanline fill pixel count accuracy.
  - `clipping_colour_test`: Validates line clipping against 120,000 randomized test lines, edge cases, polygon clipping, color filters, and alpha blending.
  - `lines_circles_test`: Validates Bresenham across all 8 octants, midpoint circle 8-way symmetry and radial distance, circle fill pixel counts, and midpoint ellipse 4-way symmetry and algebraic fit.
  - `animation_test`: Validates `Mat3` rotation, reflection, shear and inverse (M·M⁻¹ = I, singular matrices), `Transform` pivot behaviour, `lerp` exactness at both ends, `Timeline` (first frame t = 0, last frame t = 1), timed `Scene` layers, and the frame exporter (file names, downscale, contact sheet size).
  - `sticker_test`: Validates transformed polygon area scaling by $s^2$, non-uniform scaling ($s_x \cdot s_y$), inverse-matrix hit testing after rotation ($0^\circ, 45^\circ, 90^\circ, 135^\circ, 180^\circ, 270^\circ$), matrix inverse round-trip ($M \cdot M^{-1} \approx I$), z-order pick priority, `bringToFront`, and polygon vertex morphing endpoints and continuity.
  - `easing_paths_test`: Validates easing functions, Catmull-Rom & Bézier arc-length motion paths, and page curl edge interpolation.
  - `opengl_test`: Validates 3x3 to 4x4 matrix conversion for OpenGL shaders, orthographic pixel-to-NDC projection, and transformation vector equivalence.
  - `calendar_test`: Validates calendar cell geometry (`cellCentre`, `dayAt`), quadratic Bézier highlight trajectories, plain-text day layout serialization and loading (`saveDayLayout`/`loadDayLayout`), software thumbnail generation (`renderDayThumbnail`), ear-clipping triangulation area preservation (`triangulateConcave`), live drop-and-settle (`evaluateDropAndSettle`), and live page-turn regions (`computePageTurnClipRegion`).
- **Checkpoint 2, Animation Core & Stickers**:
  - **Animation Core**: `Transform`, extended `Mat3` (shear, reflection, inverse), `Timeline`/`lerp`, frame exporter with contact sheets, animated `Scene` layers and three demo sequences.
  - **Stickers & Interactive Transforms**: `Sticker` struct bundling polygon, `Transform`, colour and `zOrder`; composite affine transformations about centroid pivot; inverse-matrix hit testing; `StickerScene` front-to-back picking and `bringToFront`; scripted drag simulation sequence; and polygon vertex morphing.
- **Checkpoint 3, OpenGL 3.3 Core Renderer & Windowing Foundation**:
  - **Windowing & Event Loop**: 800×600 interactive GLFW window with V-Sync, dynamic framebuffer resize callback, keyboard callbacks, and escape exit.
  - **Cross-Platform Dependencies**: `CMakeLists.txt` automatically fetches GLFW 3.4 via `FetchContent` if not installed locally; bundles GLAD OpenGL 3.3 Core profile loader.
  - **Shader Loader**: Compiles vertex and fragment shaders with detailed compile/link error reporting.
  - **Orthographic Projection & Matrix Conversion**: Computes top-left origin pixel-space orthographic projection matrix and converts `Mat3` affine transforms into column-major 4×4 float arrays for shaders.
  - **Primary Deliverable & Fallback**: Renders smooth hardware-shaded coloured triangle (`gl::drawTriangle`), alongside a fallback that uploads the software `Framebuffer` as a dynamic OpenGL texture (`gl::drawFramebufferTexture`). Toggleable via keys `[1]`–`[6]` or `[Space]`.
- **Checkpoint 3, Option 5 — Calendar, Live Animations & Multi-Day Layout Saving**:
  - **What Was Built & Connection to CP1/CP2**: Turns DigiScrap into an interactive multi-day scrapbook product (`src/calendar/calendar_gl.*`, `src/calendar/day_storage.*`, `src/calendar/calendar.*`). The calendar month view uses our CP1/CP2 `CalendarLayout` grid maths (`cellRect`, `cellCentre`, `dayAt`) and Sakamoto's calendar algorithm for month navigation, while rendering live miniature thumbnails of saved pages generated by scaling each day's elements with `Mat3` into our CP1 scanline polygon rasterizer (`fillPolygon` and `alphaBlend`) and uploading them as OpenGL textures (`gl::createTexture`). Clicking a day triggers our CP2 quadratic Bézier highlight arch (`highlightCentre`), cubic Bézier page-curl transition (`motion::pageCurlRegion`, `motion::pageCurlEdge`, `easing::easeInOutCubic`), and staggered sticker drop-and-settle animation (`easing::easeOutBounce`, `easing::easeOutBack`), while persisting every element's polygon type, `Transform`, RGBA fill/edge colours, `zOrder`, and photo path to plain-text files (`saves/day_YYYY_MM_DD.txt`) with automatic save and reload across app sessions.
  - **Two-Sentence OpenGL & Algorithm Summary**: Our CP1/CP2 geometry, easing (`easeOutBounce`, `easeOutBack`, `easeInOutCubic`), quadratic/cubic Bézier motion paths (`highlightCentre`, `pageCurlRegion`), ear-clipping triangulation (`triangulateConcave`), and `Mat3` affine transforms compute every vertex position, thumbnail framebuffer, and hit-test on the CPU each frame. OpenGL 3.3 Core then displays those CPU-computed triangles, outlines, and thumbnail/photo textures through our orthographic 4×4 matrix shader pipeline (`gl::drawPolygon`, `gl::drawPolyline`, `gl::drawTexture`, `gl::drawStampText`).

---
