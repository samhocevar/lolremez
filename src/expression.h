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
// Powerful arithmetic expression parser/evaluator
//
// Usage:
//   expression e;
//   e.parse(" 2*x^3 + 3 * sin(x - atan(x))");
//   auto y = e.eval("1.5");
//

#include "tao/pegtl.hpp"

namespace grammar
{

using namespace tao::pegtl;

enum class id : uint8_t
{
    /* Variables and constants */
    x, y,
    constant,
    /* Unary functions/operators */
    plus, minus, abs,
    sqrt, cbrt,
    exp, exp2, erf, log, log2, log10,
    sin, cos, tan,
    asin, acos, atan,
    sinh, cosh, tanh,
    /* Binary functions/operators */
    add, sub, mul, div,
    atan2, pow,
    min, max,
};

struct expression
{
    /*
     * Evaluate expression at x
     */
    lol::real eval(lol::real const &x) const
    {
        /* Use a stack */
        lol::array<lol::real> stack;

        for (int i = 0; i < m_ops.count(); ++i)
        {
            /* Rules that do not consume stack elements */
            if (m_ops[i].m1 == id::x)
            {
                stack.push(x);
                continue;
            }
            else if (m_ops[i].m1 == id::y)
            {
                stack.push(0); // TODO
                continue;
            }
            else if (m_ops[i].m1 == id::constant)
            {
                stack.push(m_constants[m_ops[i].m2]);
                continue;
            }

            /* All other rules consume at least the head of the stack */
            lol::real head = stack.pop();

            switch (m_ops[i].m1)
            {
            case id::plus:  stack.push(head);  break;
            case id::minus: stack.push(-head); break;

            case id::abs:   stack.push(fabs(head));  break;
            case id::sqrt:  stack.push(sqrt(head));  break;
            case id::cbrt:  stack.push(cbrt(head));  break;
            case id::exp:   stack.push(exp(head));   break;
            case id::exp2:  stack.push(exp2(head));  break;
            case id::erf:   stack.push(erf(head));   break;
            case id::log:   stack.push(log(head));   break;
            case id::log2:  stack.push(log2(head));  break;
            case id::log10: stack.push(log10(head)); break;
            case id::sin:   stack.push(sin(head));   break;
            case id::cos:   stack.push(cos(head));   break;
            case id::tan:   stack.push(tan(head));   break;
            case id::asin:  stack.push(asin(head));  break;
            case id::acos:  stack.push(acos(head));  break;
            case id::atan:  stack.push(atan(head));  break;
            case id::sinh:  stack.push(sinh(head));  break;
            case id::cosh:  stack.push(cosh(head));  break;
            case id::tanh:  stack.push(tanh(head));  break;

            case id::add:   stack.push(stack.pop() + head); break;
            case id::sub:   stack.push(stack.pop() - head); break;
            case id::mul:   stack.push(stack.pop() * head); break;
            case id::div:   stack.push(stack.pop() / head); break;

            case id::atan2: stack.push(atan2(stack.pop(), head)); break;
            case id::pow:   stack.push(pow(stack.pop(), head));   break;
            case id::min:   stack.push(min(stack.pop(), head));   break;
            case id::max:   stack.push(max(stack.pop(), head));   break;

            case id::x:
            case id::y:
            case id::constant:
                /* Already handled above */
                break;
            }
        }

        ASSERT(stack.count() == 1);
        return stack.pop();
    }

    /*
     * Is expression constant? i.e. does not depend on x
     */
    bool is_constant() const
    {
        for (auto op : m_ops)
            if (op.m1 == id::x)
                return false;

        return true;
    }

private:
    lol::array<id, int> m_ops;
    lol::array<lol::real> m_constants;

private:
    struct r_expr;

    // r_ <- <blank> *
    struct _ : star<space> {};

    // r_float <- <digit> + ( "." <digit> * ) ? ( ( [eE] [+-] ? <digit> + ) ?
    struct r_float : seq<plus<digit>,
                         opt<seq<one<'.'>,
                                 star<digit>>>,
                         opt<seq<one<'e', 'E'>,
                                 opt<one<'+', '-'>>,
                                 plus<digit>>>> {};

    // r_hex_float <- "0" [xX] <xdigit> + ( "." <xdigit> * ) ? ( ( [pP] [+-] ? <digit> + ) ?
    struct r_hex_float : seq<one<'0'>,
                             one<'x', 'X'>,
                             plus<xdigit>,
                             opt<seq<one<'.'>,
                                     star<xdigit>>>,
                             opt<seq<one<'p', 'P'>,
                                     opt<one<'+', '-'>>,
                                     plus<digit>>>> {};

