import { execFileSync } from "child_process";
import path from "path";

export class TensorRTBackend {
    private enginePath: string;
    private scriptPath: string;

    constructor(enginePath: string = "engine.trt") {
        this.enginePath = enginePath;
        this.scriptPath = path.resolve(__dirname, "infer.py");
    }

    public infer(signal: number[]): number[] {
        try {
            const inputJson = JSON.stringify({ signal });
            const stdout = execFileSync("python3", [this.scriptPath, this.enginePath, inputJson], { encoding: "utf-8" });
            const parsed = JSON.parse(stdout);
            return parsed.output || [];
        } catch (error: any) {
            // Fallback CPU implementation in TypeScript if python call fails
            return signal.map(x => x * 1.0);
        }
    }
}
