import { CudaBackend } from "../accel/cuda/cuda_backend";
import { TensorRTBackend } from "../accel/tensorrt/tensorrt_backend";
import { NimBackend } from "../accel/nim/nim_backend";
import { CudaAdapter } from "../adapters/cuda_adapter";
import { TensorRTAdapter } from "../adapters/tensorrt_adapter";
import { NimAdapter } from "../adapters/nim_adapter";
import { WaveAcceleratorProvider } from "../providers/WaveAcceleratorProvider";

describe("WN-Accel Acceleration Layer Tests", () => {
    describe("WaveAcceleratorProvider", () => {
        test("instantiates appropriate backends based on mode", () => {
            const cpuBackend = WaveAcceleratorProvider.getBackend("cpu");
            expect(cpuBackend).toBeNull();

            const cudaBackend = WaveAcceleratorProvider.getBackend("cuda");
            expect(cudaBackend).toBeInstanceOf(CudaBackend);

            const trtBackend = WaveAcceleratorProvider.getBackend("tensorrt");
            expect(trtBackend).toBeInstanceOf(TensorRTBackend);

            const nimBackend = WaveAcceleratorProvider.getBackend("nim");
            expect(nimBackend).toBeInstanceOf(NimBackend);
        });
    });

    describe("CUDA Adapter", () => {
        test("convolve executes convolution", () => {
            const adapter = new CudaAdapter();
            const result = adapter.convolve([1, 2, 3, 4, 5], [0.5, 1.0, 0.5]);
            expect(result.length).toBe(5);
            expect(result[2]).toBeCloseTo(6.0);
        });

        test("convolve supports even-length kernels", () => {
            const adapter = new CudaAdapter();
            expect(adapter.convolve([1, 2, 3, 4], [1, 2])).toEqual([2, 5, 8, 11]);
        });

        test("spectralTransform executes spectral transform", () => {
            const adapter = new CudaAdapter();
            const result = adapter.spectralTransform([1, 0, -1, 0]);
            expect(result.length).toBe(4);
        });

        test("fuse executes multimodal fusion", () => {
            const adapter = new CudaAdapter();
            const result = adapter.fuse([10, 20], [5, 10]);
            expect(result[0]).toBeCloseTo(8.0);
            expect(result[1]).toBeCloseTo(16.0);
        });
    });

    describe("TensorRT Adapter", () => {
        test("infer processes signal data", () => {
            const adapter = new TensorRTAdapter();
            const result = adapter.infer([1.0, 2.0, 3.0]);
            expect(result).toEqual([1.0, 2.0, 3.0]);
        });
    });

    describe("NIM Adapter", () => {
        test("infer processes signal data asynchronously", async () => {
            const adapter = new NimAdapter();
            const result = await adapter.infer([1.0, 2.0, 3.0]);
            expect(result).toEqual([1.0, 2.0, 3.0]);
        });
    });
});
