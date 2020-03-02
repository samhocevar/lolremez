//
//  LolRemez — Remez algorithm implementation
//
//  Copyright © 2005—2019 Sam Hocevar <sam@hocevar.net>
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

#include <lol/base/utils.h>
#include <lol/base/getopt.h>
#include <lol/types/real.h>

#include "solver.h"
#include "expression.h"

using lol::real;

static void version(void)
{
    std::cout
        << "lolremez " << PACKAGE_VERSION << "\n"
        << "Copyright © 2005—2017 Sam Hocevar <sam@hocevar.net>\n"
        << "This program is free software. It comes without any warranty, to the extent\n"
        << "permitted by applicable law. You can redistribute it and/or modify it under\n"
        << "the terms of the Do What the Fuck You Want to Public License, Version 2, as\n"
        << "published by the WTFPL Task Force. See http://www.wtfpl.net/ for more details.\n"
        << "\n"
        << "Written by Sam Hocevar. Report bugs to <sam@hocevar.net>.\n";
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
        << "Tutorial available on https://github.com/samhocevar/lolremez/wiki\n"
        << "Written by Sam Hocevar. Report bugs to <sam@hocevar.net>.\n";
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

    bool has_weight = false;
    bool show_stats = false;
    bool show_progress = false;
    bool show_debug = false;

    remez_solver solver;

    lol::getopt opt(argc, argv);
    opt.add_opt('h', "help",      false);
    opt.add_opt('V', "version",   false);
    opt.add_opt('d', "degree",    true);
    opt.add_opt('r', "range",     true);
    opt.add_opt('p', "precision", true);
    opt.add_opt(200, "float",     false);
    opt.add_opt(201, "double",    false);
    opt.add_opt(202, "long-double", false);
    opt.add_opt(203, "stats",     false);
    opt.add_opt(204, "progress",  false);
    opt.add_opt(205, "debug",     false);

    for (;;)
    {
        int c = opt.parse();
        if (c == -1)
            break;

        switch (c)
        {
        case 'd': { /* --degree */
            int degree = atoi(opt.arg);
            if (degree < 1)
                FAIL("invalid degree: must be at least 1");
            solver.set_order(degree);
          } break;
        case 'r': { /* --range */
            auto arg = lol::split(std::string(opt.arg), ':');
            if (arg.size() != 2)
                FAIL("invalid range");
            str_xmin = arg[0];
            str_xmax = arg[1];
          } break;
        case 'p': { /* --precision */
            int bits = atoi(opt.arg);
            if (bits < 32 || bits > 65535)
                FAIL("invalid precision %s", opt.arg);
            real::global_bigit_count((bits + 31) / 32);
          } break;
        case 200: /* --float */
            mode = mode_float;
            break;
        case 201: /* --double */
            mode = mode_double;
            break;
        case 202: /* --long-double */
            mode = mode_long_double;
            break;
        case 203: /* --stats */
            show_stats = true;
            break;
        case 204: /* --progress */
            show_progress = true;
            break;
        case 205: /* --debug */
            show_debug = true;
            break;
        case 'h': /* --help */
            usage();
            return EXIT_SUCCESS;
        case 'V': /* --version */
            version();
            return EXIT_SUCCESS;
        default:
            FAIL();
        }
    }

    /* Initialise solver: ranges */
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

    /* Initialise solver: functions */
    if (opt.index >= argc)
        FAIL("too few arguments: no function specified");

    if (opt.index + 2 < argc)
        FAIL("too many arguments");

    has_weight = (opt.index + 1 < argc);

    if (!ex.parse(argv[opt.index]))
        FAIL("invalid function: %s", argv[opt.index]);

    /* Special case: if the function is constant, evaluate it immediately */
    if (ex.is_constant())
    {
        std::cout << std::setprecision(int(real::DEFAULT_BIGIT_COUNT * 16 / 3.321928094) + 2);
        std::cout << ex.eval(real::R_0()) << '\n';
        return EXIT_SUCCESS;
    }

    solver.set_func(ex);

    if (has_weight)
    {
        if (!ex.parse(argv[opt.index + 1]))
            FAIL("invalid weight function: %s", argv[opt.index + 1]);

        solver.set_weight(ex);
    }

    /* https://en.wikipedia.org/wiki/Floating-point_arithmetic#Internal_representation */
    int digits = mode == mode_float ? FLT_DIG + 2 :
                 mode == mode_double ? DBL_DIG + 2 : LDBL_DIG + 2;
    solver.set_digits(digits);

    solver.show_stats = show_stats;
    solver.show_debug = show_debug;

    /* Solve polynomial */
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
    std::cout << "// Approximation of f(x) = " << argv[opt.index] << '\n';
    if (has_weight)
        std::cout << "// with weight function g(x) = " << argv[opt.index + 1] << '\n';
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

