import { WaveBuffer } from "../wave_runtime/WaveBuffer";
import { FusionTransform } from "../wave_transforms/FusionTransform";
import { AlignmentStrategy } from "./AlignmentStrategy";
import { MultimodalStream } from "./MultimodalStream";

export class MultimodalNode {
    public id: string;
    private fusionTransform: FusionTransform;

    constructor(id: string, weightA: number = 0.6, weightB: number = 0.4) {
        this.id = id;
        this.fusionTransform = new FusionTransform(weightA, weightB);
    }

    public fuse(streamA: MultimodalStream, streamB: MultimodalStream): WaveBuffer {
        const bufA = streamA.getBuffer();
        const bufB = streamB.getBuffer();
        const { alignedA, alignedB } = AlignmentStrategy.alignBuffers(bufA, bufB);
        return this.fusionTransform.apply(alignedA, alignedB);
    }
}
