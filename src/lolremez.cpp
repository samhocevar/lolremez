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

#include <float.h>
#include <iostream>
#include <iomanip>
#include <optional> // std::optional

#include <lol/utils>
#include <lol/cli>
#include <lol/real>

#include "solver.h"
#include "expression.h"

using lol::real;

static std::string copyright =
    "Copyright © 2005—2020 Sam Hocevar <sam@hocevar.net>\n"
    "This program is free software. It comes without any warranty, to the extent\n"
    "permitted by applicable law. You can redistribute it and/or modify it under\n"
    "the terms of the Do What the Fuck You Want to Public License, Version 2, as\n"
    "published by the WTFPL Task Force. See http://www.wtfpl.net/ for more details.\n";

static std::string footer =
    "\n"
    "Examples:\n"
    "  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\"\n"
    "  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\" \"exp(1+x)\"\n"
    "\n"
    "Tutorial available on https://github.com/samhocevar/lolremez/wiki\n";

static std::string bugs =
    "\n"
    "Written by Sam Hocevar. Report bugs to <sam@hocevar.net> or to the\n"
    "issue page: https://github.com/samhocevar/lolremez/issues\n";

// FIXME: improve --version output by maybe reusing this function
static void version()
{
    std::cout
        << "lolremez " << PACKAGE_VERSION << "\n"
        << "\n"
        << copyright
        << bugs;
}

// FIXME: improve --help output by mayne adding some messages
static void usage()
{
    std::cout
        << "Usage: lolremez [-d degree] [-r xmin:xmax] x-expression [x-error]\n"
        << "       lolremez -h | --help\n"
        << "       lolremez -V | --version\n" 
        << "Find a polynomial approximation for x-expression.\n"
        << "\n"
        << "Mandatory arguments to long options are mandatory for short options too.\n"
        << "  -d, --degree <degree>      degree of final polynomial\n"
        << "  -r, --range <xmin>:<xmax>  range over which to approximate\n"
        << "  -p, --precision <bits>     floating-point precision (default 512)\n"
        << "      --progress             print progress\n"
        << "      --stats                print timing statistics\n"
        << "  -h, --help                 display this help and exit\n"
        << "  -V, --version              output version information and exit\n"
        << footer
        << bugs;
}

static void FAIL(char const *message = nullptr, ...)
{
    if (message)
    {
        printf("Error: ");
        va_list ap;
        va_start(ap, message);
        vfprintf(stdout, message, ap);
        va_end(ap);
        printf("\n");
    }
    printf("Try 'lolremez --help' for more information.\n");
    exit(EXIT_FAILURE);
}

