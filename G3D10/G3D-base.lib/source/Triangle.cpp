/**
  \file G3D-base.lib/source/Triangle.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/platform.h"
#include "G3D-base/Triangle.h"
#include "G3D-base/Plane.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/debugAssert.h"
#include "G3D-base/AABox.h"
#include "G3D-base/Ray.h"
#include "G3D-base/Random.h"

namespace G3D {

    
void Triangle::init(const Vector3& v0, const Vector3& v1, const Vector3& v2) {

    _plane = Plane(v0, v1, v2);
    _vertex[0] = v0;
    _vertex[1] = v1;
    _vertex[2] = v2;

    static int next[] = {1,2,0};

    for (int i = 0; i < 3; ++i) {
        const Vector3& e  = _vertex[next[i]] - _vertex[i];
        edgeMagnitude[i]  = e.magnitude();

        if (edgeMagnitude[i] == 0) {
            edgeDirection[i] = Vector3::zero();
        } else {
            edgeDirection[i] = e / (float)edgeMagnitude[i];
        }
    }

    _edge01 = _vertex[1] - _vertex[0];
    _edge02 = _vertex[2] - _vertex[0];

    _primaryAxis = _plane.normal().primaryAxis();
    _area = 0.5f * edgeDirection[0].cross(edgeDirection[2]).magnitude() * (edgeMagnitude[0] * edgeMagnitude[2]);
        //0.5f * (_vertex[1] - _vertex[0]).cross(_vertex[2] - _vertex[0]).dot(_plane.normal());
}


Triangle::Triangle() {
    init(Vector3::zero(), Vector3::zero(), Vector3::zero());
}
    

Triangle::Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2) {
    init(v0, v1, v2);
}

    
Triangle::~Triangle() {
}


Triangle::Triangle(class BinaryInput& b) {
    deserialize(b);
}


void Triangle::serialize(class BinaryOutput& b) {
    _vertex[0].serialize(b);
    _vertex[1].serialize(b);
    _vertex[2].serialize(b);
}


void Triangle::deserialize(class BinaryInput& b) {
    _vertex[0].deserialize(b);
    _vertex[1].deserialize(b);
    _vertex[2].deserialize(b);
    init(_vertex[0], _vertex[1], _vertex[2]);
}


float Triangle::area() const {
    return _area;
}

const Vector3& Triangle::normal() const {
    return _plane.normal();
}


const Plane& Triangle::plane() const {
    return _plane;
}


Vector3 Triangle::center() const {
    return (_vertex[0] + _vertex[1] + _vertex[2]) / 3.0;
}


Vector3 Triangle::randomPoint(Random& rnd) const {
    // Choose a random point in the parallelogram

    float s = rnd.uniform();
    float t = rnd.uniform();

    if (t > 1.0f - s) {
        // Outside the triangle; reflect about the
        // diagonal of the parallelogram
        t = 1.0f - t;
        s = 1.0f - s;
    }

    return _edge01 * s + _edge02 * t + _vertex[0];
}


Vector3 Triangle::barycentric(const Point3& P) const {
    // Based on Christer Ericson's Real-Time Collision Detection, via
    // http://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
    const Vector3& v2 = P - _vertex[0];
    float d00 = _edge01.dot(_edge01);
    float d01 = _edge01.dot(_edge02);
    float d11 = _edge02.dot(_edge02);
    float d20 = v2.dot(_edge01);
    float d21 = v2.dot(_edge02);
    float scale = 1.0f / (d00 * d11 - d01 * d01);

    Vector3 b;
    b.y = (d11 * d20 - d01 * d21) * scale;
    b.z = (d00 * d21 - d01 * d20) * scale;
    b.x = 1.0f - b.y - b.z;
    return b;
}


void Triangle::getBounds(AABox& out) const {
    Vector3 lo = _vertex[0];
    Vector3 hi = lo;

    for (int i = 1; i < 3; ++i) {
        lo = lo.min(_vertex[i]);
        hi = hi.max(_vertex[i]);
    }

    out = AABox(lo, hi);
}


bool Triangle::intersect(const Ray& ray, float& distance, float baryCoord[3]) const {
    static const float EPS = 1e-5f;

    // See RTR2 ch. 13.7 for the algorithm.

    const Vector3& e1 = edge01();
    const Vector3& e2 = edge02();
    const Vector3 p(ray.direction().cross(e2));
    const float a = e1.dot(p);

    if (abs(a) < EPS) {
        // Determinant is ill-conditioned; abort early
        return false;
    }

    const float f = 1.0f / a;
    const Vector3 s(ray.origin() - vertex(0));
    const float u = f * s.dot(p);

    if ((u < 0.0f) || (u > 1.0f)) {
        // We hit the plane of the m_geometry, but outside the m_geometry
        return false;
    }

    const Vector3 q(s.cross(e1));
    const float v = f * ray.direction().dot(q);

    if ((v < 0.0f) || ((u + v) > 1.0f)) {
        // We hit the plane of the triangle, but outside the triangle
        return false;
    }

    const float t = f * e2.dot(q);

    if ((t > 0.0f) && (t < distance)) {
        // This is a new hit, closer than the previous one
        distance = t;

        baryCoord[0] = 1.0f - u - v;
        baryCoord[1] = u;
        baryCoord[2] = v;

        return true;
    } else {
        // This hit is after the previous hit, so ignore it
        return false;
    }
}

} // G3D
