export type Modality = "audio" | "video" | "imu" | "rf" | "generic";

export interface ModalityDescriptor {
    name: string;
    type: Modality;
    sampleRate: number;
    channels: number;
}
