//
//  LolRemez — Remez algorithm implementation
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

#include <functional>

#include <lol/engine.h>

#include <lol/math/real.h>
#include <lol/math/polynomial.h>

#include "matrix.h"
#include "solver.h"

using lol::real;

remez_solver::remez_solver()
{
    /* Spawn 4 worker threads */
    for (int i = 0; i < 4; ++i)
    {
        auto th = new thread(std::bind(&remez_solver::worker_thread, this));
        m_workers.push(th);
    }
}

remez_solver::~remez_solver()
{
    /* Signal worker threads to quit, wait for worker threads to answer,
     * and kill worker threads. */
    for (auto worker : m_workers)
        UNUSED(worker), m_questions.push(-1);

    for (auto worker : m_workers)
        UNUSED(worker), m_answers.pop();

    for (auto worker : m_workers)
        delete worker;
}

void remez_solver::set_order(int order)
{
    m_order = order;
}

void remez_solver::set_decimals(int decimals)
{
    m_decimals = decimals;
}

void remez_solver::set_range(real a, real b)
{
    m_xmin = a;
    m_xmax = b;
}

void remez_solver::set_func(char const *func)
{
    m_func_string = func;
    m_func.parse(func);
}

void remez_solver::set_weight(char const *weight)
{
    if (weight)
    {
        m_weight_string = weight;
        m_weight.parse(weight);
    }
    m_has_weight = !!weight;
}

void remez_solver::do_init()
{
    m_k1 = (m_xmax + m_xmin) / 2;
    m_k2 = (m_xmax - m_xmin) / 2;
    m_epsilon = pow((real)10, (real)-(m_decimals + 2));

    remez_init();
}

bool remez_solver::do_step()
{
    real const old_error = m_error;

    find_extrema();
    remez_step();

    if (m_error >= (real)0
         && fabs(m_error - old_error) < m_error * m_epsilon)
        return false;

    find_zeros();
    return true;
}

/*
 * This is basically the first Remez step: we solve a system of
 * order N+1 and get a good initial polynomial estimate.
 */
void remez_solver::remez_init()
{
    /* m_order + 1 zeros of the error function */
    m_zeros.resize(m_order + 1);

    /* m_order + 1 zeros to find */
    m_zeros_state.resize(m_order + 1);

    /* m_order + 2 control points */
    m_control.resize(m_order + 2);

    /* m_order extrema to find */
    m_extrema_state.resize(m_order);

    /* Initial estimates for the x_i where the error will be zero and
     * precompute f(x_i). */
    array<real> fxn;
    for (int i = 0; i < m_order + 1; i++)
    {
        m_zeros[i] = (real)(2 * i - m_order) / (real)(m_order + 1);
        fxn.push(eval_func(m_zeros[i]));
    }

    /* We build a matrix of Chebyshev evaluations: row i contains the
     * evaluations of x_i for polynomial order n = 0, 1, ... */
    linear_system<real> system(m_order + 1);
    for (int n = 0; n < m_order + 1; n++)
    {
        auto p = polynomial<real>::chebyshev(n);
        for (int i = 0; i < m_order + 1; i++)
            system[i][n] = p.eval(m_zeros[i]);
    }

    /* Solve the system */
    system = system.inverse();

    /* Compute new Chebyshev estimate */
    m_estimate = polynomial<real>();
    for (int n = 0; n < m_order + 1; n++)
    {
        real weight = 0;
        for (int i = 0; i < m_order + 1; i++)
            weight += system[n][i] * fxn[i];

        m_estimate += weight * polynomial<real>::chebyshev(n);
    }
}

/*
 * Every subsequent iteration of the Remez algorithm: we solve a system
 * of order N+2 to both refine the estimate and compute the error.
 */
