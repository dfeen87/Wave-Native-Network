import os
import json
import numpy as np
import pytest
from fastapi.testclient import TestClient

from accel.tensorrt.build_engine import build_engine
from accel.tensorrt.infer import infer
from accel.nim.server import app

def test_build_engine_fallback(tmp_path):
    model_path = str(tmp_path / "model.onnx")
    engine_path = str(tmp_path / "engine.trt")
    with open(model_path, "w") as f:
        f.write("dummy_onnx_content")

    res = build_engine(model_path, engine_path)
    assert os.path.exists(engine_path)

def test_infer_numpy_array():
    data = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    output = infer("dummy.trt", data)
    assert isinstance(output, np.ndarray)
    np.testing.assert_array_almost_equal(output, data)

def test_nim_server_fastapi():
    client = TestClient(app)
    health_res = client.get("/health")
    assert health_res.status_code == 200
    assert health_res.json()["status"] == "ok"

    infer_res = client.post("/wave", json={"signal": [1.0, 2.0, 3.0]})
    assert infer_res.status_code == 200
    assert infer_res.json()["output"] == [1.0, 2.0, 3.0]
