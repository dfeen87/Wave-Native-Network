export class WaveClock {
    private sampleRate: number;
    private currentTick: number;

    constructor(sampleRate: number = 44100) {
        if (!Number.isFinite(sampleRate) || sampleRate <= 0) {
            throw new RangeError("sampleRate must be a positive, finite number.");
        }
        this.sampleRate = sampleRate;
        this.currentTick = 0;
    }

    public getSampleRate(): number {
        return this.sampleRate;
    }

    public getTick(): number {
        return this.currentTick;
    }

    public getTime(): number {
        return this.currentTick / this.sampleRate;
    }

    public tick(count: number = 1): number {
        this.currentTick += count;
        return this.currentTick;
    }

    public reset(): void {
        this.currentTick = 0;
    }

    public sampleToTime(sampleIndex: number): number {
        return sampleIndex / this.sampleRate;
    }

    public timeToSample(timeSeconds: number): number {
        return Math.floor(timeSeconds * this.sampleRate);
    }
}
