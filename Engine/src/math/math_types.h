#pragma once

#include "defines.h"

typedef union Vector2_u{
    f32 elements[2];
    union{
        struct{ f32 x,y; };
        struct{ f32 r,g; };
        struct{ f32 s,t; };
        struct{ f32 u,v; };
    };
}Vector2;

typedef struct Vector3_u{
    union{
        f32 elements[3];
        union{
            struct{ f32 x,y,z; };
            struct{ f32 r,g,b; };
            struct{ f32 s,t,p; };
            struct{ f32 u,v,w; };
        };
    };
}Vector3;

typedef struct Vector4_u{
    f32 elements[4];
    union{
        struct{ f32 x, y, z, w; };
        struct{ f32 r, g, b, a; };
        struct{ f32 s, t, p, q; };
    };
}Vector4;

typedef Vector4 quaternion;

typedef union Matrix4_u{
    f32 data[16];
}Matrix4;