import { WaveBuffer } from "../wave_runtime/WaveBuffer";

export class AlignmentStrategy {
    public static resample(buffer: WaveBuffer, targetSampleRate: number): WaveBuffer {
        if (!Number.isFinite(targetSampleRate) || targetSampleRate <= 0) {
            throw new RangeError("targetSampleRate must be a positive, finite number.");
        }
        if (buffer.sampleRate === targetSampleRate || buffer.length === 0) {
            return buffer.clone();
        }

        const ratio = targetSampleRate / buffer.sampleRate;
        const targetLength = Math.floor(buffer.length * ratio);
        const output = new Float32Array(targetLength);

        for (let i = 0; i < targetLength; i++) {
            const srcIdx = i / ratio;
            const index = Math.floor(srcIdx);
            const frac = srcIdx - index;

            if (index + 1 < buffer.length) {
                output[i] = buffer.data[index] * (1 - frac) + buffer.data[index + 1] * frac;
            } else if (index < buffer.length) {
                output[i] = buffer.data[index];
            } else {
                output[i] = 0;
            }
        }

        return new WaveBuffer(output, targetSampleRate);
    }

    public static alignBuffers(a: WaveBuffer, b: WaveBuffer): { alignedA: WaveBuffer; alignedB: WaveBuffer } {
        const targetRate = Math.max(a.sampleRate, b.sampleRate);
        let resampledA = a.sampleRate !== targetRate ? AlignmentStrategy.resample(a, targetRate) : a;
        let resampledB = b.sampleRate !== targetRate ? AlignmentStrategy.resample(b, targetRate) : b;

        const targetLen = Math.min(resampledA.length, resampledB.length);
        const slicedA = new WaveBuffer(resampledA.data.slice(0, targetLen), targetRate);
        const slicedB = new WaveBuffer(resampledB.data.slice(0, targetLen), targetRate);

        return { alignedA: slicedA, alignedB: slicedB };
    }
}
