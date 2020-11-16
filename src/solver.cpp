//
//  LolRemez — Remez algorithm implementation
//
//  Copyright © 2005—2020 Sam Hocevar <sam@hocevar.net>
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
#include <iostream>
#include <iomanip>

#include <lol/thread>
#include <lol/real>
#include <lol/math>

#include "matrix.h"
#include "solver.h"

using lol::real;

remez_solver::remez_solver()
{
    /* Spawn 4 worker threads */
    for (int i = 0; i < 4; ++i)
    {
        auto th = new thread(std::bind(&remez_solver::worker_thread, this));
        m_workers.push_back(th);
    }
}

remez_solver::~remez_solver()
{
    /* Signal worker threads to quit, wait for worker threads to answer,
     * and kill worker threads. */
    for (auto worker : m_workers)
        (void)worker, m_questions.push(-1);

    for (auto worker : m_workers)
        (void)worker, m_answers.pop();

    for (auto worker : m_workers)
        delete worker;
}

void remez_solver::set_order(int order)
{
    m_order = order;
}

void remez_solver::set_digits(int digits)
{
    m_digits = digits;
}

void remez_solver::set_range(real a, real b)
{
    m_xmin = a;
    m_xmax = b;
}

void remez_solver::set_func(expression const &expr)
{
    m_func = expr;
}

void remez_solver::set_weight(expression const &expr)
{
    m_has_weight = !expr.is_constant();
    m_weight = expr;
}

void remez_solver::set_root_finder(root_finder rf)
{
    m_rf = rf;
}

