import { WaveBuffer, ComplexNumber } from "../wave_runtime/WaveBuffer";
import { TransformInterface } from "./TransformInterface";

export class SpectralTransform implements TransformInterface {
    public name = "SpectralTransform";

    public apply(input: WaveBuffer): WaveBuffer {
        const signal = input.data;
        const n = signal.length;
        const complexOutput: ComplexNumber[] = new Array(n);
        const magnitudeOutput = new Float32Array(n);

        for (let idx = 0; idx < n; idx++) {
            let real = 0.0;
            let imag = 0.0;

            for (let k = 0; k < n; k++) {
                const angle = -2.0 * Math.PI * idx * k / n;
                real += signal[k] * Math.cos(angle);
                imag += signal[k] * Math.sin(angle);
            }

            complexOutput[idx] = { real, imag };
            magnitudeOutput[idx] = Math.sqrt(real * real + imag * imag);
        }

        return new WaveBuffer(magnitudeOutput, input.sampleRate, complexOutput);
    }
}
