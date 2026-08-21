import { CudaBackend } from "../accel/cuda/cuda_backend";

export class CudaAdapter {
    constructor(private backend = new CudaBackend()) {}

    public convolve(signal: number[], kernel: number[]): number[] {
        const rawOutput = this.backend.runKernel("wave_convolution", [
            JSON.stringify(signal),
            JSON.stringify(kernel)
        ]);
        return JSON.parse(rawOutput.toString("utf-8"));
    }

    public spectralTransform(signal: number[]): number[] {
        const rawOutput = this.backend.runKernel("spectral_transform", [
            JSON.stringify(signal)
        ]);
        return JSON.parse(rawOutput.toString("utf-8"));
    }

    public fuse(a: number[], b: number[]): number[] {
        const rawOutput = this.backend.runKernel("multimodal_fusion", [
            JSON.stringify(a),
            JSON.stringify(b)
        ]);
        return JSON.parse(rawOutput.toString("utf-8"));
    }
}
