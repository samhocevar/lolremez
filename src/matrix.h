//
//  LolRemez — Remez algorithm implementation
//
//  Copyright © 2005–2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

using namespace lol;

#include <cassert>

/*
 * Arbitrarily-sized square matrices; for now this only supports
 * naive inversion and is used for the Remez inversion method.
 */

template<typename T>
class array2d
{
public:
    array2d() = default;

    array2d(size_t cols, size_t rows)
    {
        resize(cols, rows);
    }

    void resize(size_t cols, size_t rows)
    {
        m_data.resize(cols * rows);
        m_cols = cols;
        m_rows = rows;
    }

    size_t cols() const { return m_cols; }
    size_t rows() const { return m_rows; }

    T *operator[](size_t line)
    {
        return &m_data[line * m_cols];
    }

    T const *operator[](size_t line) const
    {
        return &m_data[line * m_cols];
    }

private:
    std::vector<T> m_data;
    size_t m_cols = 0;
    size_t m_rows = 0;
};

template<typename T>
struct linear_system : public array2d<T>
{
    inline linear_system<T>(size_t n)
    {
        assert(n > 0);
        this->resize(n, n);
    }

    void init(T const &x)
    {
        auto n = this->cols();

        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++)
                (*this)[j][i] = (i == j) ? x : (T)0;
    }

    /* Naive matrix inversion */
    linear_system<T> inverse() const
    {
        auto n = this->cols();
        linear_system a(*this), b(n);

        b.init((T)1);

        /* Inversion method: iterate through all columns and make sure
         * all the terms are 1 on the diagonal and 0 everywhere else */
        for (int i = 0; i < n; i++)
        {
            /* If the expected coefficient is zero, add one of
             * the other lines. The first we meet will do. */
            if (!a[i][i])
            {
                for (int j = i + 1; j < n; j++)
                {
                    if (!a[j][i])
                        continue;
                    /* Add row j to row i */
                    for (int k = 0; k < n; k++)
                    {
                        a[i][k] += a[j][k];
                        b[i][k] += b[j][k];
                    }
                    break;
                }
            }

            /* Now we know the diagonal term is non-zero. Get its inverse
             * and use that to nullify all other terms in the column */
            T x = (T)1 / a[i][i];
            for (int j = 0; j < n; j++)
            {
                if (j == i)
                    continue;
                T mul = x * a[j][i];
                for (int k = 0; k < n; k++)
                {
                    a[j][k] -= mul * a[i][k];
                    b[j][k] -= mul * b[i][k];
                }
            }

            /* Finally, ensure the diagonal term is 1 */
            for (int k = 0; k < n; k++)
            {
                a[i][k] *= x;
                b[i][k] *= x;
            }
        }

        return b;
    }
};

