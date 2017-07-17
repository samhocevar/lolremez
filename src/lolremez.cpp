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

#include <lol/engine.h>

#include <lol/math/real.h>

#include "solver.h"
#include "expression.h"

using lol::array;
using lol::real;
using lol::String;

static void print_poly(lol::polynomial<lol::real> const &p);

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
    printf("Written by Sam Hocevar. Report bugs to <sam@hocevar.net>.\n");
}

static void FAIL(char const *message = nullptr)
{
    if (message)
        printf("Error: %s\n", message);
    printf("Try 'lolremez --help' for more information.\n");
    exit(EXIT_FAILURE);
}

/* See the tutorial at http://lolengine.net/wiki/doc/maths/remez */
int main(int argc, char **argv)
{
    lol::String str_xmin("-1"), str_xmax("1");
    int decimals = 40;
    bool has_weight = false;
    bool show_stats = false;
    bool show_progress = false;

    remez_solver solver;

    lol::getopt opt(argc, argv);
    opt.add_opt('h', "help",     false);
    opt.add_opt('v', "version",  false);
    opt.add_opt('d', "degree",   true);
    opt.add_opt('r', "range",    true);
    opt.add_opt(200, "stats",    false);
    opt.add_opt(201, "progress", false);

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
        case 200: /* --stats */
            show_stats = true;
            break;
        case 201: /* --progress */
            show_progress = true;
            break;
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
    ex.parse(str_xmin.C());
    if (!ex.is_constant())
        FAIL("invalid range: xmin must be constant");
    xmin = ex.eval(real::R_0());
    ex.parse(str_xmax.C());
    if (!ex.is_constant())
        FAIL("invalid range: xmax must be constant");
    xmax = ex.eval(real::R_0());
    ex.parse(str_xmax.C());
    if (xmin >= xmax)
        FAIL("invalid range: xmin >= xmax");
    solver.set_range(xmin, xmax);

    /* Initialise solver: functions */
    if (opt.index >= argc)
        FAIL("too few arguments: no function specified");

    if (opt.index + 2 < argc)
        FAIL("too many arguments");

    has_weight = (opt.index + 1 < argc);

    solver.set_func(argv[opt.index]);
    if (has_weight)
        solver.set_weight(argv[opt.index + 1]);

    solver.set_decimals(decimals);

    solver.show_stats = show_stats;

    /* Solve polynomial */
    solver.do_init();
    for (int iteration = 0; ; ++iteration)
    {
        fprintf(stderr, "Iteration: %d\r", iteration);
        if (!solver.do_step())
            break;

        if (show_progress)
        {
            print_poly(solver.get_estimate());
            fflush(stdout);
        }
    }

    /* Print final estimate as a C function */
    auto p = solver.get_estimate();
    printf("/* Approximation of f(x) = %s\n", argv[opt.index]);
    if (has_weight)
        printf(" * with weight function g(x) = %s\n", argv[opt.index + 1]);
    printf(" * on interval [ %s, %s ]\n", str_xmin.C(), str_xmax.C());
    printf(" * with a polynomial of degree %d. */\n", p.degree());
    printf("float f(float x)\n{\n");
    for (int j = p.degree(); j >= 0; --j)
    {
        char const *a = j ? "u = u * x +" : "return u * x +";
        printf("    %s ", j == p.degree() ? "float u =" : a);
        p[j].print(20);
        printf("f;\n");
    }
    printf("}\n");

    return 0;
}

static void print_poly(lol::polynomial<lol::real> const &p)
{
    for (int j = 0; j < p.degree() + 1; j++)
    {
        printf(j > 0 && p[j] >= real::R_0() ? "+" : "");
        p[j].print(20);
        printf(j == 0 ? "" : j > 1 ? "*x**%d" : "*x", j);
    }
    printf("\n");
}

