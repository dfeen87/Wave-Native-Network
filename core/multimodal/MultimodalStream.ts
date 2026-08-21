import { WaveBuffer } from "../wave_runtime/WaveBuffer";
import { ModalityDescriptor } from "./Schema";

export class MultimodalStream {
    public descriptor: ModalityDescriptor;
    private buffer: WaveBuffer;

    constructor(descriptor: ModalityDescriptor, initialBuffer?: WaveBuffer) {
        this.descriptor = descriptor;
        this.buffer = initialBuffer || WaveBuffer.zeros(0, descriptor.sampleRate);
    }

    public getBuffer(): WaveBuffer {
        return this.buffer;
    }

    public updateBuffer(buffer: WaveBuffer): void {
        this.buffer = buffer;
    }

    public appendData(data: Float32Array | number[]): void {
        const newData = data instanceof Float32Array ? data : new Float32Array(data);
        const combined = new Float32Array(this.buffer.length + newData.length);
        combined.set(this.buffer.data, 0);
        combined.set(newData, this.buffer.length);
        this.buffer = new WaveBuffer(combined, this.descriptor.sampleRate);
    }
}
