#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cuda_runtime.h>

extern "C" __global__
void multimodalFusion(const float* a, const float* b, float* out, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    float w1 = 0.6f;
    float w2 = 0.4f;

    out[idx] = (a[idx] * w1) + (b[idx] * w2);
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
        std::cerr << "Usage: multimodal_fusion <array_a> <array_b>\n";
        return 1;
    }

    std::vector<float> h_a = parseJsonArray(argv[1]);
    std::vector<float> h_b = parseJsonArray(argv[2]);

    int n = static_cast<int>(std::min(h_a.size(), h_b.size()));

    if (n == 0) {
        std::cout << "[]\n";
        return 0;
    }

    std::vector<float> h_out(n, 0.0f);

    float *d_a = nullptr, *d_b = nullptr, *d_out = nullptr;
    cudaMalloc((void**)&d_a, n * sizeof(float));
    cudaMalloc((void**)&d_b, n * sizeof(float));
    cudaMalloc((void**)&d_out, n * sizeof(float));

    cudaMemcpy(d_a, h_a.data(), n * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b.data(), n * sizeof(float), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (n + blockSize - 1) / blockSize;

    multimodalFusion<<<gridSize, blockSize>>>(d_a, d_b, d_out, n);
    cudaDeviceSynchronize();

    cudaMemcpy(h_out.data(), d_out, n * sizeof(float), cudaMemcpyDeviceToHost);

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_out);

    std::cout << "[";
    for (int i = 0; i < n; i++) {
        std::cout << h_out[i] << (i < n - 1 ? "," : "");
    }
    std::cout << "]\n";

    return 0;
}
