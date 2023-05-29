#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "aabb.h"


class material;

struct hit_record {
    vec3 p;         // point coordinates
    vec3 normal;    // normal vector
    double t;
    bool front_face;

    shared_ptr<material> mat_ptr;

    double u, v;    // used for texture

    inline void set_face_normal(const ray& r, const vec3& outward_nomal) {
        front_face = dot(r.direction(), outward_nomal) < 0;
        normal = front_face ? outward_nomal : -outward_nomal;
    }
};


class hittable {
    public:
    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const = 0;

    virtual bool bounding_box(double t0, double t1, aabb& output_box) const = 0;
};


class flip_face: public hittable{
    public:
        shared_ptr<hittable> ptr;

    public:
    flip_face(shared_ptr<hittable> p) : ptr(p) {}

    virtual bool bounding_box(double t0, double t1, aabb& output_box) const{
        return ptr->bounding_box(t0, t1, output_box);
    }

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const{
        if (!ptr->hit(r, t_min, t_max, rec))
            return false;

        rec.front_face = !rec.front_face;
        return true;
    }
};


class translate : public hittable {
    public:
        shared_ptr<hittable> ptr;
        vec3 offset;

    public:
    translate() {}

    translate(shared_ptr<hittable> p, const vec3& displacement)
        : ptr(p), offset(displacement) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        ray moved_r(r.origin() - offset, r.direction(), r.time());
        if (!ptr->hit(r, t_min, t_max, rec))
            return false;

        rec.p += offset;
        rec.set_face_normal(moved_r, rec.normal);

        return true;
    }

    virtual bool bounding_box(double t0, double t1, aabb& output_box) const {
        if (!ptr->bounding_box(t0, t1, output_box))
            return false;

        output_box = aabb(
            output_box.min() + offset,
            output_box.max() + offset
        );
        return true;
    }
};


class rotate_y : public hittable {
    public:
        shared_ptr<hittable> ptr;
        double sin_theta;
        double cos_theta;
        bool hasbox;
        aabb bbox;

    public:
    rotate_y(shared_ptr<hittable> p, double angle);

    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const;

    virtual bool bounding_box(double t0, double t1, aabb& output_box) const {
        output_box = bbox;
        return false;
    }
};

rotate_y::rotate_y(shared_ptr<hittable> p, double angle) : ptr(p)  {
    auto radians = pi / 180 * angle;
    sin_theta = sin(radians);
    cos_theta = cos(radians);
    hasbox = ptr->bounding_box(0, 1, bbox);

    vec3 min( infinity,  infinity,  infinity);
    vec3 max(-infinity, -infinity, -infinity);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                auto x = i * bbox.max().x() + (1 - i) * bbox.min().x();
                auto y = j * bbox.max().y() + (1 - j) * bbox.min().y();
                auto z = k * bbox.max().z() + (1 - k) * bbox.min().z();

                auto newx = cos_theta * x + sin_theta * z;
                auto newz = -sin_theta * x + cos_theta * z;

                vec3 tester(newx, y, newz);

                for (int c = 0; c < 3; c++) {
                    min[c] = ffmin(min[c], tester[c]);
                    max[c] = ffmax(max[c], tester[c]);
                }
            }
        }
    }
    bbox = aabb(min, max);
}

bool rotate_y::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    vec3 origin = r.origin();
    vec3 direction = r.direction();

    origin[0] = cos_theta * r.origin()[0] - sin_theta * r.origin()[2];
    origin[2] = sin_theta * r.origin()[0] + cos_theta * r.origin()[2];

    direction[0] = cos_theta * r.direction()[0] - sin_theta * r.direction()[2];
    direction[2] = sin_theta * r.direction()[0] + cos_theta * r.direction()[2];

    ray rotated_r(origin, direction, r.time());

    if (!ptr->hit(rotated_r, t_min, t_max, rec))
        return false;

    vec3 p = rec.p;
    vec3 normal = rec.normal;

    p[0] = cos_theta * rec.p[0] + sin_theta * rec.p[2];
    p[2] = -sin_theta * rec.p[0] + cos_theta * rec.p[2];

    normal[0] = cos_theta * rec.normal[0] + sin_theta * rec.normal[2];
    normal[2] = -sin_theta * rec.normal[0] + cos_theta * rec.normal[2];

    rec.p = p;
    rec.set_face_normal(rotated_r, normal);

    return true;
}

#endif