import { WaveBuffer } from "../core/wave_runtime/WaveBuffer";
import { WaveClock } from "../core/wave_runtime/WaveClock";
import { WaveRuntime } from "../core/wave_runtime/WaveRuntime";
import { WaveProcess } from "../core/wave_runtime/WaveProcess";
import { ConvolutionTransform } from "../core/wave_transforms/ConvolutionTransform";
import { SpectralTransform } from "../core/wave_transforms/SpectralTransform";
import { FusionTransform } from "../core/wave_transforms/FusionTransform";
import { MultimodalStream } from "../core/multimodal/MultimodalStream";
import { AlignmentStrategy } from "../core/multimodal/AlignmentStrategy";
import { MultimodalNode } from "../core/multimodal/MultimodalNode";
import { PipelineGraph } from "../core/pipeline/PipelineGraph";
import { PipelineExecutor } from "../core/pipeline/PipelineExecutor";
import { NodeInterface } from "../core/pipeline/NodeInterface";

describe("Core Wave Native Framework Tests", () => {
    describe("WaveBuffer & WaveClock", () => {
        test("WaveBuffer initializes and clones correctly", () => {
            const buffer = WaveBuffer.fromArray([1.0, 2.0, 3.0, 4.0], 1000);
            expect(buffer.length).toBe(4);
            expect(buffer.duration).toBe(0.004);

            const clone = buffer.clone();
            expect(clone.toArray()).toEqual([1.0, 2.0, 3.0, 4.0]);
            expect(clone.sampleRate).toBe(1000);
        });

        test("WaveClock tracks ticks and time accurately", () => {
            const clock = new WaveClock(100);
            expect(clock.getTime()).toBe(0);
            clock.tick(50);
            expect(clock.getTick()).toBe(50);
            expect(clock.getTime()).toBe(0.5);
            clock.reset();
            expect(clock.getTick()).toBe(0);
        });
    });

    describe("Wave Transforms", () => {
        test("ConvolutionTransform computes 1D convolution", () => {
            const input = WaveBuffer.fromArray([1, 2, 3, 4, 5], 100);
            const kernel = [0.5, 1.0, 0.5];
            const conv = new ConvolutionTransform(kernel);
            const output = conv.apply(input);

            expect(output.length).toBe(5);
            expect(output.data[2]).toBeCloseTo(6.0); // 2*0.5 + 3*1.0 + 4*0.5 = 6.0
        });

        test("SpectralTransform computes DFT magnitudes", () => {
            const input = WaveBuffer.fromArray([1, 0, -1, 0], 100);
            const spectral = new SpectralTransform();
            const output = spectral.apply(input);

            expect(output.length).toBe(4);
            expect(output.complexData).toBeDefined();
            expect(output.complexData!.length).toBe(4);
        });

        test("FusionTransform fuses signals with weights", () => {
            const a = WaveBuffer.fromArray([10, 20], 100);
            const b = WaveBuffer.fromArray([5, 10], 100);
            const fusion = new FusionTransform(0.6, 0.4);
            const out = fusion.apply(a, b);

            expect(out.data[0]).toBeCloseTo(8.0); // 10*0.6 + 5*0.4 = 8
            expect(out.data[1]).toBeCloseTo(16.0); // 20*0.6 + 10*0.4 = 16
        });
    });

    describe("Multimodal Streams & Alignment", () => {
        test("AlignmentStrategy resamples buffer", () => {
            const buf = WaveBuffer.fromArray([1, 2, 3, 4], 100);
            const resampled = AlignmentStrategy.resample(buf, 200);
            expect(resampled.sampleRate).toBe(200);
            expect(resampled.length).toBe(8);
        });

        test("MultimodalNode fuses two streams", () => {
            const streamA = new MultimodalStream({ name: "A", type: "audio", sampleRate: 100, channels: 1 });
            const streamB = new MultimodalStream({ name: "B", type: "video", sampleRate: 100, channels: 1 });
            streamA.appendData([10, 20]);
            streamB.appendData([5, 10]);

            const node = new MultimodalNode("fusion-1");
            const fused = node.fuse(streamA, streamB);

            expect(fused.data[0]).toBeCloseTo(8.0);
            expect(fused.data[1]).toBeCloseTo(16.0);
        });
    });

    describe("Pipeline Execution", () => {
        test("PipelineExecutor runs DAG in topological order", async () => {
            class ScaleNode implements NodeInterface {
                constructor(public id: string, private scale: number) {}
                process(input: WaveBuffer): WaveBuffer {
                    const scaled = input.data.map(v => v * this.scale);
                    return new WaveBuffer(scaled, input.sampleRate);
                }
            }

            const graph = new PipelineGraph();
            const node1 = new ScaleNode("node1", 2);
            const node2 = new ScaleNode("node2", 3);

            graph.addNode(node1);
            graph.addNode(node2);
            graph.addEdge("node1", "node2");

            const executor = new PipelineExecutor(graph);
            await executor.initialize();

            const input = WaveBuffer.fromArray([1, 2, 3], 100);
            const output = await executor.execute(input);

            expect(output.toArray()).toEqual([6, 12, 18]);
        });
    });
});
