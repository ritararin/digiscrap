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

---

## Group Members and Responsibilities

| Member |  | Responsibilities |
| :--- | :--- | :--- |
| **Ian Tuitoek** | 165913 | **Core Architecture & Engine Foundation**: Designed `Framebuffer` pixel buffer, 24-bit uncompressed BMP and Netpbm P6 PPM exporters, 3×3 homogeneous affine matrix system (`Mat3`), `Scene` painter's-order layer manager, 3×5 bitmap font engine, and initial demo pipeline. |
| **Rita Palmeris** | 159314 | **Polygons & Scanline Fill**: Polygon vector representation, geometric shape generators (`makeStar`, `makeHeart`, `makeSquare`, `makeRegularPolygon`, `makeDateStamp`), bounding box, Shoelace area, centroid calculation, ray-casting point-in-polygon test, and active-edge-table scanline fill algorithm. |
| **Myra Nyamwanda** | 166982 | **Clipping & Colour Pipeline**: Cohen-Sutherland line clipping (4-bit outcodes), Liang-Barsky parametric line clipping, Sutherland-Hodgman polygon clipping, colour filters (`grayscale`, `sepia`, `tint`, `brightness`, `contrast`, `invert`), RGBA source-over alpha blending, and 120,000-line randomized test harness. |
| **Alvin Eredi** | 168560 | **Lines, Circles, Ellipses & Calendar Grid**: All-octant integer-arithmetic Bresenham line algorithm, Midpoint circle outline and integer scanline circle fill, Midpoint ellipse outline (two-region partition) and scanline fill, ribbon generators (`makeRibbon`, `makeWavyRibbon`), lines/circles test suite, and calendar grid integration. |
| **Judah Ndivo** | 167246 | **Curves & Splines**: Cubic Bézier curve evaluator with Bernstein polynomials (`bezierCubic`), Catmull-Rom spline interpolation with boundary phantom points (`catmullRom`), composite curved shapes (`makeBezierHeart`, `makeCloud`, `makeCurvyStar`), and day page curve integration. |

---

## Technologies & Tools

- **Programming Language**: C++17 .
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
mingw32-make test      # Runs polygons_test, clipping_colour_test, and lines_circles_test
# or: make test

# Run the scrapbook demo (generates output images)
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
g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/main.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o scrapbook.exe
./scrapbook.exe

# Compile and run the test suites
g++ -Isrc -std=c++17 -D_USE_MATH_DEFINES -O2 src/polygons/test_polygons.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o polygons_test.exe
./polygons_test.exe

g++ -Isrc -D_USE_MATH_DEFINES -O2 src/clipping/test_clipping_colour.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o clipping_colour_test.exe
./clipping_colour_test.exe

g++ -Isrc -D_USE_MATH_DEFINES -O2 src/circles/test_lines_circles.cpp src/lines/lines.cpp src/polygons/polygons.cpp src/clipping/clipping.cpp src/circles/circles.cpp src/curves/curves.cpp src/colour/colour.cpp -o lines_circles_test.exe
./lines_circles_test.exe
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

---

