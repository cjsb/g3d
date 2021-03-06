/**
  \file G3D-base.lib/include/G3D-base/Triangle.h

  \cite Random point method by  Greg Turk, Generating random points in triangles.  In A. S. Glassner, ed., Graphics Gems, pp. 24-28. Academic Press, 1990

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_Triangle_h
#define G3D_Triangle_h

#include "G3D-base/platform.h"
#include "G3D-base/g3dmath.h"
#include "G3D-base/Vector3.h"
#include "G3D-base/Plane.h"
#include "G3D-base/BoundsTrait.h"
#include "G3D-base/debugAssert.h"
#include "G3D-base/G3DString.h"
#include "G3D-base/Random.h"

namespace G3D {

/**
  A generic triangle representation.  This should not be used
  as the underlying triangle for creating models; it is intended
  for providing fast property queries but requires a lot of
  storage and is mostly immutable.
  */
class Triangle {
private:
    friend class CollisionDetection;
    friend class Ray;

    Vector3                     _vertex[3];

    /** edgeDirection[i] is the normalized vector v[i+1] - v[i] */
    Vector3                     edgeDirection[3];
    float                       edgeMagnitude[3];
    Plane                       _plane;
    Vector3::Axis               _primaryAxis;

    /** vertex[1] - vertex[0] */
    Vector3                     _edge01;

    /** vertex[2] - vertex[0] */
    Vector3                     _edge02;

    float                       _area;

    void init(const Vector3& v0, const Vector3& v1, const Vector3& v2);

public:
    
    explicit Triangle(class BinaryInput& b);
    void serialize(class BinaryOutput& b);
    void deserialize(class BinaryInput& b);

    Triangle();
    
    Triangle(const Point3& v0, const Point3& v1, const Point3& v2);
    
    ~Triangle();

    /** 0, 1, or 2 */
    inline const Point3& vertex(int n) const {
        debugAssert((n >= 0) && (n < 3));
        return _vertex[n];
    }

    /** Invert winding */
    Triangle otherSide() const {
        return Triangle(vertex(2), vertex(1), vertex(0));
    }

    /** vertex[1] - vertex[0] */
    inline const Vector3& edge01() const {
        return _edge01;
    }

    /** vertex[2] - vertex[0] */
    inline const Vector3& edge02() const {
        return _edge02;
    }

    float area() const;

    Vector3::Axis primaryAxis() const {
        return _primaryAxis;
    }

    const Vector3& normal() const;

    /** Barycenter */
    Point3 center() const;

    const Plane& plane() const;

    /** Returns a random point in the triangle. */
    Point3 randomPoint(Random& rnd = Random::common()) const;

    inline void getRandomSurfacePoint
    (Point3& P, 
     Vector3& N = Vector3::ignore,
     Random& rnd = Random::common()) const {
        P = randomPoint(rnd);
        N = normal();
    }

    /**
     For two triangles to be equal they must have
     the same vertices <I>in the same order</I>.
     That is, vertex[0] == vertex[0], etc.
     */
    inline bool operator==(const Triangle& other) const {
        for (int i = 0; i < 3; ++i) {
            if (_vertex[i] != other._vertex[i]) {
                return false;
            }
        }

        return true;
    }

    inline size_t hashCode() const {
        return
            _vertex[0].hashCode() +
            (_vertex[1].hashCode() >> 2) +
            (_vertex[2].hashCode() >> 3);
    }

    /** result[i] is the weight applied to vertex(i) when blending */
    Vector3 barycentric(const Point3& P) const;

    void getBounds(class AABox&) const;

    /**
       @brief Intersect the ray at distance less than @a distance.

       @param distance Set to the maximum distance (can be G3D::inf())
       to search for an intersection.  On return, this is the smaller
       of the distance to the intersection, if one exists, and the original
       value.
       
       @param baryCoord  If a triangle is hit before @a distance, a
       the barycentric coordinates of the hit location on the triangle.
       Otherwise, unmodified.

       @return True if there was an intersection before the original distance.
     */
    bool intersect(const class Ray& ray, float& distance, float baryCoord[3]) const;
};

} // namespace G3D

template <> struct HashTrait<G3D::Triangle> {
    static size_t hashCode(const G3D::Triangle& key) { return key.hashCode(); }
};


template<> struct BoundsTrait<class G3D::Triangle> {
    static void getBounds(const G3D::Triangle& t, G3D::AABox& out) { t.getBounds(out); }
};

#endif
