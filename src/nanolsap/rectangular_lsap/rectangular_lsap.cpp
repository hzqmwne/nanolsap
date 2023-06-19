/*
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


This code implements the shortest augmenting path algorithm for the
rectangular assignment problem.  This implementation is based on the
pseudocode described in pages 1685-1686 of:

    DF Crouse. On implementing 2D rectangular assignment algorithms.
    IEEE Transactions on Aerospace and Electronic Systems
    52(4):1679-1696, August 2016
    doi: 10.1109/TAES.2016.140952

Author: PM Larsen
*/

#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>
#include "rectangular_lsap.h"

template <typename T> class matrix2d {
public:
    matrix2d(const T *d, intptr_t nr, intptr_t nc)
            : m_d(d), m_nr(nr), m_nc(nc),
            m_transpose(false), m_negative(false),
            m_subrows(nullptr), m_subcols(nullptr)  {
    }
    T get(intptr_t i, intptr_t j) const {    // TODO: always return double
        T r;
        if (this->m_transpose) {
            std::swap(i, j);
        }
        if (this->m_subrows != nullptr) {
            i = this->m_subrows[i];
        }
        if (this->m_subcols != nullptr) {
            j = this->m_subcols[j];
        }
        r = this->m_d[i * m_nc + j];
        if (this->m_negative) {
            r = -r;
        }
        return r;
    }
    void transpose() {
        this->m_transpose = !this->m_transpose;
    }
    void negative() {
        this->m_negative = !this->m_negative;
    }
    void subscript(const intptr_t *subrows, const intptr_t *subcols) {
        this->m_subrows = subrows;
        this->m_subcols = subcols;
    }
private:
    const T *m_d;
    intptr_t m_nr;
    intptr_t m_nc;
    bool m_transpose;
    bool m_negative;
    const intptr_t *m_subrows;
    const intptr_t *m_subcols;
};

template <typename T> std::vector<intptr_t> argsort_iter(const std::vector<T> &v)
{
    std::vector<intptr_t> index(v.size());
    std::iota(index.begin(), index.end(), 0);
    std::sort(index.begin(), index.end(), [&v](intptr_t i, intptr_t j)
              {return v[i] < v[j];});
    return index;
}

template <typename T> static intptr_t
augmenting_path(intptr_t nc, const matrix2d<T>& cost, const std::vector<double>& u,
                const std::vector<double>& v, std::vector<intptr_t>& path,
                const std::vector<intptr_t>& row4col,
                std::vector<double>& shortestPathCosts, intptr_t i,
                std::vector<bool>& SR, std::vector<bool>& SC,
                std::vector<intptr_t>& remaining, double* p_minVal)
{
    double minVal = 0;

    // Crouse's pseudocode uses set complements to keep track of remaining
    // nodes.  Here we use a vector, as it is more efficient in C++.
    intptr_t num_remaining = nc;
    for (intptr_t it = 0; it < nc; it++) {
        // Filling this up in reverse order ensures that the solution of a
        // constant cost matrix is the identity matrix (c.f. #11602).
        remaining[it] = nc - it - 1;
    }

    std::fill(SR.begin(), SR.end(), false);
    std::fill(SC.begin(), SC.end(), false);
    std::fill(shortestPathCosts.begin(), shortestPathCosts.end(), INFINITY);

    // find shortest augmenting path
    intptr_t sink = -1;
    while (sink == -1) {

        intptr_t index = -1;
        double lowest = INFINITY;
        SR[i] = true;

        for (intptr_t it = 0; it < num_remaining; it++) {
            intptr_t j = remaining[it];

            double r = minVal + cost.get(i, j) - u[i] - v[j];
            if (r < shortestPathCosts[j]) {
                path[j] = i;
                shortestPathCosts[j] = r;
            }

            // When multiple nodes have the minimum cost, we select one which
            // gives us a new sink node. This is particularly important for
            // integer cost matrices with small co-efficients.
            if (shortestPathCosts[j] < lowest ||
                (shortestPathCosts[j] == lowest && row4col[j] == -1)) {
                lowest = shortestPathCosts[j];
                index = it;
            }
        }

        minVal = lowest;
        if (minVal == INFINITY) { // infeasible cost matrix
            return -1;
        }

        intptr_t j = remaining[index];
        if (row4col[j] == -1) {
            sink = j;
        } else {
            i = row4col[j];
        }

        SC[j] = true;
        remaining[index] = remaining[--num_remaining];
    }

    *p_minVal = minVal;
    return sink;
}

