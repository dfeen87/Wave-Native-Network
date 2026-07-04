#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include <optional>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <mutex>

#include "zk_verifier.hpp"

namespace wave_native {
namespace mesh_legacy {

    enum class InterfaceType {
        Ethernet,
        WiFi,
        LteModem,
        UsbTether,
        BluetoothPan,
        Unknown
    };

    struct InterfaceInfo {
        std::string name;
        InterfaceType iface_type;
        bool is_up;
        bool has_carrier;

        bool is_wan_candidate() const {
            return is_up && has_carrier && iface_type != InterfaceType::Unknown;
        }
    };

    class InterfaceRegistry {
    public:
        InterfaceRegistry() = default;

        void register_interface(const InterfaceInfo& info);
        void unregister_interface(const std::string& name);

        std::vector<InterfaceInfo> get_wan_candidates() const;
        std::optional<InterfaceInfo> get_interface(const std::string& name) const;
        std::vector<InterfaceInfo> get_all_interfaces() const;

    private:
        mutable std::shared_mutex mutex_;
        std::map<std::string, InterfaceInfo> interfaces_;
    };

    struct ResonantPeer {
        std::vector<uint8_t> signature;
        long double omega;
        double psi_snr;
        AileeTrustScore ailee_score;
        long double t_s;
        std::atomic<double> latency_ema{0.0};
        std::atomic<double> jitter_ema{0.0};
        long double phase_theta = 0.0L;

        ResonantPeer() = default;

        ResonantPeer(const std::vector<uint8_t>& sig, long double w, double snr, AileeTrustScore score, long double ts, long double theta)
            : signature(sig), omega(w), psi_snr(snr), ailee_score(score), t_s(ts), phase_theta(theta) {}

        ResonantPeer(const ResonantPeer& other)
            : signature(other.signature),
              omega(other.omega),
              psi_snr(other.psi_snr),
              ailee_score(other.ailee_score),
              t_s(other.t_s),
              latency_ema(other.latency_ema.load()),
              jitter_ema(other.jitter_ema.load()),
              phase_theta(other.phase_theta) {}

        ResonantPeer& operator=(const ResonantPeer& other) {
            if (this != &other) {
                signature = other.signature;
                omega = other.omega;
                psi_snr = other.psi_snr;
                ailee_score = other.ailee_score;
                t_s = other.t_s;
                latency_ema.store(other.latency_ema.load());
                jitter_ema.store(other.jitter_ema.load());
                phase_theta = other.phase_theta;
            }
            return *this;
        }
    };

    class ResonantPeerTable {
    public:
        void insert_or_update(const ResonantPeer& peer);
        std::optional<ResonantPeer> get_peer(const std::vector<uint8_t>& signature) const;
        void prune_decayed_peers(double current_time);

        size_t get_peer_count() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return peers_.size();
        }

        std::vector<ResonantPeer> get_all_peers() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<ResonantPeer> all_peers;
            all_peers.reserve(peers_.size());
            for (const auto& [sig, peer] : peers_) {
                all_peers.push_back(peer);
            }
            return all_peers;
        }

    private:
        mutable std::shared_mutex mutex_;
        std::map<std::vector<uint8_t>, ResonantPeer> peers_;
    };

    class AdaptiveSpectralMonitor {
    public:
        AdaptiveSpectralMonitor() : latest_snr_(0.0) {}

        void analyze_stream(const std::vector<double>& stream);
        double get_latest_snr() const { return latest_snr_.load(); }
        std::optional<double> get_candidate_frequency() const {
            std::lock_guard<std::mutex> lock(cand_mutex_);
            return candidate_frequency_;
        }

    private:
        std::atomic<double> latest_snr_;
        mutable std::mutex cand_mutex_;
        std::optional<double> candidate_frequency_;
    };

    class PeerGatekeeper {
    public:
        explicit PeerGatekeeper(std::shared_ptr<class ZKVerifier> verifier);

        // Immediately parses the connection, verifies the proof and the trust score.
        // Drops connection ruthlessly if verification fails. Returns true only on full success.
        bool process_incoming_peer(const std::vector<uint8_t>& phase_signature,
                                   const struct ZKProof& proof,
                                   struct AileeTrustScore& score,
                                   double psi_snr);

    private:
        std::shared_ptr<class ZKVerifier> verifier_;
        std::map<std::vector<uint8_t>, std::vector<double>> drift_history_;
        std::map<std::vector<uint8_t>, bool> trust_state_;
        std::map<std::vector<uint8_t>, double> ema_consistency_;
    };

    class InterfaceDiscovery {
    public:
        explicit InterfaceDiscovery(std::shared_ptr<InterfaceRegistry> registry, std::shared_ptr<PeerGatekeeper> gatekeeper);

        void start_monitoring();
        bool discover_interfaces();

    private:
        std::shared_ptr<InterfaceRegistry> registry_;
        std::shared_ptr<PeerGatekeeper> gatekeeper_;

        // Mock method for discovering interfaces.
        bool discover_linux_interfaces();
        bool use_mock_interfaces();
    };

} // namespace mesh_legacy
} // namespace wave_native
