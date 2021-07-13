from typing import Text


class InferenceError(Exception):
    ...

def infer_shapes(b: bytes, check_type: bool, strict_mode: bool) -> bytes: ...

def infer_shapes_path(model_path: Text, output_path: Text, check_type: bool, strict_mode: bool, data_prop: bool) -> None: ...
