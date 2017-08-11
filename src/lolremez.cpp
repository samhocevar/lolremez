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

#include <float.h>

#include <lol/engine.h>

#include <lol/math/real.h>

#include "solver.h"
#include "expression.h"

using lol::array;
using lol::real;
using lol::String;

static void version(void)
{
    printf("lolremez %s\n", PACKAGE_VERSION);
    printf("Copyright © 2005—2017 Sam Hocevar <sam@hocevar.net>\n");
    printf("This program is free software. It comes without any warranty, to the extent\n");
    printf("permitted by applicable law. You can redistribute it and/or modify it under\n");
    printf("the terms of the Do What the Fuck You Want to Public License, Version 2, as\n");
    printf("published by the WTFPL Task Force. See http://www.wtfpl.net/ for more details.\n");
    printf("\n");
    printf("Written by Sam Hocevar. Report bugs to <sam@hocevar.net>.\n");
}

static void usage()
{
    printf("Usage: lolremez [-d degree] [-r xmin:xmax] x-expression [x-error]\n");
    printf("       lolremez -h | --help\n");
    printf("       lolremez -V | --version\n"); 
    printf("Find a polynomial approximation for x-expression.\n");
    printf("\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("  -d, --degree <degree>      degree of final polynomial\n");
    printf("  -r, --range <xmin>:<xmax>  range over which to approximate\n");
    printf("      --progress             print progress\n");
    printf("      --stats                print timing statistics\n");
    printf("  -h, --help                 display this help and exit\n");
    printf("  -V, --version              output version information and exit\n");
    printf("\n");
    printf("Examples:\n");
    printf("  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\"\n");
    printf("  lolremez -d 4 -r -1:1 \"atan(exp(1+x))\" \"exp(1+x)\"\n");
    printf("\n");
    printf("Tutorial available on https://github.com/samhocevar/lolremez/wiki\n");
    printf("Written by Sam Hocevar. Report bugs to <sam@hocevar.net>.\n");
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
    lol::String str_xmin("-1"), str_xmax("1");
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

    remez_solver solver;

    lol::getopt opt(argc, argv);
    opt.add_opt('h', "help",     false);
    opt.add_opt('v', "version",  false);
    opt.add_opt('d', "degree",   true);
    opt.add_opt('r', "range",    true);
    opt.add_opt(200, "float",    false);
    opt.add_opt(201, "double",   false);
    opt.add_opt(202, "long-double", false);
    opt.add_opt(203, "stats",    false);
    opt.add_opt(204, "progress", false);
    opt.add_opt(205, "calc",     true);

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
            array<String> arg = String(opt.arg).split(':');
            if (arg.count() != 2)
                FAIL("invalid range");
            str_xmin = arg[0];
            str_xmax = arg[1];
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
        case 205: { /* --calc */
            expression ex;
            if (!ex.parse(opt.arg))
                FAIL("invalid range syntax: %s", opt.arg);
            if (!ex.is_constant())
                FAIL("invalid range: expression must be constant");
            ex.eval(real::R_0()).print(40);
            printf("\n");
            return EXIT_SUCCESS;
          } break;
        case 'h': /* --help */
            usage();
            return EXIT_SUCCESS;
        case 'v': /* --version */
            version();
            return EXIT_SUCCESS;
        default:
            FAIL();
        }
    }

    /* Initialise solver: ranges */
    lol::real xmin, xmax;
    expression ex;

    if (!ex.parse(str_xmin.C()))
        FAIL("invalid range xmin syntax: %s", str_xmin.C());
    if (!ex.is_constant())
        FAIL("invalid range: xmin must be constant");
    xmin = ex.eval(real::R_0());

    if (!ex.parse(str_xmax.C()))
        FAIL("invalid range xmax syntax: %s", str_xmax.C());
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

    if (!solver.set_func(argv[opt.index]))
        FAIL("invalid function: %s", argv[opt.index]);

    if (has_weight && !solver.set_weight(argv[opt.index + 1]))
        FAIL("invalid weight function: %s", argv[opt.index + 1]);

    /* https://en.wikipedia.org/wiki/Floating-point_arithmetic#Internal_representation */
    int digits = mode == mode_float ? FLT_DIG + 2 :
                 mode == mode_double ? DBL_DIG + 2 : LDBL_DIG + 2;
    solver.set_digits(digits);

    solver.show_stats = show_stats;

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
                printf(j > 0 && p[j] >= real::R_0() ? "+" : "");
                p[j].print(digits);
                printf(j == 0 ? "" : j > 1 ? "*x**%d" : "*x", j);
            }
            printf("\n");
            fflush(stdout);
        }
    }

    /* Print final estimate as a C function */
    auto p = solver.get_estimate();
    char const *type = mode == mode_float ? "float" :
                       mode == mode_double ? "double" : "long double";
    printf("/* Approximation of f(x) = %s\n", argv[opt.index]);
    if (has_weight)
        printf(" * with weight function g(x) = %s\n", argv[opt.index + 1]);
    printf(" * on interval [ %s, %s ]\n", str_xmin.C(), str_xmax.C());
    printf(" * with a polynomial of degree %d. */\n", p.degree());
    printf("%s f(%s x)\n{\n", type, type);
    for (int j = p.degree(); j >= 0; --j)
    {
        char const *a = j ? "u = u * x +" : "return u * x +";
        if (j == p.degree())
            printf("    %s u = ", type);
        else
            printf("    %s ", a);
        p[j].print(digits);
        switch (mode)
        {
            case mode_float: printf("f;\n"); break;
            case mode_double: printf(";\n"); break;
            case mode_long_double: printf("l;\n"); break;
        }
    }
    printf("}\n");

    return 0;
}

