import { WaveBuffer } from "../wave_runtime/WaveBuffer";

export interface NodeInterface {
    id: string;
    init?(): Promise<void> | void;
    process(input: WaveBuffer): Promise<WaveBuffer> | WaveBuffer;
    flush?(): Promise<WaveBuffer | void> | WaveBuffer | void;
}
