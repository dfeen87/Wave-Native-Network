import { WaveBuffer } from "../wave_runtime/WaveBuffer";

export class FusionTransform {
    public name = "FusionTransform";
    private weightA: number;
    private weightB: number;

    constructor(weightA: number = 0.6, weightB: number = 0.4) {
        this.weightA = weightA;
        this.weightB = weightB;
    }

    public apply(inputA: WaveBuffer, inputB: WaveBuffer): WaveBuffer {
        const n = Math.min(inputA.length, inputB.length);
        const output = new Float32Array(n);

        for (let idx = 0; idx < n; idx++) {
            output[idx] = (inputA.data[idx] * this.weightA) + (inputB.data[idx] * this.weightB);
        }

        return new WaveBuffer(output, inputA.sampleRate);
    }
}
