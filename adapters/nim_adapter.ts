import axios from "axios";
import { NimBackend } from "../accel/nim/nim_backend";

export class NimAdapter {
    constructor(private backend = new NimBackend()) {}

    public async infer(signal: number[]): Promise<number[]> {
        return await this.backend.infer(signal);
    }
}
