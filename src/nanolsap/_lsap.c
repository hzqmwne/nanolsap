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
*/

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"
#include "numpy/ndarraytypes.h"
#include "rectangular_lsap/rectangular_lsap.h"


static intptr_t convert_npy_typ_to_lsap_typ(intptr_t npy_typ) {
    switch (npy_typ) {
    case NPY_BOOL:
        return LSAP_BOOL;
    case NPY_BYTE:
        return LSAP_BYTE;
    case NPY_UBYTE:
        return LSAP_UBYTE;
    case NPY_SHORT:
        return LSAP_SHORT;
    case NPY_USHORT:
        return LSAP_USHORT;
    case NPY_INT:
        return LSAP_INT;
    case NPY_UINT:
        return LSAP_UINT;
    case NPY_LONG:
        return LSAP_LONG;
    case NPY_ULONG:
        return LSAP_ULONG;
    case NPY_LONGLONG:
        return LSAP_LONGLONG;
    case NPY_ULONGLONG:
        return LSAP_ULONGLONG;
    case NPY_FLOAT:
        return LSAP_FLOAT;
    case NPY_DOUBLE:
        return LSAP_DOUBLE;
    case NPY_LONGDOUBLE:
        return LSAP_LONGDOUBLE;
    default:
        return LSAP_INVALID;
    }
}

static PyObject*
linear_sum_assignment(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* a = NULL;
    PyObject* b = NULL;
    PyObject* result = NULL;
    PyObject* obj_cost = NULL;
    PyArrayObject* obj_cont = NULL;
    int maximize = 0;
    PyObject* obj_subrows = Py_None;
    PyObject* obj_subcols = Py_None;
    PyArrayObject* array_subrows = NULL;
    PyArrayObject* array_subcols = NULL;
    intptr_t *subrows = NULL;
    intptr_t n_subrows = 0;
    intptr_t *subcols = NULL;
    intptr_t n_subcols = 0;
    static const char *kwlist[] = { (const char*)"cost_matrix",
                                    (const char*)"maximize",
                                    (const char*)"subrows",
                                    (const char*)"subcols",
                                    NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|pOO", (char**)kwlist,
                                     &obj_cost, &maximize, &obj_subrows, &obj_subcols)) {
        return NULL;
    }

    intptr_t npy_typ = NPY_DOUBLE;
    intptr_t dtype = LSAP_DOUBLE;
    if (PyArray_Check(obj_cost)) {
        intptr_t tmp_npy_typ = PyArray_TYPE((PyArrayObject*)obj_cost);
        intptr_t tmp_dtype = convert_npy_typ_to_lsap_typ(tmp_npy_typ);
        if (tmp_dtype != LSAP_INVALID) {
            npy_typ = tmp_npy_typ;
            dtype = tmp_dtype;
        }
    }

    obj_cont = (PyArrayObject*)PyArray_ContiguousFromAny(obj_cost, npy_typ, 0, 0);
    if (!obj_cont) {
        return NULL;
    }

    if (PyArray_NDIM(obj_cont) != 2) {
        PyErr_Format(PyExc_ValueError,
                     "expected a matrix (2-D array), got a %d array",
                     PyArray_NDIM(obj_cont));
        goto cleanup;
    }

    void* cost_matrix = PyArray_DATA(obj_cont);
    if (cost_matrix == NULL) {
        PyErr_SetString(PyExc_TypeError, "invalid cost matrix object");
        goto cleanup;
    }

    if (obj_subrows != Py_None) {
        array_subrows = (PyArrayObject*)PyArray_ContiguousFromAny(obj_subrows, NPY_INTP, 0, 0);
        if (!array_subrows) {
            return NULL;
        }
        if (PyArray_NDIM(array_subrows) != 1) {
            PyErr_Format(PyExc_ValueError,
                        "subrows expected a 1-D array, got a %d array",
                        PyArray_NDIM(array_subrows));
            goto cleanup;
        }
        subrows = (intptr_t *)PyArray_DATA(array_subrows);
        if (subrows == NULL) {
            PyErr_SetString(PyExc_TypeError, "invalid subrows array object");
            goto cleanup;
        }
        n_subrows = PyArray_DIM(array_subrows, 0);
    }
    if (obj_subcols != Py_None) {
        array_subcols = (PyArrayObject*)PyArray_ContiguousFromAny(obj_subcols, NPY_INTP, 0, 0);
        if (!array_subcols) {
            return NULL;
        }
        if (PyArray_NDIM(array_subcols) != 1) {
            PyErr_Format(PyExc_ValueError,
                        "subcols expected a 1-D array, got a %d array",
                        PyArray_NDIM(array_subcols));
            goto cleanup;
        }
        subcols = (intptr_t *)PyArray_DATA(array_subcols);
        if (subcols == NULL) {
            PyErr_SetString(PyExc_TypeError, "invalid subcols array object");
            goto cleanup;
        }
        n_subcols = PyArray_DIM(array_subcols, 0);
    }

    npy_intp num_rows = PyArray_DIM(obj_cont, 0);
    npy_intp num_cols = PyArray_DIM(obj_cont, 1);
    npy_intp dim_num_rows = n_subrows ? n_subrows : num_rows;
    npy_intp dim_num_cols = n_subcols ? n_subcols : num_cols;
    npy_intp dim[1] = { dim_num_rows < dim_num_cols ? dim_num_rows : dim_num_cols };
    a = PyArray_SimpleNew(1, dim, NPY_INT64);
    if (!a)
        goto cleanup;

    b = PyArray_SimpleNew(1, dim, NPY_INT64);
    if (!b)
        goto cleanup;

    int ret = solve_rectangular_linear_sum_assignment_dtype(
      num_rows, num_cols, cost_matrix, dtype, maximize,
      subrows, n_subrows, subcols, n_subcols,
      PyArray_DATA((PyArrayObject*)a),
      PyArray_DATA((PyArrayObject*)b));
    if (ret == RECTANGULAR_LSAP_INFEASIBLE) {
        PyErr_SetString(PyExc_ValueError, "cost matrix is infeasible");
        goto cleanup;
    }
    else if (ret == RECTANGULAR_LSAP_INVALID) {
        PyErr_SetString(PyExc_ValueError,
                        "matrix contains invalid numeric entries");
        goto cleanup;
    }
    else if (ret == RECTANGULAR_LSAP_SUBSCRIPT_INVALID) {
        PyErr_SetString(PyExc_ValueError,
                        "subrows or subcols is invalid");
        goto cleanup;
    }

    result = Py_BuildValue("OO", a, b);

