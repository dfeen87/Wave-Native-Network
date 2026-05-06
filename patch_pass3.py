with open('mesh_legacy/mesh_discovery.hpp', 'r') as f:
    content = f.read()

content = content.replace('std::map<std::vector<uint8_t>, std::vector<double>> drift_history_;', 'std::map<std::vector<uint8_t>, std::vector<double>> drift_history_;\n        std::map<std::vector<uint8_t>, bool> trust_state_;\n        std::map<std::vector<uint8_t>, double> ema_consistency_;')

with open('mesh_legacy/mesh_discovery.hpp', 'w') as f:
    f.write(content)

with open('mesh_legacy/mesh_discovery.cpp', 'r') as f:
    content = f.read()

old_consistency = """        if (history.size() >= 2) {
            double mean = 0.0;
            for (double d : history) mean += d;
            mean /= history.size();

            double variance = 0.0;
            for (double d : history) variance += (d - mean) * (d - mean);
            variance /= (history.size() - 1);

            double stddev = std::sqrt(variance);
            score.consistency_score = std::max(0.0, 1.0 - stddev);
        } else {
            score.consistency_score = 1.0;
        }"""

new_consistency = """        if (history.size() >= 2) {
            double mean = 0.0;
            for (double d : history) mean += d;
            mean /= history.size();

            double variance = 0.0;
            for (double d : history) variance += (d - mean) * (d - mean);
            variance /= (history.size() - 1);

            double stddev = std::sqrt(variance);
            double raw_consistency = std::max(0.0, 1.0 - stddev);

            // EMA for consistency smoothing
            if (ema_consistency_.find(phase_signature) == ema_consistency_.end()) {
                ema_consistency_[phase_signature] = raw_consistency;
            } else {
                ema_consistency_[phase_signature] = 0.2 * raw_consistency + 0.8 * ema_consistency_[phase_signature];
            }
            score.consistency_score = ema_consistency_[phase_signature];
        } else {
            score.consistency_score = 1.0;
        }"""

content = content.replace(old_consistency, new_consistency)

old_dropout = """        if (score.aggregate_score() < 0.70) {
            drift_history_.erase(phase_signature);
            return false; // Phase Decoloration
        }

        return true;"""

new_dropout = """        double agg_score = score.aggregate_score();
        bool is_trusted = trust_state_[phase_signature];

        // Schmitt Trigger Logic for hysteresis
        if (is_trusted) {
            if (agg_score < 0.65) {
                drift_history_.erase(phase_signature);
                trust_state_[phase_signature] = false;
                return false; // Drop-out
            }
        } else {
            if (agg_score >= 0.75) {
                trust_state_[phase_signature] = true;
            } else {
                return false; // Not locked in yet
            }
        }

        return true;"""

content = content.replace(old_dropout, new_dropout)

with open('mesh_legacy/mesh_discovery.cpp', 'w') as f:
    f.write(content)
