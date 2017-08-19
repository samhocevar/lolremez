//
//  Lol Engine — Sample math program: polynomials
//
//  Copyright © 2005—2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <cstdio>

#include <lol/engine.h>

using namespace lol;

real cheb(int i, int n)
{
    return -cos(real::R_PI() * i / n) * real("0.999999999999999");
}

real f(real const &x, real const &y)
{
    static array<real,real,real> cache;
    static real const one = real::R_1();

    for (auto const &v : cache)
        if (x == v.m1 && y == v.m2)
            return v.m3;

    //return exp(sin(x+x+x)+cos(y-one/4));
    real f = (x + one) / 2;
    real d = (y + one) / 2;
    real ret = sin((one - f) * acos(d)) / sqrt(one - d * d);
    cache.push(x, y, ret);
    return ret;
}

real x_1, y_1;
real x_2, y_2;
real x_3, y_3;
real x_4, y_4;
real x_5, y_5;
real x_6, y_6;

real e0(real const &x, real const &y) { return f(x,y); }
real f0(real const &x, real const &y) { return real::R_0(); }

real d1(real const &x, real const &y) { return e0(x,y_1) * e0(x_1,y) / e0(x_1,y_1); }
real e1(real const &x, real const &y) { return e0(x,y) - d1(x,y); }
real f1(real const &x, real const &y) { return f0(x,y) + d1(x,y); }

real d2(real const &x, real const &y) { return e1(x,y_2) * e1(x_2,y) / e1(x_2,y_2); }
real e2(real const &x, real const &y) { return e1(x,y) - d2(x,y); }
real f2(real const &x, real const &y) { return f1(x,y) + d2(x,y); }

real d3(real const &x, real const &y) { return e2(x,y_3) * e2(x_3,y) / e2(x_3,y_3); }
real e3(real const &x, real const &y) { return e2(x,y) - d3(x,y); }
real f3(real const &x, real const &y) { return f2(x,y) + d3(x,y); }

real d4(real const &x, real const &y) { return e3(x,y_4) * e3(x_4,y) / e3(x_4,y_4); }
real e4(real const &x, real const &y) { return e3(x,y) - d4(x,y); }
real f4(real const &x, real const &y) { return f3(x,y) + d4(x,y); }

real d5(real const &x, real const &y) { return e4(x,y_5) * e4(x_5,y) / e4(x_5,y_5); }
real e5(real const &x, real const &y) { return e4(x,y) - d5(x,y); }
real f5(real const &x, real const &y) { return f4(x,y) + d5(x,y); }

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    int const grid = 33;

    array<real> coeff;
    for (int i = 0; i <= grid; ++i)
        coeff.push(cheb(i, grid));
    real max_abs;

    printf("e0(x,y)=f(x,y)\n");
    printf("f0(x,y)=0\n");

#define DO(n0,n1) \
    max_abs = real::R_0(); \
    for (auto const &y : coeff) \
    for (auto const &x : coeff) \
    { \
        real ret = abs(e##n0(x, y)); \
        if (ret > max_abs) { x_##n1 = x; y_##n1 = y; max_abs = ret; } \
    } \
    printf("x%d=", n1); x_##n1.print(20); printf("\n"); \
    printf("y%d=", n1); y_##n1.print(20); printf("\n"); \
    printf("tmp="); max_abs.print(20); printf("\n"); \
    printf("d"#n1"=e"#n0"(x"#n1",y"#n1")\n"); \
    printf("e"#n1"(x,y)=e"#n0"(x,y)-e"#n0"(x"#n1",y)*e"#n0"(x,y"#n1")/d"#n1"\n"); \
    printf("f"#n1"(x,y)=f"#n0"(x,y)+e"#n0"(x"#n1",y)*e"#n0"(x,y"#n1")/d"#n1"\n");

    DO(0,1);
    DO(1,2);
    DO(2,3);
    DO(3,4);
    DO(4,5);
    DO(5,6);

    printf("splot e6(x,y)\n");

    return EXIT_SUCCESS;
}