// See the tutorial at http://lolengine.net/wiki/doc/maths/remez
int main(int argc, char **argv)
{
    std::string str_xmin("-1"), str_xmax("1");
    enum
    {
        mode_float,
        mode_double,
        mode_long_double,

        mode_default = mode_double,
    }
    mode = mode_default;
    root_finder rf = root_finder::pegasus;

    bool show_stats = false;
    bool show_progress = false;
    bool show_debug = false;

    std::string expr;
    std::optional<std::string> error, range;
    std::optional<int> degree;
    std::optional<int> bits;

    remez_solver solver;

    lol::cli::app opts("lolremez");
    opts.set_version_flag("-V,--version", PACKAGE_VERSION);
    opts.footer(footer + bugs);

    // Approximation parameters
    opts.add_option("-d,--degree", degree, "degree of final polynomial")->type_name("<int>");
    opts.add_option("-r,--range", range, "range over which to approximate")->type_name("<xmin>:<xmax>");
    // Precision parameters
    opts.add_option("-p,--precision", bits, "floating-point precision (default 512)")->type_name("<int>");
    opts.add_flag("--float", [&](int64_t) { mode = mode_float; }, "use float type");
    opts.add_flag("--double", [&](int64_t) { mode = mode_double; }, "use double type");
    opts.add_flag("--long-double", [&](int64_t) { mode = mode_long_double; }, "use long double type");
    opts.add_flag("--bisect", [&](int64_t) { rf = root_finder::bisect; }, "use bisection for root finding");
    opts.add_flag("--regula-falsi", [&](int64_t) { rf = root_finder::regula_falsi; }, "use regula falsi for root finding");
    opts.add_flag("--illinois", [&](int64_t) { rf = root_finder::illinois; }, "use Illinois algorithm for root finding");
    opts.add_flag("--pegasus", [&](int64_t) { rf = root_finder::pegasus; }, "use Pegasus algorithm for root finding (default)");
    opts.add_flag("--ford", [&](int64_t) { rf = root_finder::ford; }, "use Ford algorithm for root finding");
    // Runtime flags
    opts.add_flag("--progress", show_progress, "print progress");
    opts.add_flag("--stats", show_stats, "print timing statistics");
    opts.add_flag("--debug", show_debug, "print debug messages");
    // Expression to evaluate and optional error expression
    opts.add_option("expression", expr)->type_name("<x-expression>")->required();
    opts.add_option("error", error)->type_name("<x-expression>");

    CLI11_PARSE(opts, argc, argv);

    if (degree)
    {
        if (*degree < 1)
            FAIL("invalid degree: must be at least 1");
        solver.set_order(*degree);
    }

    if (range)
    {
        auto arg = lol::split(*range, ':');
        if (arg.size() != 2)
            FAIL("invalid range");
        str_xmin = arg[0];
        str_xmax = arg[1];
    }

    if (bits)
    {
        if (*bits < 32 || *bits > 65535)
            FAIL("invalid precision %d", *bits);
        real::global_bigit_count((*bits + 31) / 32);
    }

    // Initialise solver: ranges
    lol::real xmin, xmax;
    expression ex;

    if (!ex.parse(str_xmin))
        FAIL("invalid range xmin syntax: %s", str_xmin.c_str());
    if (!ex.is_constant())
        FAIL("invalid range: xmin must be constant");
    xmin = ex.eval(real::R_0());

    if (!ex.parse(str_xmax))
        FAIL("invalid range xmax syntax: %s", str_xmax.c_str());
    if (!ex.is_constant())
        FAIL("invalid range: xmax must be constant");
    xmax = ex.eval(real::R_0());

    if (xmin >= xmax)
        FAIL("invalid range: xmin >= xmax");
    solver.set_range(xmin, xmax);

    if (!ex.parse(expr))
        FAIL("invalid function: %s", expr.c_str());

    // Special case: if the function is constant, evaluate it immediately
    if (ex.is_constant())
    {
        std::cout << std::setprecision(int(real::DEFAULT_BIGIT_COUNT * 16 / 3.321928094) + 2);
        std::cout << ex.eval(real::R_0()) << '\n';
        return EXIT_SUCCESS;
    }

    solver.set_func(ex);

    if (error)
    {
        if (!ex.parse(*error))
            FAIL("invalid weight function: %s", error->c_str());

        solver.set_weight(ex);
    }

    // https://en.wikipedia.org/wiki/Floating-point_arithmetic#Internal_representation
    int digits = mode == mode_float ? FLT_DIG + 2 :
                 mode == mode_double ? DBL_DIG + 2 : LDBL_DIG + 2;
    solver.set_digits(digits);
    solver.set_root_finder(rf);

    solver.show_stats = show_stats;
    solver.show_debug = show_debug;

    // Solve polynomial
    solver.do_init();
    for (int iteration = 0; ; ++iteration)
    {
        fprintf(stderr, "Iteration: %d\r", iteration);
        fflush(stderr); // Required on Windows because stderr is buffered.
        if (!solver.do_step())
            break;

        if (show_progress)
        {
            auto p = solver.get_estimate();
            for (int j = 0; j < p.degree() + 1; j++)
            {
                if (j > 0 && p[j] >= real::R_0())
                    std::cout << '+';
                std::cout << std::setprecision(digits) << p[j];
                if (j > 1)
                    std::cout << "*x**" << j;
                else if (j == 1)
                    std::cout << "*x";
            }
            std::cout << '\n';
            fflush(stdout);
        }
    }

    // Print final estimate
    auto p = solver.get_estimate();
    char const *type = mode == mode_float ? "float" :
                       mode == mode_double ? "double" : "long double";
    std::cout << "// Approximation of f(x) = " << expr << '\n';
    if (error)
        std::cout << "// with weight function g(x) = " << *error << '\n';
    std::cout << "// on interval [ " << str_xmin << ", " << str_xmax << " ]\n";
    std::cout << "// with a polynomial of degree " << p.degree() << ".\n";

    // Print expression in Horner form
    std::cout << std::setprecision(digits);
    std::cout << "// p(x)=";
    for (int j = 0; j < p.degree() - 1; ++j)
        std::cout << '(';
    std::cout << p[p.degree()];
    for (int j = p.degree() - 1; j >= 0; --j)
        std::cout << (j < p.degree() - 1 ? ")" : "") << "*x"
                  << (p[j] > real::R_0() ? "+" : "") << p[j];
    std::cout << '\n';

    // Print C/C++ function
    std::cout << std::setprecision(digits);
    std::cout << type << " f(" << type << " x)\n{\n";
    for (int j = p.degree(); j >= 0; --j)
    {
        char const *a = j ? "u = u * x +" : "return u * x +";
        if (j == p.degree())
            std::cout << "    " << type << " u = ";
        else
            std::cout << "    " << a << " ";
        std::cout << p[j];
        switch (mode)
        {
            case mode_float: std::cout << "f;\n"; break;
            case mode_double: std::cout << ";\n"; break;
            case mode_long_double: std::cout << "l;\n"; break;
        }
    }
    std::cout << "}\n";

    return 0;
}

