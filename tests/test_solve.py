import numpy as np
from nanolsap import linear_sum_assignment as solve
from scipy.optimize import linear_sum_assignment as scipy_linear_sum_assignment


def test_solve_numpy_square():
    mat = [[82, 83, 69, 92], [77, 37, 49, 92], [11, 69, 5, 86], [8, 9, 98, 23]]
    rows, cols = solve(np.array(mat))
    assert rows.tolist() == [0, 1, 2, 3]
    assert cols.tolist() == [2, 1, 0, 3]


def test_solve_square():
    mat = [[82, 83, 69, 92], [77, 37, 49, 92], [11, 69, 5, 86], [8, 9, 98, 23]]
    rows, cols = solve(mat)
    assert rows.tolist() == [0, 1, 2, 3]
    assert cols.tolist() == [2, 1, 0, 3]


def test_solve_non_square():
    mat = [[82, 92, 69, 83], [77, 92, 49, 37], [11, 86, 5, 69]]
    rows, cols = solve(mat)
    assert rows.tolist() == [0, 1, 2]
    assert cols.tolist() == [2, 3, 0]


def test_solve_non_square_2():
    mat = [[82, 77, 11], [92, 92, 86], [69, 49, 5], [83, 37, 69]]
    rows, cols = solve(mat)
    print(rows, cols, flush=True)
    assert rows.tolist() == [0, 2, 3]
    assert cols.tolist() == [2, 0, 1]


def test_solve_empty():
    rows, cols = solve([[]])
    assert rows.tolist() == []
    assert cols.tolist() == []


def test_random_data():
    np.random.seed(1234)
    for _ in range(100):
        row_size = np.random.randint(51, 100)
        col_size = np.random.randint(51, 100)
        dense = np.random.random((row_size, col_size))
        lsa_raises = False
        scipy_raises = False
        try:
            row_ind, col_ind = solve(dense)
            lsa_cost = dense[row_ind, col_ind].sum()
        except ValueError:
            lsa_raises = True
        try:
            row_ind, col_ind = scipy_linear_sum_assignment(dense)
            scipy_cost = dense[row_ind, col_ind].sum()
        except ValueError:
            scipy_raises = True
        # Ensure that if one method raises, so does the other one.
        assert lsa_raises == scipy_raises
        if not lsa_raises:
            assert lsa_cost == scipy_cost
        else:
            assert False
