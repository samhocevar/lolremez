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

#pragma once

//
// The remez_solver class
// ----------------------
//

#include <cstdio>

#include "expression.h"

class remez_solver
{
public:
    remez_solver();
    ~remez_solver();

    enum class format
    {
        gnuplot,
        cpp,
    };

    void set_order(int order);
    void set_decimals(int decimals);
    void set_range(lol::real xmin, lol::real xmax);
    void set_func(char const *func);
    void set_weight(char const *weight);

    void do_init();
    bool do_step();
    void do_print(format fmt);

    bool show_stats = false;

private:
    void remez_init();
    void remez_step();

    void find_zeros();
    void find_extrema();

    void worker_thread();

    lol::real eval_estimate(lol::real const &x);
    lol::real eval_func(lol::real const &x);
    lol::real eval_weight(lol::real const &x);
    lol::real eval_error(lol::real const &x);

private:
    /* User-defined parameters */
    lol::String m_func_string, m_weight_string;
    expression m_func, m_weight;
    lol::real m_xmin = -lol::real::R_1();
    lol::real m_xmax = +lol::real::R_1();
    int m_order = 4;
    int m_decimals = 20;
    bool m_has_weight = false;

    /* Solver state */
    lol::polynomial<lol::real> m_estimate;

    lol::array<lol::real> m_zeros;
    lol::array<lol::real> m_control;

    lol::real m_k1, m_k2, m_epsilon, m_error;

    struct point
    {
        lol::real x, err;
    };

    lol::array<point, point, point> m_zeros_state;
    lol::array<point, point, point> m_extrema_state;

    /* Threading information */
    lol::array<lol::thread *> m_workers;
    lol::queue<int> m_questions, m_answers;
};

