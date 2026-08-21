export interface PipelineConfig {
    name: string;
    sampleRate: number;
    nodes: Array<{
        id: string;
        type: string;
        options?: Record<string, any>;
    }>;
    connections: Array<{
        from: string;
        to: string;
    }>;
}
