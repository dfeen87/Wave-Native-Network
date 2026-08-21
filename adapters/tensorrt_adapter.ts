import { TensorRTBackend } from "../accel/tensorrt/tensorrt_backend";

export class TensorRTAdapter {
    constructor(private backend = new TensorRTBackend()) {}

    public infer(signal: number[]): number[] {
        return this.backend.infer(signal);
    }
}
