from typing import Optional, Tuple

import numpy as np
import numpy.typing as npt


def linear_sum_assignment(
    cost_matrix: npt.ArrayLike,
    maximize: bool = False,
    subrows: Optional[npt.ArrayLike] = None,
    subcols: Optional[npt.ArrayLike] = None,
) -> Tuple[np.ndarray, np.ndarray]:
    ...
