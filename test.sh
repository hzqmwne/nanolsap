#!/bin/bash

set -x

rm dist/nanolsap-*
python3 setup.py bdist_wheel
python3 -m pip uninstall -y nanolsap
python3 -m pip install dist/nanolsap-*
python3 -m pytest
