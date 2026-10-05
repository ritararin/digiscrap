# digiscrap

## Clipping & Colour

All rendering and colour operations are pure C++17, with no external graphics
libraries. Pixels are the team's `Color {r, g, b, a}` with 8-bit channels, stored
row-major in `Framebuffer::pixels`. BMP output is 24-bit BGR and discards alpha.

### Algorithms and APIs

- `clipLine`: Cohen-Sutherland. Outside region bits are left=1, right=2,
	below=4, above=8. Zero combined codes accept; a shared outside bit rejects.
	Otherwise intersect one outside endpoint with its violated edge and repeat.
- `clipLineLiangBarsky`: Liang-Barsky. For `P(t)=P0+t*(P1-P0)`, start with
	`[t0,t1]=[0,1]`. The four edges use `p={-dx,dx,-dy,dy}` and
	`q={x0-xmin,xmax-x0,y0-ymin,ymax-y0}`. Negative p raises t0; positive p
	lowers t1. Zero p rejects only if q is negative. Reject when t0 exceeds t1.
- Both line functions accept endpoint coordinates by reference and return bool.
	Acceptance overwrites them with the visible segment; rejection preserves them.
	Rectangular window edges are inclusive. Reversed bounds reject.
- `clipPolygon`: Sutherland-Hodgman, processing left/right/below/above in turn.
	Each closed polygon edge uses the previous and current vertex: in->in keeps
	current; in->out emits an intersection; out->out emits nothing; out->in emits
	the intersection then current. Fewer than three input vertices or reversed
	bounds return an empty list. The existing `polygonArea` shoelace helper is reused.
- `grayscale`: weighted luminance `0.299R + 0.587G + 0.114B`.
- `sepia`: matrix rows `(0.393,0.769,0.189)`, `(0.349,0.686,0.168)`,
	`(0.272,0.534,0.131)`, applied to the original RGB simultaneously.
- `tint`: `c*(1-s) + tint*s`; strength is clamped to [0,1].
- `brightness`: add delta. `contrast`: `128 + (c-128)*factor`; negative factors
	are treated as zero. `invert`: `255-c`.
- Filters round to the nearest integer, clamp RGB to [0,255], and preserve alpha.
- `alphaBlend(source,destination)`: straight-RGBA source-over. Over opaque
	destination, RGB is `src*a + dst*(1-a)` with `a=src.a/255`. For transparent
	destination, combine premultiplied contributions and divide by output alpha
	before storing straight RGB. Fully transparent source leaves destination intact.
- `applyFilter`: whole framebuffer or a clamped, half-open pixel rectangle
	`[x0,x1) x [y0,y1)`. Empty callbacks and empty/reversed regions do nothing.
	Parameterized filters can be passed as lambdas.

### Files and Integration

- `src/clipping/clipping.h` / `.cpp`: line and polygon clipping.
- `src/colour/colour.h` / `.cpp`: colour filters, source-over, image filtering.
- `src/clipping/test_clipping_colour.cpp`: framework-free self-checking tests.
- Shared edits: `CMakeLists.txt` and `Makefile` separate demo/test executables;
	`src/main.cpp` integrates filters/blending and checks output creation/saving;
	this README documents the work. `output/day_page.bmp` is regenerated.
- No changes to teammates' framebuffer, Mat3, scene, font, line, polygon,
	circle/ellipse or curve implementations.

The day page clips exactly one corner star against the card window
`(70,30)-(770,570)` before filling and outlining it. The procedural landscape
photo receives sepia only within `(140,100)-(460,360)`. The rotated square is
tinted towards green. Tape is drawn by the existing scanline filler onto
transparent temporary buffers and source-over blended onto the page, preserving
the existing replacement-only `setPixel` contract. Scene insertion order provides
layering; existing Mat3 transforms and line/shape drawing APIs are reused.

### Build and Run

Run from the `digiscrap` directory. With GNU make, a POSIX-compatible shell, and
a C++17 compiler (for example, a recent GCC):

```sh
make clean
make
make test
make run
```

CMake builds `scrapbook`, `polygons_test`, and `clipping_colour_test` separately.
Its `check` target builds/runs both registered tests; its `run` target builds/runs
the demo. In VS Code, open **digiscrap itself** as the folder, select an installed
compiler kit with **CMake: Select a Kit**, configure and build with CMake Tools,
then use **CMake: Build Target** for `check` or `run`. Tests are also registered
with CTest for the CMake Test Explorer. MSVC targets define `_USE_MATH_DEFINES`
for teammates' existing `M_PI` uses.

On this Windows machine, Visual Studio 2022 Build Tools provides MSVC, NMake,
CMake and Ninja, but they were initially not on PATH. GNU g++/make were not found.
The nested-workspace CMake Tools adapter could not configure; explicit CMake
configuration and the generated Visual Studio solution/check target were verified
with MSBuild instead. GNU Make execution remains unverified on this machine.
After a Release build, direct execution in PowerShell is also possible:

```powershell
.\build\Release\clipping_colour_test.exe
.\build\Release\polygons_test.exe
.\build\Release\scrapbook.exe
```

The demo writes `output/day_page.bmp`, `output/calendar.bmp`, and corresponding
PPMs. The polygon self-test writes `output/polygons_test.bmp`.

### Verification and Limitations

The clipping/colour self-test checks both line algorithms against the requested
inside/outside/crossing/corner-miss/edge/point cases, plus corner touches and
vertical lines. It compares visibility and endpoints (absolute tolerance 1e-8)
over 120,000 seeded random lines/windows, including horizontal/vertical/point
segments. Polygon checks cover unchanged inside vertices, empty outside output,
exact half-square area, all four edges, winding, and a concave star. Colour checks
cover luminance, tint endpoints, clamping, contrast, all-channel invert identity,
RGBA blending and whole-image/region filtering. Both registered test programs
passed in an MSVC Release build; the generated BMPs were visually inspected.

- Coordinates and floating-point filter parameters must be finite and of ordinary
	image-scale magnitude; extreme values, NaNs and infinities are not supported.
- Polygon input must be an ordered simple boundary. The single-list output does
	not represent holes or disconnected clipped components as separate contours.
	Convex polygons and the demo star are supported; boundary duplicates may occur.
- Colour arithmetic operates on stored RGB, not linear-light/gamma-corrected RGB.
	Filters preserve alpha; BMP transparency is flattened by blending before export.
- The photo is procedural, not an imported photograph.
- Teammate milestone gaps remain: the Bresenham entry point delegates to DDA,
	circle/ellipse outlines and curves are placeholders, and the bitmap font only
	renders digits/hyphens. Calendar letters therefore remain blank. These files
	have intentionally not been rewritten.

### Suggested GitHub Issues

1. Implement genuine all-octant Bresenham behind the existing line API.
2. Complete midpoint circle and ellipse outline algorithms.
3. Implement cubic Bezier and Catmull-Rom curve sampling.
4. Extend the bitmap font to uppercase letters for month/weekday labels.
5. Add GNU Make CI coverage and a documented Windows build setup.
6. Confirm whether the milestone requires an imported photograph; if so, add a
	 pure-C++ BMP/PPM loader rather than an external image library.