    // r_sup_digit <- "⁰" / "¹" / "²" / "³" / "⁴" / "⁵" / "⁶" / "⁷" / "⁸" / "⁹"
    struct r_sup_digit : sor<TAOCPP_PEGTL_STRING("⁰"),
                             TAOCPP_PEGTL_STRING("¹"),
                             TAOCPP_PEGTL_STRING("²"),
                             TAOCPP_PEGTL_STRING("³"),
                             TAOCPP_PEGTL_STRING("⁴"),
                             TAOCPP_PEGTL_STRING("⁵"),
                             TAOCPP_PEGTL_STRING("⁶"),
                             TAOCPP_PEGTL_STRING("⁷"),
                             TAOCPP_PEGTL_STRING("⁸"),
                             TAOCPP_PEGTL_STRING("⁹")> {};

    // r_sup_float <- <r_sup_digit> +
    struct r_sup_float : plus<r_sup_digit> {};

    // r_name <- r_hex_float / r_float / "x" / "y" / "e" / "pi" / "π" / "tau" / "τ"
    struct r_name : sor<r_hex_float,
                        r_float,
                        TAOCPP_PEGTL_STRING("x"),
                        TAOCPP_PEGTL_STRING("y"),
                        TAOCPP_PEGTL_STRING("e"),
                        TAOCPP_PEGTL_STRING("pi"),
                        TAOCPP_PEGTL_STRING("π"),
                        TAOCPP_PEGTL_STRING("tau"),
                        TAOCPP_PEGTL_STRING("τ")> {};

    // r_binary_call <- <r_binary_fun> "(" r_expr "," r_expr ")"
    struct r_binary_fun : sor<TAOCPP_PEGTL_STRING("atan2"),
                              TAOCPP_PEGTL_STRING("pow"),
                              TAOCPP_PEGTL_STRING("min"),
                              TAOCPP_PEGTL_STRING("max")> {};

    struct r_binary_call : seq<r_binary_fun,
                               _, one<'('>,
                               _, r_expr,
                               _, one<','>,
                               _, r_expr,
                               _, one<')'>> {};

    // r_unary_call <- <r_unary_fun> "(" r_expr ")"
    struct r_unary_fun : sor<TAOCPP_PEGTL_STRING("abs"),
                             TAOCPP_PEGTL_STRING("sqrt"),
                             TAOCPP_PEGTL_STRING("cbrt"),
                             TAOCPP_PEGTL_STRING("exp"),
                             TAOCPP_PEGTL_STRING("exp2"),
                             TAOCPP_PEGTL_STRING("erf"),
                             TAOCPP_PEGTL_STRING("log"),
                             TAOCPP_PEGTL_STRING("log2"),
                             TAOCPP_PEGTL_STRING("log10"),
                             TAOCPP_PEGTL_STRING("sin"),
                             TAOCPP_PEGTL_STRING("cos"),
                             TAOCPP_PEGTL_STRING("tan"),
                             TAOCPP_PEGTL_STRING("asin"),
                             TAOCPP_PEGTL_STRING("acos"),
                             TAOCPP_PEGTL_STRING("atan"),
                             TAOCPP_PEGTL_STRING("sinh"),
                             TAOCPP_PEGTL_STRING("cosh"),
                             TAOCPP_PEGTL_STRING("tanh")> {};

    struct r_unary_call : seq<r_unary_fun,
                              _, one<'('>,
                              _, r_expr,
                              _, one<')'>> {};

    // r_call <- r_binary_call / r_unary_call
    struct r_call : sor<r_binary_call,
                        r_unary_call> {};

    // r_parentheses <- "(" r_expr ")"
    struct r_parentheses : seq<one<'('>,
                               pad<r_expr, space>,
                               one<')'>> {};

    // r_terminal <- ( r_call / r_name / r_parentheses ) r_sup_float ?
    struct r_terminal : seq<sor<r_call,
                                r_name,
                                r_parentheses>,
                            _, opt<r_sup_float>> {};

    // r_signed <- "-" r_signed / "+" r_signed / r_terminal
    struct r_signed;
    struct r_negative : seq<one<'-'>, _, r_signed> {};
    struct r_signed : sor<r_negative,
                          seq<one<'+'>, _, r_signed>,
                          r_terminal> {};

    // r_pow <- ( "^" / "**" ) r_signed
    struct r_pow : seq<pad<sor<one<'^'>,
                               string<'*', '*'>>, space>,
                       r_signed> {};

    // r_factor <- r_terminal ( r_pow ) *
    struct r_factor : seq<r_terminal,
                          star<r_pow>> {};

    // r_signed2 <- "-" r_signed2 / "+" r_signed2 / r_factor
    struct r_signed2;
    struct r_negative2 : seq<one<'-'>, _, r_signed2> {};
    struct r_signed2 : sor<r_negative2,
                           seq<one<'+'>, _, r_signed2>,
                           r_factor> {};

    // r_mul <- "*" r_signed2
    // r_div <- "/" r_signed2
    // r_term <- r_signed2 ( r_mul / r_div ) *
    struct r_mul : seq<_, one<'*'>, _, r_signed2> {};
    struct r_div : seq<_, one<'/'>, _, r_signed2> {};
    struct r_term : seq<r_signed2,
                        star<sor<r_mul, r_div>>> {};

