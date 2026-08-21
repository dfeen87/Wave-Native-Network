#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cuda_runtime.h>
#include <cuComplex.h>

extern "C" __global__
void spectralTransform(const float* input, cuComplex* output, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    float real = 0.0f;
    float imag = 0.0f;

    for (int k = 0; k < n; k++) {
        float angle = -2.0f * 3.14159265358979f * idx * k / n;
        real += input[k] * cosf(angle);
        imag += input[k] * sinf(angle);
    }

    output[idx] = make_cuComplex(real, imag);
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
    if (argc < 2) {
        std::cerr << "Usage: spectral_transform <input_json_array>\n";
        return 1;
    }

    std::vector<float> h_input = parseJsonArray(argv[1]);
    int n = static_cast<int>(h_input.size());

    if (n == 0) {
        std::cout << "[]\n";
        return 0;
    }

    std::vector<cuComplex> h_output(n);

    float *d_input = nullptr;
    cuComplex *d_output = nullptr;

    cudaMalloc((void**)&d_input, n * sizeof(float));
    cudaMalloc((void**)&d_output, n * sizeof(cuComplex));

    cudaMemcpy(d_input, h_input.data(), n * sizeof(float), cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (n + blockSize - 1) / blockSize;

    spectralTransform<<<gridSize, blockSize>>>(d_input, d_output, n);
    cudaDeviceSynchronize();

    cudaMemcpy(h_output.data(), d_output, n * sizeof(cuComplex), cudaMemcpyDeviceToHost);

    cudaFree(d_input);
    cudaFree(d_output);

    std::cout << "[";
    for (int i = 0; i < n; i++) {
        float real = cuCrealf(h_output[i]);
        float imag = cuCimagf(h_output[i]);
        float mag = sqrtf(real * real + imag * imag);
        std::cout << mag << (i < n - 1 ? "," : "");
    }
    std::cout << "]\n";

    return 0;
}