cleanup:
    Py_XDECREF((PyObject*)array_subcols);
    Py_XDECREF((PyObject*)array_subrows);
    Py_XDECREF((PyObject*)obj_cont);
    Py_XDECREF(a);
    Py_XDECREF(b);
    return result;
}

static PyMethodDef lsap_methods[] = {
    { "linear_sum_assignment",
      (PyCFunction)linear_sum_assignment,
      METH_VARARGS | METH_KEYWORDS,
"Solve the linear sum assignment problem.\n"
"\n"
"Parameters\n"
"----------\n"
"cost_matrix : array\n"
"    The cost matrix of the bipartite graph.\n"
"\n"
"maximize : bool (default: False)\n"
"    Calculates a maximum weight matching if true.\n"
"\n"
"subrows : array (default: None)\n"
"    Use sub rows from cost matrix if not None.\n"
"\n"
"subcols : array (default: None)\n"
"    Use sub cols from cost matrix if not None.\n"
"\n"
"Returns\n"
"-------\n"
"row_ind, col_ind : array\n"
"    An array of row indices and one of corresponding column indices giving\n"
"    the optimal assignment. The cost of the assignment can be computed\n"
"    as ``cost_matrix[row_ind, col_ind].sum()``. The row indices will be\n"
"    sorted; in the case of a square cost matrix they will be equal to\n"
"    ``numpy.arange(cost_matrix.shape[0])``.\n"
"\n"
"See Also\n"
"--------\n"
"scipy.sparse.csgraph.min_weight_full_bipartite_matching : for sparse inputs\n"
"\n"
"Notes\n"
"-----\n"
"\n"
"The linear sum assignment problem [1]_ is also known as minimum weight\n"
"matching in bipartite graphs. A problem instance is described by a matrix\n"
"C, where each C[i,j] is the cost of matching vertex i of the first partite\n"
"set (a 'worker') and vertex j of the second set (a 'job'). The goal is to\n"
"find a complete assignment of workers to jobs of minimal cost.\n"
"\n"
"Formally, let X be a boolean matrix where :math:`X[i,j] = 1` iff row i is\n"
"assigned to column j. Then the optimal assignment has cost\n"
"\n"
".. math::\n"
"    \\min \\sum_i \\sum_j C_{i,j} X_{i,j}\n"
"\n"
"where, in the case where the matrix X is square, each row is assigned to\n"
"exactly one column, and each column to exactly one row.\n"
"\n"
"This function can also solve a generalization of the classic assignment\n"
"problem where the cost matrix is rectangular. If it has more rows than\n"
"columns, then not every row needs to be assigned to a column, and vice\n"
"versa.\n"
"\n"
"This implementation is a modified Jonker-Volgenant algorithm with no\n"
"initialization, described in ref. [2]_.\n"
"\n"
".. versionadded:: 0.17.0\n"
"\n"
"References\n"
"----------\n"
"\n"
".. [1] https://en.wikipedia.org/wiki/Assignment_problem\n"
"\n"
".. [2] DF Crouse. On implementing 2D rectangular assignment algorithms.\n"
"       *IEEE Transactions on Aerospace and Electronic Systems*,\n"
"       52(4):1679-1696, August 2016, :doi:`10.1109/TAES.2016.140952`\n"
"\n"
"Examples\n"
"--------\n"
">>> import numpy as np\n"
">>> cost = np.array([[4, 1, 3], [2, 0, 5], [3, 2, 2]])\n"
">>> from scipy.optimize import linear_sum_assignment\n"
">>> row_ind, col_ind = linear_sum_assignment(cost)\n"
">>> col_ind\n"
"array([1, 0, 2])\n"
">>> cost[row_ind, col_ind].sum()\n"
"5\n"},
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_lsap",
    "Solves the rectangular linear sum assignment.",
    -1,
    lsap_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit__lsap(void)
{
    import_array();
    return PyModule_Create(&moduledef);
}
