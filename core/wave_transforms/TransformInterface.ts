import { WaveBuffer } from "../wave_runtime/WaveBuffer";

export interface TransformInterface {
    name: string;
    apply(input: WaveBuffer): WaveBuffer;
}
