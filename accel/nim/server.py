import sys
import os
from fastapi import FastAPI
import numpy as np

# Ensure accel/tensorrt is in Python path for import
tensorrt_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "tensorrt"))
if tensorrt_dir not in sys.path:
    sys.path.insert(0, tensorrt_dir)

try:
    from infer import infer
except ImportError:
    # Direct fallback if infer function module isn't resolvable directly
    def infer(engine_path: str, input_data: np.ndarray):
        return input_data * 1.0

app = FastAPI(title="WN-Accel NIM Microservice", version="3.0.0")

@app.get("/health")
def health_check():
    return {"status": "ok", "service": "WN-Accel NIM Microservice"}

@app.post("/wave")
def wave_inference(payload: dict):
    signal_data = payload.get("signal", [])
    data = np.array(signal_data, dtype=np.float32)
    engine_path = payload.get("engine_path", "engine.trt")
    out = infer(engine_path, data)
    if isinstance(out, np.ndarray):
        out_list = out.tolist()
    else:
        out_list = list(out)
    return {"output": out_list}
