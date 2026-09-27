import { WaveBuffer } from "../wave_runtime/WaveBuffer";
import { TransformInterface } from "./TransformInterface";

export class ConvolutionTransform implements TransformInterface {
    public name = "ConvolutionTransform";
    private kernel: Float32Array;

    constructor(kernel: number[] | Float32Array) {
        this.kernel = kernel instanceof Float32Array ? kernel : new Float32Array(kernel);
    }

    public apply(input: WaveBuffer): WaveBuffer {
        const signal = input.data;
        const n = signal.length;
        const k = this.kernel.length;
        const half = Math.floor(k / 2);
        const output = new Float32Array(n);

        for (let idx = 0; idx < n; idx++) {
            let sum = 0;
            for (let kernelIdx = 0; kernelIdx < k; kernelIdx++) {
                const s = idx + kernelIdx - half;
                if (s >= 0 && s < n) {
                    sum += signal[s] * this.kernel[kernelIdx];
                }
            }
            output[idx] = sum;
        }

        return new WaveBuffer(output, input.sampleRate);
    }
}
