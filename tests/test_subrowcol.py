import numpy as np
from nanolsap import linear_sum_assignment as solve
from scipy.optimize import linear_sum_assignment as scipy_linear_sum_assignment


def test_simple():
    dense = [[1, 0, 2, 1], [0, 0, 0, 0], [1, 0, 1, 2]]
    subrows = [0, 2]
    subcols = [0, 2, 3]
    row_ind, col_ind = solve(dense, True, subrows, subcols)
    assert row_ind.tolist() == [0, 2]
    assert col_ind.tolist() == [2, 3]


def test_subrow_subcol_random():
    np.random.seed(1234)
    for i in range(100):
        row_size = np.random.randint(51, 100)
        col_size = np.random.randint(51, 100)
        dense = np.random.random((row_size, col_size))
        subdim = (min(row_size, col_size) // 2) * (1+(i % 2)*2)
        subrows = np.random.choice(range(row_size), subdim)
        subcols = np.random.choice(range(col_size), subdim)
        lsa_raises = False
        scipy_raises = False
        try:
            row_ind, col_ind = solve(dense, False, subrows, subcols)
            lsa_cost = dense[row_ind, col_ind].sum()
        except ValueError:
            lsa_raises = True
        try:
            subdense = dense[np.ix_(subrows, subcols)]
            row_ind, col_ind = scipy_linear_sum_assignment(subdense)
            scipy_cost = subdense[row_ind, col_ind].sum()
        except ValueError:
            scipy_raises = True
        # Ensure that if one method raises, so does the other one.
        assert lsa_raises == scipy_raises
        if not lsa_raises:
            assert lsa_cost == scipy_cost
        else:
            assert False
