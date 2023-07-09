import os
import platform

import numpy
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

PY_LIMITED_API_VERSION = (3, 7)
PY_LIMITED_API_MACRO = f"0x{PY_LIMITED_API_VERSION[0]:02x}{PY_LIMITED_API_VERSION[1]:02x}0000"
PY_LIMITED_API_TAG = f"cp{PY_LIMITED_API_VERSION[0]}{PY_LIMITED_API_VERSION[1]}"

cmdclass = {}

os.environ["LDFLAGS"] = "-s"

# Hack for macos because AppleClang defaults to c++98
# https://stackoverflow.com/questions/70287716/c11-warning-on-macos
# https://github.com/actions/runner-images/blob/main/images/macos/macos-11-Readme.md
# https://stackoverflow.com/questions/47872981/python-extension-using-different-compiler-flags-for-a-c-parts-and-c-parts
# https://stackoverflow.com/questions/8106258/cc1plus-warning-command-line-option-wstrict-prototypes-is-valid-for-ada-c-o/36293331#36293331
# https://stackoverflow.com/questions/15527611/how-do-i-specify-different-compiler-flags-for-just-one-python-c-extension-source
if platform.system().lower() == "darwin":
    class MacosBuildExt(build_ext):
        def build_extensions(self):
            original__compile = self.compiler._compile
            def new__compile(obj, src, ext, cc_args, extra_postargs, pp_opts):
                new_extra_postargs = list(extra_postargs)
                if src.endswith(".cpp") or src.endswith(".cc"):
                    new_extra_postargs.append("-std=c++11")
                return original__compile(obj, src, ext, cc_args, new_extra_postargs, pp_opts)
            self.compiler._compile = new__compile
            try:
                build_ext.build_extensions(self)
            finally:
                self.compiler._compile = original__compile
    cmdclass["build_ext"] = MacosBuildExt

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
