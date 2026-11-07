#pragma once
#include <cmath>
#include <vector>

struct Vec2 { double x, y; };

struct Mat3 {
    double m[3][3];

    // ---------- builders ----------
    static Mat3 identity()                      { return {{{1,0,0},{0,1,0},{0,0,1}}}; }
    static Mat3 translate(double tx, double ty) { return {{{1,0,tx},{0,1,ty},{0,0,1}}}; }
    static Mat3 scale(double sx, double sy)     { return {{{sx,0,0},{0,sy,0},{0,0,1}}}; }
    static Mat3 rotate(double rad) {
        double c = std::cos(rad), s = std::sin(rad);
        return {{{c,-s,0},{s,c,0},{0,0,1}}};
    }
    // Shear: x' = x + shx*y,  y' = y + shy*x
    static Mat3 shear(double shx, double shy)   { return {{{1,shx,0},{shy,1,0},{0,0,1}}}; }
    // Reflection across the x-axis (y -> -y) and the y-axis (x -> -x)
    static Mat3 reflectX()                      { return {{{1,0,0},{0,-1,0},{0,0,1}}}; }
    static Mat3 reflectY()                      { return {{{-1,0,0},{0,1,0},{0,0,1}}}; }
    // Reflection across a line through the origin at angle `rad` from the x-axis
    static Mat3 reflect(double rad) {
        double c = std::cos(2*rad), s = std::sin(2*rad);
        return {{{c,s,0},{s,-c,0},{0,0,1}}};
    }

    // Rotate/scale about a pivot, then move: the CP1 "sticker" transform
    static Mat3 about(Vec2 pivot, double rad, double s, Vec2 move) {
        return translate(move.x, move.y) * translate(pivot.x, pivot.y)
             * rotate(rad) * scale(s, s) * translate(-pivot.x, -pivot.y);
    }

    // ---------- operations ----------
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
    Vec2 operator*(Vec2 p) const { return apply(p); }   // M * point

    double determinant() const {
        return m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1])
             - m[0][1]*(m[1][0]*m[2][2] - m[1][2]*m[2][0])
             + m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0]);
    }
    bool invertible() const { return std::fabs(determinant()) > 1e-12; }

    // Inverse via cofactors (adjugate / determinant).
    // For a singular matrix this returns the identity: check invertible() first.
    Mat3 inverse() const {
        double det = determinant();
        if (std::fabs(det) <= 1e-12) return identity();
        double d = 1.0 / det;
        Mat3 r;
        r.m[0][0] =  (m[1][1]*m[2][2] - m[1][2]*m[2][1]) * d;
        r.m[0][1] = -(m[0][1]*m[2][2] - m[0][2]*m[2][1]) * d;
        r.m[0][2] =  (m[0][1]*m[1][2] - m[0][2]*m[1][1]) * d;
        r.m[1][0] = -(m[1][0]*m[2][2] - m[1][2]*m[2][0]) * d;
        r.m[1][1] =  (m[0][0]*m[2][2] - m[0][2]*m[2][0]) * d;
        r.m[1][2] = -(m[0][0]*m[1][2] - m[0][2]*m[1][0]) * d;
        r.m[2][0] =  (m[1][0]*m[2][1] - m[1][1]*m[2][0]) * d;
        r.m[2][1] = -(m[0][0]*m[2][1] - m[0][1]*m[2][0]) * d;
        r.m[2][2] =  (m[0][0]*m[1][1] - m[0][1]*m[1][0]) * d;
        return r;
    }
};

using Polygon = std::vector<Vec2>;

inline Polygon transformPolygon(const Polygon& p, const Mat3& M) {
    Polygon out; out.reserve(p.size());
    for (auto& v : p) out.push_back(M.apply(v));
    return out;
}