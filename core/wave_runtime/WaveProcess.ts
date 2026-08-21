import { WaveBuffer } from "./WaveBuffer";

export interface WaveProcess {
    id: string;
    name: string;
    process(input: WaveBuffer): WaveBuffer | Promise<WaveBuffer>;
    reset?(): void;
}
