# nanolsap

A Python module to solve the linear sum assignment problem (LSAP) efficiently.
Extracted from SciPy, and modified to minimal memory cost.

## Description

```
from nanolsap import linear_sum_assignment
```
```
Solve the linear sum assignment problem.

Parameters
----------
cost_matrix : array
    The cost matrix of the bipartite graph.

maximize : bool (default: False)
    Calculates a maximum weight matching if true.

subrows : array (default: None)
    Use sub rows from cost matrix if not None.

subcols : array (default: None)
    Use sub cols from cost matrix if not None.

Returns
-------
row_ind, col_ind : array
    An array of row indices and one of corresponding column indices giving
    the optimal assignment. The cost of the assignment can be computed
    as ``cost_matrix[row_ind, col_ind].sum()``. The row indices will be
    sorted if subrows and subcols are both None; in the case of a square cost
    matrix they will be equal to ``numpy.arange(cost_matrix.shape[0])``.
```

This module is useful in cases when you need an efficient LSAP solver on 
very large cost_matrix and limited memory. 

For a cost_matrix with dtype float64 has shape of 30000\*30000, it needs  30000\*30000\*8 / 1024\*\*3 = 6.7GB memory to store it. 
In scipy.optimite.linear_sum_assignment, it will first convert it to float64 contiguous numpy 2-D array, then do a copy, and finally starts the solver. 
So, the actual memory cost is at least 6.7\*2 = 13.4GB. 
if the origin cost_matrix does not match, for example a float32 2-D array, 
the  first step here will cause one extra copy, increases the actual memory cost to 6.7/2+6.7\*2 = 16.75GB. 

In this module, When input cost_matrix is a contiguous numpy 2-D array, the solver can run on it directly without any copy. 
Also, cost_matrix can use small dtype like float32 to half reduce memory, so 3.35GB memory is enough. 
It may sacrifice a little performance, but it is worth for the huge saving of memory.

The subrows and subcols arguments allow solver run on only a subgroup of row and cols on cost_matrix. 
The result should be same as scipy.optimize.linear_sum_assignment(cost_matrix[np.ix_(subrows, subcols)]), but it avoids the expensive construct of sub cost_matrix.

## License

The code in this repository is licensed under the 3-clause BSD license, except
for files including a different license header.

The LSAP solver copied from [SciPy](https://github.com/scipy/scipy ) is also licensed under the 3-clause BSD
license.

Some files copied from [minilsap](https://github.com/ntamas/lsap ) are  also licensed under the 3-clause BSD license.
