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
// Powerful arithmetic expression parser/evaluator
//
// Usage:
//   expression e;
//   e.parse(" 2*x^3 + 3 * sin(x - atan(x))");
//   auto y = e.eval("1.5");
//

#include <lol/pegtl>
#include <vector>
#include <map>
#include <tuple>
#include <cassert>

namespace grammar
{

using namespace tao::pegtl;
using long_double = long double;

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
    add, sub, mul, div, mod,
    atan2, pow,
    min, max,
    fmod,
    /* Conversion functions */
    tofloat, todouble, toldouble,
};

struct expression
{
    /*
     * Evaluate expression at x
     */
    lol::real eval(lol::real const &x) const
    {
        /* Use a stack */
        std::vector<lol::real> stack;

        auto pop_val = [&stack]() -> lol::real
        {
            auto ret = stack.back();
            stack.pop_back();
            return ret;
        };

        auto push_val = [&stack](lol::real const &v) -> void
        {
            stack.push_back(v);
        };

        for (size_t i = 0; i < m_ops.size(); ++i)
        {
            /* Rules that do not consume stack elements */
            if (std::get<0>(m_ops[i]) == id::x)
            {
                push_val(x);
                continue;
            }
            else if (std::get<0>(m_ops[i]) == id::y)
            {
                push_val(0); // TODO
                continue;
            }
            else if (std::get<0>(m_ops[i]) == id::constant)
            {
                push_val(m_constants[std::get<1>(m_ops[i])]);
                continue;
            }

            /* All other rules consume at least the head of the stack */
            lol::real head = pop_val();

            switch (std::get<0>(m_ops[i]))
            {
            case id::plus:  push_val(head);  break;
            case id::minus: push_val(-head); break;

            case id::abs:   push_val(fabs(head));  break;
            case id::sqrt:  push_val(sqrt(head));  break;
            case id::cbrt:  push_val(cbrt(head));  break;
            case id::exp:   push_val(exp(head));   break;
            case id::exp2:  push_val(exp2(head));  break;
            case id::erf:   push_val(erf(head));   break;
            case id::log:   push_val(log(head));   break;
            case id::log2:  push_val(log2(head));  break;
            case id::log10: push_val(log10(head)); break;
            case id::sin:   push_val(sin(head));   break;
            case id::cos:   push_val(cos(head));   break;
            case id::tan:   push_val(tan(head));   break;
            case id::asin:  push_val(asin(head));  break;
            case id::acos:  push_val(acos(head));  break;
            case id::atan:  push_val(atan(head));  break;
            case id::sinh:  push_val(sinh(head));  break;
            case id::cosh:  push_val(cosh(head));  break;
            case id::tanh:  push_val(tanh(head));  break;

            case id::add:   push_val(pop_val() + head); break;
            case id::sub:   push_val(pop_val() - head); break;
            case id::mul:   push_val(pop_val() * head); break;
            case id::div:   push_val(pop_val() / head); break;

            case id::atan2: push_val(atan2(pop_val(), head)); break;
            case id::pow:   push_val(pow(pop_val(), head));   break;
            case id::min:   push_val(min(pop_val(), head));   break;
            case id::max:   push_val(max(pop_val(), head));   break;
            case id::mod:
            case id::fmod:  push_val(fmod(pop_val(), head)); break;

            case id::tofloat:   push_val(lol::real(float(head))); break;
            case id::todouble:  push_val(lol::real(double(head))); break;
            case id::toldouble: push_val(lol::real(long_double(head))); break;

            case id::x:
            case id::y:
            case id::constant:
                /* Already handled above */
                break;
            }
        }

        assert(stack.size() == 1);
        return pop_val();
    }

    /*
     * Is expression constant? i.e. does not depend on x
     */
    bool is_constant() const
    {
        for (auto const &op : m_ops)
            if (std::get<0>(op) == id::x)
                return false;

        return true;
    }

private:
    std::vector<id> m_temp_op;
    std::vector<std::tuple<id, int>> m_ops;
    std::vector<lol::real> m_constants;

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
    struct r_sup_digit : sor<TAO_PEGTL_STRING("⁰"),
                             TAO_PEGTL_STRING("¹"),
                             TAO_PEGTL_STRING("²"),
                             TAO_PEGTL_STRING("³"),
                             TAO_PEGTL_STRING("⁴"),
                             TAO_PEGTL_STRING("⁵"),
                             TAO_PEGTL_STRING("⁶"),
                             TAO_PEGTL_STRING("⁷"),
                             TAO_PEGTL_STRING("⁸"),
                             TAO_PEGTL_STRING("⁹")> {};

    // r_sup_float <- <r_sup_digit> +
    struct r_sup_float : plus<r_sup_digit> {};

    // r_name <- r_hex_float / r_float / "x" / "y" / "e" / "pi" / "π" / "tau" / "τ"
    struct r_name : sor<r_hex_float,
                        r_float,
                        TAO_PEGTL_STRING("x"),
                        TAO_PEGTL_STRING("y"),
                        TAO_PEGTL_STRING("e"),
                        TAO_PEGTL_STRING("pi"),
                        TAO_PEGTL_STRING("π"),
                        TAO_PEGTL_STRING("tau"),
                        TAO_PEGTL_STRING("τ")> {};