void remez_solver::remez_step()
{
    Timer t;

    /* Pick up x_i where error will be 0 and compute f(x_i) */
    array<real> fxn;
    for (int i = 0; i < m_order + 2; i++)
        fxn.push(eval_func(m_control[i]));

    /* We build a matrix of Chebyshev evaluations: row i contains the
     * evaluations of x_i for polynomial order n = 0, 1, ... */
    linear_system<real> system(m_order + 2);
    for (int n = 0; n < m_order + 1; n++)
    {
        auto p = polynomial<real>::chebyshev(n);
        for (int i = 0; i < m_order + 2; i++)
            system[i][n] = p.eval(m_control[i]);
    }

    /* The last line of the system is the oscillating error */
    for (int i = 0; i < m_order + 2; i++)
    {
        real const error = fabs(eval_weight(m_control[i]));
        system[i][m_order + 1] = (i & 1) ? error : -error;
    }

    /* Solve the system */
    system = system.inverse();

    /* Compute new polynomial estimate */
    m_estimate = polynomial<real>();
    for (int n = 0; n < m_order + 1; n++)
    {
        real weight = 0;
        for (int i = 0; i < m_order + 2; i++)
            weight += system[n][i] * fxn[i];

        m_estimate += weight * polynomial<real>::chebyshev(n);
    }

    /* Compute the error (FIXME: unused?) */
    real error = 0;
    for (int i = 0; i < m_order + 2; i++)
        error += system[m_order + 1][i] * fxn[i];

    if (show_stats)
    {
        using std::printf;
        printf(" -:- timing for inversion: %f ms\n", t.Get() * 1000.f);
    }
}

/*
 * Find m_order + 1 zeros of the error function. No need to compute the
 * relative error: its zeros are at the same place as the absolute error!
 *
 * The algorithm used here is the midpoint algorithm, which is guaranteed
 * to converge in fixed time. I tried naïve regula falsi which sometimes
 * would be very efficient, but it often performed poorly.
 */
void remez_solver::find_zeros()
{
    Timer t;

    static real const limit = real("1e-150");
    static real const zero = real("0");

    /* Initialise an [a,b] bracket for each zero we try to find */
    for (int i = 0; i < m_order + 1; i++)
    {
        point &a = m_zeros_state[i].m1;
        point &b = m_zeros_state[i].m2;

        a.x = m_control[i];
        a.err = eval_estimate(a.x) - eval_func(a.x);
        b.x = m_control[i + 1];
        b.err = eval_estimate(b.x) - eval_func(b.x);

        m_questions.push(i);
    }

    /* Watch all brackets for updates from worker threads */
    for (int finished = 0; finished < m_order + 1; )
    {
        int i = m_answers.pop();

        point const &a = m_zeros_state[i].m1;
        point const &b = m_zeros_state[i].m2;
        point const &c = m_zeros_state[i].m3;

        if (c.err == zero || fabs(a.x - b.x) <= limit)
        {
            m_zeros[i] = c.x;
            ++finished;
            continue;
        }

        m_questions.push(i);
    }

    if (show_stats)
    {
        using std::printf;
        printf(" -:- timing for zeros: %f ms\n", t.Get() * 1000.f);
    }
}

/*
 * Find m_order extrema of the error function. We maximise the relative
 * error, since its extrema are at slightly different locations than the
 * absolute error’s.
 *
 * The algorithm used here is successive parabolic interpolation. FIXME: we
 * could use Brent’s method instead, which combines parabolic interpolation
 * and golden ratio search and has superlinear convergence. However, the
 * real bottleneck for now is the root finding, so this has low priority.
 */
void remez_solver::find_extrema()
{
    Timer t;

    m_control[0] = -1;
    m_control[m_order + 1] = 1;
    m_error = 0;

    /* Initialise an [a,b,c] bracket for each extremum we try to find */
    for (int i = 0; i < m_order; i++)
    {
        point &a = m_extrema_state[i].m1;
        point &b = m_extrema_state[i].m2;
        point &c = m_extrema_state[i].m3;

        a.x = m_zeros[i];
        b.x = m_zeros[i + 1];
        c.x = a.x + (b.x - a.x) * real(rand(0.4f, 0.6f));

        a.err = eval_error(a.x);
        b.err = eval_error(b.x);
        c.err = eval_error(c.x);

        m_questions.push(i + 1000);
    }

    /* Watch all brackets for updates from worker threads */
    for (int finished = 0; finished < m_order; )
    {
        int i = m_answers.pop();

        point const &a = m_extrema_state[i - 1000].m1;
        point const &b = m_extrema_state[i - 1000].m2;
        point const &c = m_extrema_state[i - 1000].m3;

        static real const limit = real("1e-150");

        if (b.x - a.x <= limit)
        {
            m_control[i - 1000 + 1] = c.x;
            if (c.err > m_error)
                m_error = c.err;
            ++finished;
            continue;
        }

        m_questions.push(i);
    }

    if (show_stats)
    {
        using std::printf;
        printf(" -:- timing for extrema: %f ms\n", t.Get() * 1000.f);
        printf(" -:- error: ");
        m_error.print(m_decimals);
        printf("\n");
    }
}

