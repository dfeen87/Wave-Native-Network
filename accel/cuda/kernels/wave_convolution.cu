#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cuda_runtime.h>

extern "C" __global__
void waveConvolution(const float* signal, const float* kernel, float* output, int n, int k) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    float sum = 0.0f;
    int half = k / 2;

    for (int i = -half; i <= half; i++) {
        int s = idx + i;
        if (s >= 0 && s < n) {
            sum += signal[s] * kernel[i + half];
        }
    }
    output[idx] = sum;
}

// Host CLI harness for standalone execution
static std::vector<float> parseJsonArray(const std::string& str) {
    std::vector<float> result;
    std::string s = str;
    for (char& c : s) {
        if (c == '[' || c == ']' || c == ',') c = ' ';
    }
    std::stringstream ss(s);
    float val;
    while (ss >> val) {
        result.push_back(val);
    }
    return result;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: wave_convolution <signal_json_array> <kernel_json_array>\n";
        return 1;
    }

    std::vector<float> h_signal = parseJsonArray(argv[1]);
    std::vector<float> h_kernel = parseJsonArray(argv[2]);

    int n = static_cast<int>(h_signal.size());
    int k = static_cast<int>(h_kernel.size());

    if (n == 0 || k == 0) {
        std::cout << "[]\n";
        return 0;
    }

    std::vector<float> h_output(n, 0.0f);

    float *d_signal = nullptr, *d_kernel = nullptr, *d_output = nullptr;
    cudaMalloc((void**)&d_signal, n * sizeof(float));
    cudaMalloc((void**)&d_kernel, k * sizeof(float));
    cudaMalloc((void**)&d_output, n * sizeof(float));

    cudaMemcpy(d_signal, h_signal.data(), n * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_kernel, h_kernel.data(), k * sizeof(float), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (n + blockSize - 1) / blockSize;

    waveConvolution<<<gridSize, blockSize>>>(d_signal, d_kernel, d_output, n, k);
    cudaDeviceSynchronize();

    cudaMemcpy(h_output.data(), d_output, n * sizeof(float), cudaMemcpyDeviceToHost);

    cudaFree(d_signal);
    cudaFree(d_kernel);
    cudaFree(d_output);

    std::cout << "[";
    for (int i = 0; i < n; i++) {
        std::cout << h_output[i] << (i < n - 1 ? "," : "");
    }
    std::cout << "]\n";

    return 0;
}
