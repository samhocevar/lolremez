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

struct solver
{
    solver(int grid)
      : m_grid(grid)
    {
        for (int i = 0; i <= grid; ++i)
            m_coeff.push(cheb(i, grid));
    }

    void step()
    {
        rvec2 best(0);
        real max_abs = real::R_0();

        for (auto const &y : m_coeff)
        for (auto const &x : m_coeff)
        {
            real ret = abs(e_n(m_pivots.count(), x, y));
            if (ret > max_abs)
            {
                best = rvec2(x,y);
                max_abs = ret;
            }
        }
        m_pivots.push(rvec3(best, real::R_1() / e_n(m_pivots.count(), best.x, best.y)));
    }

    void dump_gnuplot()
    {
        printf("f(x,y)=sin((1-x)/2*acos((1+y)/2))/sqrt(1-((y+1)/2)**2)\n");
        printf("e0(x,y)=f(x,y)\n");
        //printf("f0(x,y)=0\n");

        for (int n = 0; n < m_pivots.count(); ++n)
        {
            printf("x%d=", n+1); m_pivots[n].x.print(20); printf("\n");
            printf("y%d=", n+1); m_pivots[n].y.print(20); printf("\n");
            printf("d%d=e%d(x%d,y%d)\n", n+1, n, n+1, n+1);
            printf("e%d(x,y)=e%d(x,y)-e%d(x%d,y)*e%d(x,y%d)/d%d\n", n+1, n, n, n+1, n, n+1, n+1);
            //printf("f%d(x,y)=f%d(x,y)+e%d(x%d,y)*e%d(x,y%d)/d%d\n", n+1, n, n, n+1, n, n+1, n+1);
        }

        printf("splot [-1:1][-1:1] e%d(x,y)\n", m_pivots.count());
    }

private:
    real d_n(int i, real const &x, real const &y)
    {
        return e_n(i, x, m_pivots[i].y) * e_n(i, m_pivots[i].x, y) * m_pivots[i].z;
    }

    real e_n(int i, real const &x, real const &y)
    {
        return i == 0 ? f(x, y) : e_n(i - 1, x, y) - d_n(i - 1, x, y);
    }

    real f_n(int i, real const &x, real const &y)
    {
        return i == 0 ? real::R_0() : f_n(i - 1, x, y) + d_n(i - 1, x, y);
    }

    real cheb(int i, int n)
    {
        return -cos(real::R_PI() * i / n) * real("0.999999999999999");
    }

    int m_grid;
    array<real> m_coeff;
    array<rvec3> m_pivots;
};

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    int const iters = 6;

    /* Create solver and iterate */
    solver s(33);
    for (int n = 0; n < iters; ++n)
        s.step();

    /* Dump gnuplot data about the solver */
    s.dump_gnuplot();

    return EXIT_SUCCESS;
}

