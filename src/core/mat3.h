#pragma once
#include <cmath>
#include <vector>

struct Vec2 { double x, y; };

struct Mat3 {
    double m[3][3];
    static Mat3 identity()                     { return {{{1,0,0},{0,1,0},{0,0,1}}}; }
    static Mat3 translate(double tx, double ty){ return {{{1,0,tx},{0,1,ty},{0,0,1}}}; }
    static Mat3 scale(double sx, double sy)    { return {{{sx,0,0},{0,sy,0},{0,0,1}}}; }
    static Mat3 rotate(double rad) {
        double c = std::cos(rad), s = std::sin(rad);
        return {{{c,-s,0},{s,c,0},{0,0,1}}};
    }
    Mat3 operator*(const Mat3& o) const {
        Mat3 r{};
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++) r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }
    Vec2 apply(Vec2 p) const {
        return { m[0][0]*p.x + m[0][1]*p.y + m[0][2],
                 m[1][0]*p.x + m[1][1]*p.y + m[1][2] };
    }
    // Rotate/scale about a pivot, then move: the "sticker" transform
    static Mat3 about(Vec2 pivot, double rad, double s, Vec2 move) {
        return translate(move.x, move.y) * translate(pivot.x, pivot.y)
             * rotate(rad) * scale(s, s) * translate(-pivot.x, -pivot.y);
    }
};

using Polygon = std::vector<Vec2>;

inline Polygon transformPolygon(const Polygon& p, const Mat3& M) {
    Polygon out; out.reserve(p.size());
    for (auto& v : p) out.push_back(M.apply(v));
    return out;
}