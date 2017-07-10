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
    remez_solver(int order, int decimals);
    ~remez_solver();

    void run(lol::real a, lol::real b,
             char const *func, char const *weight = nullptr);

    bool show_stats = false;

private:
    void remez_init();
    void remez_step();

    void find_zeroes();
    void find_extrema();

    void worker_thread();

    void print_poly();

    lol::real eval_estimate(lol::real const &x);
    lol::real eval_func(lol::real const &x);
    lol::real eval_weight(lol::real const &x);
    lol::real eval_error(lol::real const &x);

private:
    /* User-defined parameters */
    expression m_func, m_weight;
    int m_order, m_decimals;
    bool m_has_weight;

    /* Solver state */
    lol::polynomial<lol::real> m_estimate;

    lol::array<lol::real> m_zeroes;
    lol::array<lol::real> m_control;

    lol::real m_k1, m_k2, m_epsilon, m_error;

    struct point
    {
        lol::real x, err;
    };

    lol::array<point, point, point> m_zeroes_state;
    lol::array<point, point, point> m_extrema_state;

    /* Threading information */
    lol::array<lol::thread *> m_workers;
    lol::queue<int> m_questions, m_answers;
};