    // r_add <- "+" r_term
    // r_sub <- "-" r_term
    // r_expr <- r_term ( r_add / r_sub ) *
    struct r_add : seq<_, one<'+'>, _, r_term> {};
    struct r_sub : seq<_, one<'-'>, _, r_term> {};
    struct r_expr : seq<r_term,
                        star<sor<r_add, r_sub>>> {};

    // r_stmt <- r_expr <end>
    struct r_stmt : must<pad<r_expr, space>, tao::pegtl::eof> {};

    //
    // Default actions
    //

    template<typename R>
    struct action : nothing<R> {};

    template<id OP>
    struct generic_action
    {
        static void apply0(expression *that)
        {
            that->m_ops.push(OP, -1);
        }
    };

public:
    /*
     * Parse arithmetic expression in x, e.g. 2*x+3
     */
    bool parse(std::string const &str)
    {
        m_ops.empty();
        m_constants.empty();

        tao::pegtl::memory_input<> in(str, "expression");
        try
        {
            tao::pegtl::parse<r_stmt, action>(in, this);
            return true;
        }
        catch (const tao::pegtl::parse_error &ex)
        {
            printf("parse error: %s\n", ex.what());
            return false;
        }
    }
};

//
// Rule specialisations for simple operators
//

template<> struct expression::action<expression::r_pow> : generic_action<id::pow> {};
template<> struct expression::action<expression::r_mul> : generic_action<id::mul> {};
template<> struct expression::action<expression::r_div> : generic_action<id::div> {};
template<> struct expression::action<expression::r_add> : generic_action<id::add> {};
template<> struct expression::action<expression::r_sub> : generic_action<id::sub> {};
template<> struct expression::action<expression::r_negative> : generic_action<id::minus> {};
template<> struct expression::action<expression::r_negative2> : generic_action<id::minus> {};

//
// Rule specialisations for unary and binary function calls
//

template<>
struct expression::action<expression::r_binary_call>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        struct { id ret; char const *name; } lut[] =
        {
            { id::atan2, "atan2" },
            { id::pow,   "pow" },
            { id::min,   "min" },
            { id::max,   "max" },
        };

        for (auto pair : lut)
        {
            if (strncmp(in.string().c_str(), pair.name, strlen(pair.name)) != 0)
                continue;

            that->m_ops.push(pair.ret, -1);
            return;
        }
    }
};

template<>
struct expression::action<expression::r_unary_call>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        struct { id ret; char const *name; } lut[] =
        {
            { id::abs,   "abs" },
            { id::sqrt,  "sqrt" },
            { id::cbrt,  "cbrt" },
            { id::exp,   "exp" },
            { id::exp2,  "exp2" },
            { id::erf,   "erf" },
            { id::log10, "log10" },
            { id::log2,  "log2" },
            { id::log,   "log" },
            { id::sinh,  "sinh" },
            { id::cosh,  "cosh" },
            { id::tanh,  "tanh" },
            { id::sin,   "sin" },
            { id::cos,   "cos" },
            { id::tan,   "tan" },
            { id::asin,  "asin" },
            { id::acos,  "acos" },
            { id::atan,  "atan" },
        };

        for (auto pair : lut)
        {
            if (strncmp(in.string().c_str(), pair.name, strlen(pair.name)) != 0)
                continue;

            that->m_ops.push(pair.ret, -1);
            return;
        }
    }
};

template<>
struct expression::action<expression::r_sup_float>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        lol::real val = lol::real::R_0();

        for (char const *p = in.string().c_str(); *p; )
        {
            val *= lol::real::R_10();

            static char const *lut[] =
            {
                "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹",
            };

            for (int i = 0; i < 10; ++i)
            {
                if (memcmp(p, lut[i], strlen(lut[i])) == 0)
                {
                    val += lol::real(i);
                    p += strlen(lut[i]);
                    break;
                }
            }
        }

        that->m_ops.push(id::constant, that->m_constants.count());
        that->m_constants.push(val);
        that->m_ops.push(id::pow, -1);
    }
};

template<>
struct expression::action<expression::r_name>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        if (in.string() == "x")
            that->m_ops.push(id::x, -1);
        else if (in.string() == "y")
            that->m_ops.push(id::y, -1);
        else
        {
            that->m_ops.push(id::constant, that->m_constants.count());
            if (in.string() == "e")
                that->m_constants.push(lol::real::R_E());
            else if (in.string() == "pi" || in.string() == "π")
                that->m_constants.push(lol::real::R_PI());
            else if (in.string() == "tau" || in.string() == "τ")
                that->m_constants.push(lol::real::R_TAU());
            else /* FIXME: check if the constant is already in the list */
                that->m_constants.push(lol::real(in.string().c_str()));
        }
    }
};

} /* namespace grammar */

using grammar::expression;