    // r_binary_call <- <r_binary_fun> "(" r_expr "," r_expr ")"
    struct r_binary_fun : sor<TAO_PEGTL_STRING("atan2"),
                              TAO_PEGTL_STRING("pow"),
                              TAO_PEGTL_STRING("min"),
                              TAO_PEGTL_STRING("max"),
                              TAO_PEGTL_STRING("fmod")> {};

    struct r_binary_call : seq<r_binary_fun,
                               _, one<'('>,
                               _, r_expr,
                               _, one<','>,
                               _, r_expr,
                               _, one<')'>> {};

    // r_unary_call <- <r_unary_fun> "(" r_expr ")"
    // XXX: “log2” must come before “log” etc. because the parser is greedy.
    struct r_unary_fun : sor<TAO_PEGTL_STRING("tanh"),
                             TAO_PEGTL_STRING("tan"),
                             TAO_PEGTL_STRING("sqrt"),
                             TAO_PEGTL_STRING("sinh"),
                             TAO_PEGTL_STRING("sin"),
                             TAO_PEGTL_STRING("log2"),
                             TAO_PEGTL_STRING("log10"),
                             TAO_PEGTL_STRING("log"),
                             TAO_PEGTL_STRING("ldouble"),
                             TAO_PEGTL_STRING("float"),
                             TAO_PEGTL_STRING("exp2"),
                             TAO_PEGTL_STRING("exp"),
                             TAO_PEGTL_STRING("erf"),
                             TAO_PEGTL_STRING("double"),
                             TAO_PEGTL_STRING("cbrt"),
                             TAO_PEGTL_STRING("cosh"),
                             TAO_PEGTL_STRING("cos"),
                             TAO_PEGTL_STRING("asin"),
                             TAO_PEGTL_STRING("atan"),
                             TAO_PEGTL_STRING("acos"),
                             TAO_PEGTL_STRING("abs")> {};

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
    // r_mod <- "%" r_signed2
    // r_term <- r_signed2 ( r_mul / r_div / r_mod ) *
    struct r_mul : seq<_, one<'*'>, _, r_signed2> {};
    struct r_div : seq<_, one<'/'>, _, r_signed2> {};
    struct r_mod : seq<_, one<'%'>, _, r_signed2> {};
    struct r_term : seq<r_signed2,
                        star<sor<r_mul, r_div, r_mod>>> {};

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
            that->m_ops.push_back(std::make_tuple(OP, -1));
        }
    };

public:
    /*
     * Parse arithmetic expression in x, e.g. 2*x+3
     */
    bool parse(std::string const &str)
    {
        m_ops.clear();
        m_constants.clear();

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
template<> struct expression::action<expression::r_mod> : generic_action<id::mod> {};
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
            { id::fmod,  "fmod" },
        };

        for (auto pair : lut)
        {
            if (strncmp(in.string().c_str(), pair.name, strlen(pair.name)) != 0)
                continue;

            that->m_ops.push_back(std::make_tuple(pair.ret, -1));
            return;
        }
    }
};

template<>
struct expression::action<expression::r_unary_fun>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        static std::map<std::string, id> lut =
        {
            { "abs",   id::abs },
            { "sqrt",  id::sqrt },
            { "cbrt",  id::cbrt },
            { "exp",   id::exp },
            { "exp2",  id::exp2 },
            { "erf",   id::erf },
            { "log10", id::log10 },
            { "log2",  id::log2 },
            { "log",   id::log },
            { "sinh",  id::sinh },
            { "cosh",  id::cosh },
            { "tanh",  id::tanh },
            { "sin",   id::sin },
            { "cos",   id::cos },
            { "tan",   id::tan },
            { "asin",  id::asin },
            { "acos",  id::acos },
            { "atan",  id::atan },
            { "float",   id::tofloat },
            { "double",  id::todouble },
            { "ldouble", id::toldouble },
        };

        that->m_temp_op.push_back(lut[in.string()]);
    }
};

template<>
struct expression::action<expression::r_unary_call>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        that->m_ops.push_back(std::make_tuple(that->m_temp_op.back(), -1));
        that->m_temp_op.pop_back();
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

        that->m_ops.push_back(std::make_tuple(id::constant, (int)that->m_constants.size()));
        that->m_constants.push_back(val);
        that->m_ops.push_back(std::make_tuple(id::pow, -1));
    }
};

template<>
struct expression::action<expression::r_name>
{
    template<typename INPUT>
    static void apply(INPUT const &in, expression *that)
    {
        if (in.string() == "x")
            that->m_ops.push_back(std::make_tuple(id::x, -1));
        else if (in.string() == "y")
            that->m_ops.push_back(std::make_tuple(id::y, -1));
        else
        {
            that->m_ops.push_back(std::make_tuple(id::constant, (int)that->m_constants.size()));
            if (in.string() == "e")
                that->m_constants.push_back(lol::real::R_E());
            else if (in.string() == "pi" || in.string() == "π")
                that->m_constants.push_back(lol::real::R_PI());
            else if (in.string() == "tau" || in.string() == "τ")
                that->m_constants.push_back(lol::real::R_TAU());
            else /* FIXME: check if the constant is already in the list */
                that->m_constants.push_back(lol::real(in.string().c_str()));
        }
    }
};

} /* namespace grammar */

using grammar::expression;