void remez_solver::do_init()
{
    m_k1 = (m_xmax + m_xmin) / 2;
    m_k2 = (m_xmax - m_xmin) / 2;
    m_epsilon = pow((real)10, (real)-(m_digits + 2));

    if (show_debug)
        std::cout << std::setprecision(m_digits) << "[debug] k1: " << m_k1
                  << " k2: " << m_k2 << " epsilon: " << m_epsilon << '\n';

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

polynomial<real> remez_solver::get_estimate() const
{
    /* Transform our polynomial in the [-1..1] range into a polynomial
     * in the [a..b] range by composing it with the following polynomial:
     *  q(x) = 2x / (b-a) - (b+a) / (b-a) */
    polynomial<real> q ({ -m_k1 / m_k2, real(1) / m_k2 });
    return m_estimate.eval(q);
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
    m_extrema_state.resize(m_order + 2);

    /* Initial estimates for the x_i where the error will be zero and
     * precompute f(x_i). */
    std::vector<real> fxn;
    for (int i = 0; i < m_order + 1; i++)
    {
        m_zeros[i] = (real)(2 * i - m_order) / (real)(m_order + 1);
        fxn.push_back(eval_func(m_zeros[i]));
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
    timer t;

    /* Pick up x_i where error will be 0 and compute f(x_i) */
    std::vector<real> fxn;
    for (int i = 0; i < m_order + 2; i++)
        fxn.push_back(eval_func(m_control[i]));

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
        std::cout << " -:- timing for inversion: " << (t.get() * 1000.f) << " ms\n";
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
    timer t;

    /* Initialise an [a,b] bracket for each zero we try to find */
    for (int i = 0; i < m_order + 1; i++)
    {
        point &a = m_zeros_state[i][0];
        point &b = m_zeros_state[i][1];
        point &c = m_zeros_state[i][1];

        a.x = m_control[i];
        a.err = eval_estimate(a.x) - eval_func(a.x);
        b.x = m_control[i + 1];
        b.err = eval_estimate(b.x) - eval_func(b.x);
        c.err = 0;

        m_questions.push(i);
    }

    /* Watch all brackets for updates from worker threads */
    for (int finished = 0; finished < m_order + 1; )
    {
        int i = m_answers.pop();

        point const &a = m_zeros_state[i][0];
        point const &b = m_zeros_state[i][1];
        point const &c = m_zeros_state[i][2];

        if (c.err.is_zero() || fabs(a.x - b.x) <= m_epsilon)
        {
            m_zeros[i] = c.x;
            ++finished;
            continue;
        }

        m_questions.push(i);
    }

    if (show_stats)
        std::cout << " -:- timing for zeros: " << (t.get() * 1000.f) << " ms\n";
}


// Find m_order + 2 extrema of the error function. We maximise the relative
// error, since its extrema are at slightly different locations than the
// absolute error’s.
//
// If the weight function is 1, we would only need to compute m_order extrema
// because we already know that -1 and +1 are extrema. However when weighing
// the error the exact extrema locations get slightly moved around.
//
// The algorithm used here is successive parabolic interpolation. FIXME: we
// could use Brent’s method instead, which combines parabolic interpolation
// and golden ratio search and has superlinear convergence.
void remez_solver::find_extrema()
{
    timer t;

    m_control[0] = -1;
    m_control[m_order + 1] = 1;
    m_error = 0;

    /* Initialise an [a,b,c] bracket for each extremum we try to find */
    for (int i = 0; i < m_order + 2; i++)
    {
        point &a = m_extrema_state[i][0];
        point &b = m_extrema_state[i][1];
        point &c = m_extrema_state[i][2];

        a.x = i == 0 ? (real)-1 : m_zeros[i - 1];
        b.x = i == m_order + 1 ? (real)1 : m_zeros[i];
        c.x = a.x + (b.x - a.x) * real(rand(0.4f, 0.6f));

        a.err = eval_error(a.x);
        b.err = eval_error(b.x);
        c.err = eval_error(c.x);

        m_questions.push(i + 1000);
    }

    /* Watch all brackets for updates from worker threads */
    for (int finished = 0; finished < m_order + 2; )
    {
        int i = m_answers.pop() - 1000;

        point const &a = m_extrema_state[i][0];
        point const &b = m_extrema_state[i][1];
        point const &c = m_extrema_state[i][2];

        if (b.x - a.x <= m_epsilon)
        {
            m_control[i] = c.x;
            if (c.err > m_error)
                m_error = c.err;
            ++finished;
            continue;
        }

        m_questions.push(i + 1000);
    }

    if (show_stats)
        std::cout << " -:- timing for extrema: " << (t.get() * 1000.f) << " ms\n";

    if (show_debug)
        std::cout << "[debug] error: " << std::setprecision(m_digits) << m_error << "\n";
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

// Worker threads handle jobs from the main thread, computing either a root finding step
// or an extrema finding iteration step.
void remez_solver::worker_thread()
{
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
            // Root finding step
            point &a = m_zeros_state[i][0];
            point &b = m_zeros_state[i][1];
            point &c = m_zeros_state[i][2];

            auto old_c_err = c.err;

            // Bisect method uses the midpoint. Other methods such as regula falsi (slow) and
            // some improved versions use the “false position”.
            if (m_rf == root_finder::bisect)
                c.x = (a.x + b.x) / 2;
            else
                c.x = a.x - a.err * (b.x - a.x) / (b.err - a.err);
            c.err = eval_estimate(c.x) - eval_func(c.x);

            // pd is the point with a different error sign from c, ps has same sign
            point *pd = &a, *ps = &b;
            if (sign(a.err) * sign(c.err) > 0)
                std::swap(pd, ps);

            // Regula falsi variations tweak a.err or b.err for the next iteration
            // when the computed error has the same sign as the last time.
            if (sign(c.err) * sign(old_c_err) > 0)
            {
                switch (m_rf)
                {
                case root_finder::illinois:
                    // Illinois algorithm
                    pd->err /= 2;
                    break;
                case root_finder::pegasus:
                    // Pegasus algorithm from doi:10.1007/BF01932959 by M. Dowell and P. Jarratt.
                    // “The philosophy of the method is to scale down the value fi-1 by the factor
                    // fi/(fi+fi+1) […]”.
                    pd->err *= old_c_err / (old_c_err + c.err);
                    break;
                case root_finder::ford:
                    // Method 4 of https://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.53.8676
                    // by J. A. Ford
                    pd->err *= real::R_1() - c.err / ps->err - c.err / pd->err;
                    break;
                }
            }

            // Either a or b becomes c
            *ps = c;

            m_answers.push(i);
        }
        else if (i < 2000)
        {
            // Extrema finding step
            i -= 1000;

            point &a = m_extrema_state[i][0];
            point &b = m_extrema_state[i][1];
            point &c = m_extrema_state[i][2];
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

            m_answers.push(i + 1000);
        }
    }
}

