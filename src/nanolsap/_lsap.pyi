from typing import Any, Optional, Tuple

import numpy.typing as npt


def linear_sum_assignment(
    cost_matrix: npt.ArrayLike,
    maximize: bool = False,
    subrows: Optional[npt.ArrayLike] = None,
    subcols: Optional[npt.ArrayLike] = None,
) -> Tuple[npt.NDArray[Any], npt.NDArray[Any]]:
    ...
