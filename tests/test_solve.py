from nanolsap import linear_sum_assignment as solve


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
