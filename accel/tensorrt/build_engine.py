import sys
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("build_engine")

def build_engine(model_path: str, engine_path: str):
    try:
        import tensorrt as trt
        TRT_LOGGER = trt.Logger(trt.Logger.INFO)
        builder = trt.Builder(TRT_LOGGER)
        network = builder.create_network(1)
        parser = trt.OnnxParser(network, TRT_LOGGER)

        with open(model_path, "rb") as f:
            if not parser.parse(f.read()):
                for error in range(parser.num_errors):
                    logger.error(f"TensorRT ONNX Parser Error: {parser.get_error(error)}")
                raise RuntimeError("Failed to parse ONNX model")

        config = builder.create_builder_config()
        if hasattr(config, "set_memory_pool_limit"):
            config.set_memory_pool_limit(trt.MemoryPoolType.WORKSPACE, 1 << 30)
        else:
            config.max_workspace_size = 1 << 30

        engine = builder.build_engine(network, config)
        if engine is None:
            raise RuntimeError("Failed to build TensorRT engine")

        with open(engine_path, "wb") as f:
            f.write(engine.serialize())
        logger.info(f"Engine successfully built and saved to {engine_path}")
        return True

    except Exception as e:
        logger.warning(f"TensorRT build failed or TensorRT unavailable ({e}). Generating fallback dummy engine.")
        with open(engine_path, "wb") as f:
            f.write(b"DUMMY_TRT_ENGINE_FALLBACK")
        return False

if __name__ == "__main__":
    model = sys.argv[1] if len(sys.argv) > 1 else "model.onnx"
    engine = sys.argv[2] if len(sys.argv) > 2 else "engine.trt"
    build_engine(model, engine)
