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

#pragma once

//
// The remez_solver class
// ----------------------
//

#include <lol/thread>
#include <lol/math>
#include <lol/real>

#include <vector>
#include <array>

#include "expression.h"

enum class root_finder
{
    bisect,
    regula_falsi,
    illinois,
    pegasus,
    ford,
};

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
    void set_digits(int digits);
    void set_range(lol::real xmin, lol::real xmax);
    void set_func(expression const &expr);
    void set_weight(expression const &expr);
    void set_root_finder(root_finder rf);

    void do_init();
    bool do_step();

    lol::polynomial<lol::real> get_estimate() const;

    bool show_stats = false;
    bool show_debug = false;

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
    expression m_func, m_weight;
    lol::real m_xmin = -lol::real::R_1();
    lol::real m_xmax = +lol::real::R_1();
    int m_order = 4;
    int m_digits = 40;
    bool m_has_weight = false;
    root_finder m_rf = root_finder::pegasus;

    /* Solver state */
    lol::polynomial<lol::real> m_estimate;

    std::vector<lol::real> m_zeros;
    std::vector<lol::real> m_control;

    lol::real m_k1, m_k2, m_epsilon, m_error;

    struct point
    {
        lol::real x, err;
    };

    std::vector<std::array<point, 3>> m_zeros_state;
    std::vector<std::array<point, 3>> m_extrema_state;

    /* Threading information */
    std::vector<lol::thread *> m_workers;
    lol::queue<int> m_questions, m_answers;
};

