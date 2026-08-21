import { execSync } from "child_process";
import fs from "fs";
import path from "path";

export class CudaBackend {
    private cudaAvailable: boolean;

    constructor() {
        this.cudaAvailable = this.checkCudaAvailability();
    }

    private checkCudaAvailability(): boolean {
        try {
            execSync("which nvcc", { stdio: "ignore" });
            return true;
        } catch {
            return false;
        }
    }

    public isAvailable(): boolean {
        return this.cudaAvailable;
    }

    public runKernel(kernelName: string, args: string[]): Buffer {
        // Look for built executable or fallback
        const possiblePath = path.resolve(process.cwd(), kernelName);
        const altPath = path.resolve(__dirname, "kernels", kernelName);
        let execPath = kernelName;

        if (fs.existsSync(possiblePath)) {
            execPath = possiblePath;
        } else if (fs.existsSync(altPath)) {
            execPath = altPath;
        }

        const formattedArgs = args.map(arg => `'${arg}'`).join(" ");
        const cmd = `${execPath} ${formattedArgs}`;

        try {
            return execSync(cmd, { stdio: ["pipe", "pipe", "ignore"] });
        } catch {
            // Return CPU simulated JSON buffer if GPU binary or execution is unavailable
            return Buffer.from(this.fallbackCpuExecute(kernelName, args));
        }
    }

    private fallbackCpuExecute(kernelName: string, args: string[]): string {
        try {
            if (kernelName.includes("convolution")) {
                const signal: number[] = JSON.parse(args[0] || "[]");
                const kernel: number[] = JSON.parse(args[1] || "[]");
                const n = signal.length;
                const k = kernel.length;
                const half = Math.floor(k / 2);
                const out: number[] = new Array(n).fill(0);
                for (let idx = 0; idx < n; idx++) {
                    let sum = 0;
                    for (let i = -half; i <= half; i++) {
                        const s = idx + i;
                        if (s >= 0 && s < n) {
                            sum += signal[s] * kernel[i + half];
                        }
                    }
                    out[idx] = sum;
                }
                return JSON.stringify(out);
            } else if (kernelName.includes("spectral")) {
                const signal: number[] = JSON.parse(args[0] || "[]");
                const n = signal.length;
                const out: number[] = [];
                for (let idx = 0; idx < n; idx++) {
                    let real = 0, imag = 0;
                    for (let k = 0; k < n; k++) {
                        const angle = -2.0 * Math.PI * idx * k / n;
                        real += signal[k] * Math.cos(angle);
                        imag += signal[k] * Math.sin(angle);
                    }
                    out.push(Math.sqrt(real * real + imag * imag));
                }
                return JSON.stringify(out);
            } else if (kernelName.includes("fusion")) {
                const a: number[] = JSON.parse(args[0] || "[]");
                const b: number[] = JSON.parse(args[1] || "[]");
                const n = Math.min(a.length, b.length);
                const out: number[] = [];
                for (let idx = 0; idx < n; idx++) {
                    out.push(a[idx] * 0.6 + b[idx] * 0.4);
                }
                return JSON.stringify(out);
            }
        } catch {
            return "[]";
        }
        return "[]";
    }
}
