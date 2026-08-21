import { WaveBuffer } from "../wave_runtime/WaveBuffer";
import { PipelineGraph } from "./PipelineGraph";

export class PipelineExecutor {
    private graph: PipelineGraph;

    constructor(graph: PipelineGraph) {
        this.graph = graph;
    }

    public async initialize(): Promise<void> {
        const order = this.graph.getTopologicalOrder();
        for (const node of order) {
            if (node.init) {
                await node.init();
            }
        }
    }

    public async execute(input: WaveBuffer): Promise<WaveBuffer> {
        const order = this.graph.getTopologicalOrder();
        let current = input;

        for (const node of order) {
            current = await node.process(current);
        }

        return current;
    }

    public async flush(): Promise<void> {
        const order = this.graph.getTopologicalOrder();
        for (const node of order) {
            if (node.flush) {
                await node.flush();
            }
        }
    }
}
