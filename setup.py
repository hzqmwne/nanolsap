import os
import numpy

from setuptools import setup, Extension

PY_LIMITED_API_VERSION = (3, 7)
PY_LIMITED_API_MACRO = f"0x{PY_LIMITED_API_VERSION[0]:02x}{PY_LIMITED_API_VERSION[1]:02x}0000"
PY_LIMITED_API_TAG = f"cp{PY_LIMITED_API_VERSION[0]}{PY_LIMITED_API_VERSION[1]}"

cmdclass = {}

os.environ["LDFLAGS"] = "-s"
setup_args = dict(
    ext_modules=[
        Extension(
            "nanolsap._lsap",
            ["src/nanolsap/_lsap.c", "src/nanolsap/rectangular_lsap/rectangular_lsap.cpp"],
            py_limited_api=True,
            include_dirs=[numpy.get_include()],
            define_macros=[("Py_LIMITED_API", PY_LIMITED_API_MACRO), ("PY_SSIZE_T_CLEAN", 1)],
        )
    ],
    cmdclass=cmdclass,
    options=dict(
        bdist_wheel={
            "py_limited_api": PY_LIMITED_API_TAG,
        },
    )
)
setup(**setup_args)
