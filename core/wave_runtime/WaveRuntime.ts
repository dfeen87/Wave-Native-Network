import { WaveBuffer } from "./WaveBuffer";
import { WaveClock } from "./WaveClock";
import { WaveProcess } from "./WaveProcess";

export class WaveRuntime {
    private clock: WaveClock;
    private processes: Map<string, WaveProcess>;

    constructor(sampleRate: number = 44100) {
        this.clock = new WaveClock(sampleRate);
        this.processes = new Map();
    }

    public getClock(): WaveClock {
        return this.clock;
    }

    public registerProcess(process: WaveProcess): void {
        this.processes.set(process.id, process);
    }

    public unregisterProcess(processId: string): boolean {
        return this.processes.delete(processId);
    }

    public getProcess(processId: string): WaveProcess | undefined {
        return this.processes.get(processId);
    }

    public async stepProcess(processId: string, input: WaveBuffer): Promise<WaveBuffer> {
        const proc = this.processes.get(processId);
        if (!proc) {
            throw new Error(`Process with id '${processId}' not found in WaveRuntime.`);
        }
        const output = await proc.process(input);
        this.clock.tick(input.length);
        return output;
    }

    public async runPipelineSequence(processIds: string[], input: WaveBuffer): Promise<WaveBuffer> {
        let currentBuffer = input;
        for (const pid of processIds) {
            currentBuffer = await this.stepProcess(pid, currentBuffer);
        }
        return currentBuffer;
    }

    public reset(): void {
        this.clock.reset();
        for (const proc of this.processes.values()) {
            if (proc.reset) {
                proc.reset();
            }
        }
    }
}
