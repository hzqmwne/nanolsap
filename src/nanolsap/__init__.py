from ._lsap import linear_sum_assignment


try:
    try:
        from ._version import __version__
    except Exception:
        import importlib.metadata

        __version__ = importlib.metadata.version("nanolsap")
except Exception:
    pass


__all__ = [
    "linear_sum_assignment",
    "__version__",
]
