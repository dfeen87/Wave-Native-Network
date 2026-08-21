import { CudaBackend } from "../accel/cuda/cuda_backend";
import { TensorRTBackend } from "../accel/tensorrt/tensorrt_backend";
import { NimBackend } from "../accel/nim/nim_backend";

export type AccelMode = "cpu" | "cuda" | "tensorrt" | "nim";

export class WaveAcceleratorProvider {
    static getBackend(mode: "cuda"): CudaBackend;
    static getBackend(mode: "tensorrt"): TensorRTBackend;
    static getBackend(mode: "nim"): NimBackend;
    static getBackend(mode: "cpu"): null;
    static getBackend(mode: AccelMode): CudaBackend | TensorRTBackend | NimBackend | null;
    static getBackend(mode: AccelMode) {
        switch (mode) {
            case "cuda":
                return new CudaBackend();
            case "tensorrt":
                return new TensorRTBackend();
            case "nim":
                return new NimBackend();
            case "cpu":
            default:
                return null;
        }
    }
}