template <typename T> static int
solve(intptr_t nr, intptr_t nc, const T* cost, bool maximize,
      const intptr_t *subrows, intptr_t n_subrows, const intptr_t *subcols, intptr_t n_subcols,
      int64_t* a, int64_t* b)
{
    // handle trivial inputs
    if (nr == 0 || nc == 0) {
        return 0;
    }

    // test for NaN and -inf entries
    for (intptr_t i = 0; i < nr * nc; i++) {
        if (cost[i] != cost[i] || ((cost[i] == -INFINITY) && !maximize) || ((cost[i] == INFINITY) && maximize)) {
            return RECTANGULAR_LSAP_INVALID;
        }
    }

    // check subrows and subcols in bound.
    if (n_subrows == 0) {
        subrows = nullptr;
    }
    else {
        // notice n_subrows larger than nr is legal, same for n_subcols and nc
        if (n_subrows < 0) {
            return RECTANGULAR_LSAP_SUBSCRIPT_INVALID;
        }
        for (intptr_t i = 0; i < n_subrows; i++) {
            intptr_t v = subrows[i];
            if (v < 0 || v >= nr) {
                return RECTANGULAR_LSAP_SUBSCRIPT_INVALID;
            }
        }
    }
    if (n_subcols == 0) {
        subcols = nullptr;
    }
    else {
        if (n_subcols < 0) {
            return RECTANGULAR_LSAP_SUBSCRIPT_INVALID;
        }
        for (intptr_t j = 0; j < n_subcols; j++) {
            intptr_t v = subcols[j];
            if (v < 0 || v >= nc) {
                return RECTANGULAR_LSAP_SUBSCRIPT_INVALID;
            }
        }
    }

    matrix2d costmat{cost, nr, nc};

    bool subscript = (subrows != nullptr) || (subcols != nullptr);
    if (subscript) {
        costmat.subscript(subrows, subcols);
        nr = n_subrows;
        nc = n_subcols;
    }

    // tall rectangular cost matrix must be transposed
    bool transpose = nc < nr;
    if (transpose) {
        costmat.transpose();
        std::swap(nr, nc);
    }
    if (maximize) {
        costmat.negative();
    }

    // initialize variables
    std::vector<double> u(nr, 0);
    std::vector<double> v(nc, 0);
    std::vector<double> shortestPathCosts(nc);
    std::vector<intptr_t> path(nc, -1);
    std::vector<intptr_t> col4row(nr, -1);
    std::vector<intptr_t> row4col(nc, -1);
    std::vector<bool> SR(nr);
    std::vector<bool> SC(nc);
    std::vector<intptr_t> remaining(nc);

    // iteratively build the solution
    for (intptr_t curRow = 0; curRow < nr; curRow++) {

        double minVal;
        intptr_t sink = augmenting_path(nc, costmat, u, v, path, row4col,
                                        shortestPathCosts, curRow, SR, SC,
                                        remaining, &minVal);
        if (sink < 0) {
            return RECTANGULAR_LSAP_INFEASIBLE;
        }

        // update dual variables
        u[curRow] += minVal;
        for (intptr_t i = 0; i < nr; i++) {
            if (SR[i] && i != curRow) {
                u[i] += minVal - shortestPathCosts[col4row[i]];
            }
        }

        for (intptr_t j = 0; j < nc; j++) {
            if (SC[j]) {
                v[j] -= minVal - shortestPathCosts[j];
            }
        }

        // augment previous solution
        intptr_t j = sink;
        while (1) {
            intptr_t i = path[j];
            row4col[j] = i;
            std::swap(col4row[i], j);
            if (i == curRow) {
                break;
            }
        }
    }

    if (transpose) {
        intptr_t i = 0;
        for (auto v: argsort_iter(col4row)) {
            a[i] = col4row[v];
            b[i] = v;
            i++;
        }
    }
    else {
        for (intptr_t i = 0; i < nr; i++) {
            a[i] = i;
            b[i] = col4row[i];
        }
    }

    if (subscript) {
        for (intptr_t i = 0; i < nr; i++) {
            if (subrows != nullptr) {
                a[i] = subrows[a[i]];
            }
            if (subcols != nullptr) {
                b[i] = subcols[b[i]];
            }
        }
    }

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int
solve_rectangular_linear_sum_assignment(intptr_t nr, intptr_t nc,
                                        double* input_cost, bool maximize,
                                        int64_t* a, int64_t* b)
{
    return solve(nr, nc, input_cost, maximize, nullptr, 0, nullptr, 0, a, b);
}


int solve_rectangular_linear_sum_assignment_dtype(
    intptr_t nr, intptr_t nc, void* input_cost, intptr_t dtype, bool maximize,
    const intptr_t *subrows, intptr_t n_subrows, const intptr_t *subcols, intptr_t n_subcols,
    int64_t* a, int64_t* b)
{
    switch (dtype) {
    case LSAP_BOOL:
        return solve(nr, nc, (bool *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_BYTE:
        return solve(nr, nc, (char *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_UBYTE:
        return solve(nr, nc, (unsigned char *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_SHORT:
        return solve(nr, nc, (short *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_USHORT:
        return solve(nr, nc, (unsigned short *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_INT:
        return solve(nr, nc, (int *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_UINT:
        return solve(nr, nc, (unsigned int *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_LONG:
        return solve(nr, nc, (long *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_ULONG:
        return solve(nr, nc, (unsigned long *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_LONGLONG:
        return solve(nr, nc, (long long *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_ULONGLONG:
        return solve(nr, nc, (unsigned long long *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_FLOAT:
        return solve(nr, nc, (float *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_DOUBLE:
        return solve(nr, nc, (double *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    case LSAP_LONGDOUBLE:
        return solve(nr, nc, (long double *)input_cost, maximize, subrows, n_subrows, subcols, n_subcols, a, b);
    default:
        return RECTANGULAR_LSAP_DTYPE_INVALID;
    }
}

#ifdef __cplusplus
}
#endif
