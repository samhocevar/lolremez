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

#include <lol/utils>
#include <lol/cli>
#include <lol/real>

#include "solver.h"
#include "expression.h"

using lol::real;

static void bugs()
{
    std::cout
        << "Written by Sam Hocevar. Report bugs to <sam@hocevar.net> or to the\n"
        << "issue page: https://github.com/samhocevar/lolremez/issues\n";
}

static void version()
{
    std::cout
        << "lolremez " << PACKAGE_VERSION << "\n"
        << "Copyright © 2005—2020 Sam Hocevar <sam@hocevar.net>\n"
        << "This program is free software. It comes without any warranty, to the extent\n"
        << "permitted by applicable law. You can redistribute it and/or modify it under\n"
        << "the terms of the Do What the Fuck You Want to Public License, Version 2, as\n"
        << "published by the WTFPL Task Force. See http://www.wtfpl.net/ for more details.\n"
        << "\n";
    bugs();
}

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
        << "\n"
        << "Examples:\n"
        << "  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\"\n"
        << "  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\" \"exp(1+x)\"\n"
        << "\n"
        << "Tutorial available on https://github.com/samhocevar/lolremez/wiki\n";
    bugs();
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

/* See the tutorial at http://lolengine.net/wiki/doc/maths/remez */
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

    bool has_degree = false, has_range = false, has_bits = false;
    bool show_help = false;
    bool show_stats = false;
    bool show_progress = false;
    bool show_debug = false;

    std::string expr, error;
    std::string range;
    int degree = -1;
    int bits = -1;

    remez_solver solver;

    auto help = lol::cli::required("-h", "--help").set(show_help, true)
              % "display this help and exit";

    auto version = lol::cli::required("-V", "--version").call([]()
                       { ::version(); exit(EXIT_SUCCESS); })
                 % "output version information and exit";

    auto run =
    (
        // Approximation parameters
        lol::cli::option("-d", "--degree")
            & lol::cli::integer("degree", degree).set(has_degree, true)
            % "degree of final polynomial",
        lol::cli::option("-r", "--range")
            & lol::cli::value("xmin>:<xmax", range).set(has_range, true)
            % "range over which to approximate",
        // Precision parameters
        lol::cli::option("-p", "--precision")
            & lol::cli::integer("bits", bits).set(has_bits, true)
            % "floating-point precision (default 512)",
        ( lol::cli::option("--float").set(mode, mode_float) |
          lol::cli::option("--double").set(mode, mode_double) |
          lol::cli::option("--long-double").set(mode, mode_long_double) ),
        // Runtime flags
        lol::cli::option("--progress").set(show_progress, true) % "print progress",
        lol::cli::option("--stats").set(show_stats, true) % "print timing statistics",
        lol::cli::option("--debug").set(show_debug, true),
        // Expression to evaluate and optional error expression
        lol::cli::value("x-expression").set(expr),
        lol::cli::opt_value("x-error").set(error)
    );

    auto success = lol::cli::parse(argc, argv, run | help | version);

    if (!success || show_help)
    {
        // clipp::documentation is not good enough yet; I would like the
        // following formatting:
        //
        //    -p, --precision <bits>     floating-point precision (default 512)
        //        --progress             print progress
        //
        // But I actually get:
        //
        //         <bits>      floating-point precision (default 512)
        //         --progress  print progress
        usage();
        return success ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (has_degree)
    {
        if (degree < 1)
            FAIL("invalid degree: must be at least 1");
        solver.set_order(degree);
    }

    if (has_range)
    {
        auto arg = lol::split(std::string(range), ':');
        if (arg.size() != 2)
            FAIL("invalid range");
        str_xmin = arg[0];
        str_xmax = arg[1];
    }

    if (has_bits)
    {
        if (bits < 32 || bits > 65535)
            FAIL("invalid precision %d", bits);
        real::global_bigit_count((bits + 31) / 32);
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

    if (error.length())
    {
        if (!ex.parse(error))
            FAIL("invalid weight function: %s", error.c_str());

        solver.set_weight(ex);
    }

    // https://en.wikipedia.org/wiki/Floating-point_arithmetic#Internal_representation
    int digits = mode == mode_float ? FLT_DIG + 2 :
                 mode == mode_double ? DBL_DIG + 2 : LDBL_DIG + 2;
    solver.set_digits(digits);

    solver.show_stats = show_stats;
    solver.show_debug = show_debug;

    // Solve polynomial
    solver.do_init();
    for (int iteration = 0; ; ++iteration)
    {
        fprintf(stderr, "Iteration: %d\r", iteration);
        fflush(stderr); /* Required on Windows because stderr is buffered. */
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
    if (error.length())
        std::cout << "// with weight function g(x) = " << error << '\n';
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

