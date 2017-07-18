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

#include "pegtl.hh"

namespace grammar
{

using namespace pegtl;

enum class id : uint8_t
{
    /* Variables and constants */
    x,
    constant,
    /* Unary functions/operators */
    plus, minus, abs,
    sqrt, cbrt,
    exp, exp2, log, log2, log10,
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

    // r_constant <- r_hex_float / r_float / "e" / "pi"
    struct r_constant : sor<r_hex_float,
                            r_float,
                            pegtl_string_t("e"),
                            pegtl_string_t("pi")> {};

    // r_var <- "x"
    struct r_var : pegtl_string_t("x") {};

    // r_binary_call <- <r_binary_fun> "(" r_expr "," r_expr ")"
    struct r_binary_fun : sor<pegtl_string_t("atan2"),
                              pegtl_string_t("pow"),
                              pegtl_string_t("min"),
                              pegtl_string_t("max")> {};

    struct r_binary_call : seq<r_binary_fun,
                               _, one<'('>,
                               _, r_expr,
                               _, one<','>,
                               _, r_expr,
                               _, one<')'>> {};

    // r_unary_call <- <r_unary_fun> "(" r_expr ")"
    struct r_unary_fun : sor<pegtl_string_t("abs"),
                             pegtl_string_t("sqrt"),
                             pegtl_string_t("cbrt"),
                             pegtl_string_t("exp"),
                             pegtl_string_t("exp2"),
                             pegtl_string_t("log"),
                             pegtl_string_t("log2"),
                             pegtl_string_t("log10"),
                             pegtl_string_t("sin"),
                             pegtl_string_t("cos"),
                             pegtl_string_t("tan"),
                             pegtl_string_t("asin"),
                             pegtl_string_t("acos"),
                             pegtl_string_t("atan"),
                             pegtl_string_t("sinh"),
                             pegtl_string_t("cosh"),
                             pegtl_string_t("tanh")> {};

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

    // r_terminal <- r_call / r_var / r_constant / r_parentheses
    struct r_terminal : sor<r_call,
                            r_var,
                            r_constant,
                            r_parentheses> {};

    // r_signed <- "-" r_signed / "+" r_signed / r_terminal
    struct r_signed;
    struct r_minus : seq<one<'-'>, _, r_signed> {};
    struct r_signed : sor<r_minus,
                          seq<one<'+'>, _, r_signed>,
                          r_terminal> {};

    // r_exponent <- ( "^" / "**" ) r_signed
    struct r_exponent : seq<pad<sor<one<'^'>,
                                    string<'*', '*'>>, space>,
                            r_signed> {};

    // r_factor <- r_signed ( r_exponent ) *
    struct r_factor : seq<r_signed,
                          star<r_exponent>> {};

    // r_mul <- "*" r_factor
    // r_div <- "/" r_factor
    // r_term <- r_factor ( r_mul / r_div ) *
    struct r_mul : seq<_, one<'*'>, _, r_factor> {};
    struct r_div : seq<_, one<'/'>, _, r_factor> {};
    struct r_term : seq<r_factor,
                        star<sor<r_mul, r_div>>> {};

    // r_add <- "+" r_term
    // r_sub <- "-" r_term
    // r_expr <- r_term ( r_add / r_sub ) *
    struct r_add : seq<_, one<'+'>, _, r_term> {};
    struct r_sub : seq<_, one<'-'>, _, r_term> {};
    struct r_expr : seq<r_term,
                        star<sor<r_add, r_sub>>> {};

    // r_stmt <- r_expr <end>
    struct r_stmt : must<pad<r_expr, space>, pegtl::eof> {};

    //
    // Default actions
    //

    template<typename R>
    struct action : nothing<R> {};

    template<id OP>
    struct generic_action
    {
        static void apply(action_input const &in, expression *that)
        {
            UNUSED(in);
            that->m_ops.push(OP, -1);
        }
    };

public:
    /*
     * Parse arithmetic expression in x, e.g. 2*x+3
     */
    void parse(std::string const &str)
    {
        m_ops.empty();
        m_constants.empty();

        pegtl::parse_string<r_stmt, action>(str, "expression", this);
    }
};

//
// Rule specialisations for simple operators
//

template<> struct expression::action<expression::r_var> : generic_action<id::x> {};
template<> struct expression::action<expression::r_exponent> : generic_action<id::pow> {};
template<> struct expression::action<expression::r_mul> : generic_action<id::mul> {};
template<> struct expression::action<expression::r_div> : generic_action<id::div> {};
template<> struct expression::action<expression::r_add> : generic_action<id::add> {};
template<> struct expression::action<expression::r_sub> : generic_action<id::sub> {};
template<> struct expression::action<expression::r_minus> : generic_action<id::minus> {};

//
// Rule specialisations for unary and binary function calls
//

template<>
struct expression::action<expression::r_binary_call>
{
    static void apply(action_input const &in, expression *that)
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
    static void apply(action_input const &in, expression *that)
    {
        struct { id ret; char const *name; } lut[] =
        {
            { id::abs,   "abs" },
            { id::sqrt,  "sqrt" },
            { id::cbrt,  "cbrt" },
            { id::exp2,  "exp2" },
            { id::exp,   "exp" },
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
struct expression::action<expression::r_constant>
{
    static void apply(action_input const &in, expression *that)
    {
        that->m_ops.push(id::constant, that->m_constants.count());
        if (in.string() == "e")
            that->m_constants.push(lol::real::R_E());
        else if (in.string() == "pi")
            that->m_constants.push(lol::real::R_PI());
        else /* FIXME: check if the constant is already in the list */
            that->m_constants.push(lol::real(in.string().c_str()));
    }
};

} /* namespace grammar */

using grammar::expression;