void remez_solver::do_print(remez_solver::format fmt)
{
    /* Transform our polynomial in the [-1..1] range into a polynomial
     * in the [a..b] range by composing it with q:
     *  q(x) = 2x / (b-a) - (b+a) / (b-a) */
    polynomial<real> q ({ -m_k1 / m_k2, real(1) / m_k2 });
    polynomial<real> r = m_estimate.eval(q);

    using std::printf;

    switch (fmt)
    {
    case remez_solver::format::gnuplot:
        for (int j = 0; j < m_order + 1; j++)
        {
            printf(j > 0 && r[j] >= real::R_0() ? "+" : "");
            r[j].print(m_decimals);
            printf(j == 0 ? "" : j > 1 ? "*x**%d" : "*x", j);
        }
        printf("\n");
        break;
    case remez_solver::format::cpp:
        printf("/* Approximation of f(x) = %s\n", m_func_string.C());
        if (m_has_weight)
            printf(" * with weight function g(x) = %s\n", m_weight_string.C());
        printf(" * on interval [ ");
        m_xmin.print(m_decimals);
        printf(", ");
        m_xmax.print(m_decimals);
        printf(" ]\n * with a polynomial of degree %d. */\n", m_order);
        printf("float f(float x)\n{\n");
        for (int j = m_order; j >= 0; --j)
        {
            char const *a = j ? "u = u * x +" : "return u * x +";
            printf("    %s ", j == m_order ? "float u =" : a);
            r[j].print(m_decimals);
            printf("f;\n");
        }
        printf("}\n");
        break;
    }
}

real remez_solver::eval_estimate(real const &x)
{
    return m_estimate.eval(x);
}

real remez_solver::eval_func(real const &x)
{
    return m_func.eval(x * m_k2 + m_k1);
}

real remez_solver::eval_weight(real const &x)
{
    return m_has_weight ? m_weight.eval(x * m_k2 + m_k1) : real(1);
}

real remez_solver::eval_error(real const &x)
{
    return fabs((eval_estimate(x) - eval_func(x)) / eval_weight(x));
}

void remez_solver::worker_thread()
{
    static real const zero = (real)0;

    for (;;)
    {
        int i = m_questions.pop();

        if (i < 0)
        {
            m_answers.push(i);
            break;
        }
        else if (i < 1000)
        {
            point &a = m_zeros_state[i].m1;
            point &b = m_zeros_state[i].m2;
            point &c = m_zeros_state[i].m3;

#if 0
            /* This (regula falsi) is actually really slow */
            real s = abs(b.err) / (abs(a.err) + abs(b.err));
            real newc = b.x + s * (a.x - b.x);

            /* If the third point didn't change since last iteration,
             * we may be at an inflection point. Use the midpoint to get
             * out of this situation. */
            c.x = newc != c.x ? newc : (a.x + b.x) / 2;
#else
            c.x = (a.x + b.x) / 2;
#endif
            c.err = eval_estimate(c.x) - eval_func(c.x);

            if ((a.err < zero && c.err < zero)
                 || (a.err > zero && c.err > zero))
                a = c;
            else
                b = c;

            m_answers.push(i);
        }
        else if (i < 2000)
        {
            point &a = m_extrema_state[i - 1000].m1;
            point &b = m_extrema_state[i - 1000].m2;
            point &c = m_extrema_state[i - 1000].m3;
            point d;

            real const d1 = c.x - a.x, d2 = c.x - b.x;
            real const k1 = d1 * (c.err - b.err);
            real const k2 = d2 * (c.err - a.err);
            d.x = c.x - (d1 * k1 - d2 * k2) / (k1 - k2) / 2;

            /* If parabolic interpolation failed, pick a number
             * inbetween. */
            if (d.x <= a.x || d.x >= b.x)
                d.x = (a.x + b.x) / 2;

            d.err = eval_error(d.x);

            /* Update bracketing depending on the new point. */
            if (d.err < c.err)
            {
                (d.x > c.x ? b : a) = d;
            }
            else
            {
                (d.x > c.x ? a : b) = c;
                c = d;
            }

            m_answers.push(i);
        }
    }
}

