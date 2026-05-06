with open('mesh_legacy/mesh_discovery.cpp', 'r') as f:
    content = f.read()

old_analyze = """    void AdaptiveSpectralMonitor::analyze_stream(const std::vector<double>& stream) {
        if (stream.size() < 3) {
            latest_snr_.store(0.0);
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = std::nullopt;
            return;
        }

        // Calculate Signal Power (Variance) and Mean
        double mean = 0.0;
        for (double val : stream) {
            mean += val;
        }
        mean /= stream.size();"""

new_analyze = """    void AdaptiveSpectralMonitor::analyze_stream(const std::vector<double>& stream) {
        if (stream.size() < 3) {
            latest_snr_.store(0.0);
            std::lock_guard<std::mutex> lock(cand_mutex_);
            candidate_frequency_ = std::nullopt;
            return;
        }

        // Apply Hann Window to reduce sidelobe spectral leakage
        std::vector<double> windowed_stream(stream.size());
        size_t N = stream.size();
        for (size_t i = 0; i < N; ++i) {
            double hann_multiplier = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
            windowed_stream[i] = stream[i] * hann_multiplier;
        }

        // Calculate Signal Power (Variance) and Mean from windowed stream
        double mean = 0.0;
        for (double val : windowed_stream) {
            mean += val;
        }
        mean /= windowed_stream.size();"""

content = content.replace(old_analyze, new_analyze)

content = content.replace('for (double val : stream) {\n            variance += (val - mean) * (val - mean);\n        }\n        variance /= stream.size();', 'for (double val : windowed_stream) {\n            variance += (val - mean) * (val - mean);\n        }\n        variance /= windowed_stream.size();')
content = content.replace('for (size_t i = 1; i < stream.size(); ++i) {\n            double diff = stream[i] - stream[i - 1];', 'for (size_t i = 1; i < windowed_stream.size(); ++i) {\n            double diff = windowed_stream[i] - windowed_stream[i - 1];')
content = content.replace('noise_variance /= (2.0 * (stream.size() - 1));', 'noise_variance /= (2.0 * (windowed_stream.size() - 1));')
content = content.replace('for (size_t i = 1; i < stream.size() - 1; ++i) {\n            double x = stream[i];\n            double v = (stream[i + 1] - stream[i - 1]) / (2.0 * dt);\n            double a = (stream[i + 1] - 2.0 * stream[i] + stream[i - 1]) / (dt * dt);', 'for (size_t i = 1; i < windowed_stream.size() - 1; ++i) {\n            double x = windowed_stream[i];\n            double v = (windowed_stream[i + 1] - windowed_stream[i - 1]) / (2.0 * dt);\n            double a = (windowed_stream[i + 1] - 2.0 * windowed_stream[i] + windowed_stream[i - 1]) / (dt * dt);')
content = content.replace('if (valid_points > stream.size() / 2) {', 'if (valid_points > windowed_stream.size() / 2) {')
content = content.replace('for (size_t i = 1; i < stream.size(); ++i) {\n                if ((stream[i-1] < mean && stream[i] >= mean) ||\n                    (stream[i-1] > mean && stream[i] <= mean)) {', 'for (size_t i = 1; i < windowed_stream.size(); ++i) {\n                if ((windowed_stream[i-1] < mean && windowed_stream[i] >= mean) ||\n                    (windowed_stream[i-1] > mean && windowed_stream[i] <= mean)) {')
content = content.replace('double estimated_omega = (crossings * M_PI) / (stream.size() * dt);', 'double estimated_omega = (crossings * M_PI) / (windowed_stream.size() * dt);')

with open('mesh_legacy/mesh_discovery.cpp', 'w') as f:
    f.write(content)
