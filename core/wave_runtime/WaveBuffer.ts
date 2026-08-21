export interface ComplexNumber {
    real: number;
    imag: number;
}

export class WaveBuffer {
    public data: Float32Array;
    public complexData?: ComplexNumber[];
    public sampleRate: number;

    constructor(data: Float32Array | number[], sampleRate: number = 44100, complexData?: ComplexNumber[]) {
        this.data = data instanceof Float32Array ? data : new Float32Array(data);
        this.sampleRate = sampleRate;
        this.complexData = complexData;
    }

    get length(): number {
        return this.data.length;
    }

    get duration(): number {
        return this.data.length / this.sampleRate;
    }

    public clone(): WaveBuffer {
        const copyData = new Float32Array(this.data);
        const copyComplex = this.complexData ? this.complexData.map(c => ({ ...c })) : undefined;
        return new WaveBuffer(copyData, this.sampleRate, copyComplex);
    }

    public static zeros(length: number, sampleRate: number = 44100): WaveBuffer {
        return new WaveBuffer(new Float32Array(length), sampleRate);
    }

    public static fromArray(arr: number[], sampleRate: number = 44100): WaveBuffer {
        return new WaveBuffer(new Float32Array(arr), sampleRate);
    }

    public toArray(): number[] {
        return Array.from(this.data);
    }
}
