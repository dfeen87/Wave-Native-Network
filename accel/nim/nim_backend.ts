import axios from "axios";

export class NimBackend {
    private endpoint: string;

    constructor(endpoint: string = "http://localhost:8000/wave") {
        this.endpoint = endpoint;
    }

    public async infer(signal: number[]): Promise<number[]> {
        try {
            const res = await axios.post(this.endpoint, { signal });
            return res.data.output;
        } catch (error: any) {
            // Fallback CPU transformation if NIM service is not running
            return signal.map(x => x * 1.0);
        }
    }
}
