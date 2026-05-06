import re

with open('src/calibration.hpp', 'r') as f:
    content = f.read()

content = content.replace('bool run_sweep(double& out_omega);', 'bool run_sweep(long double& out_omega);')
content = content.replace('static void save_profile(const std::string& path, double omega, double alpha, double beta, double delta);', 'static void save_profile(const std::string& path, long double omega, long double alpha, long double beta, long double delta);')
content = content.replace('static bool load_profile(const std::string& path, double& omega, double& alpha, double& beta, double& delta);', 'static bool load_profile(const std::string& path, long double& omega, long double& alpha, long double& beta, long double& delta);')

with open('src/calibration.hpp', 'w') as f:
    f.write(content)

with open('src/calibration.cpp', 'r') as f:
    content = f.read()

content = content.replace('bool CalibrationMode::run_sweep(double& out_omega)', 'bool CalibrationMode::run_sweep(long double& out_omega)')
content = content.replace('void CalibrationMode::save_profile(const std::string& path, double omega, double alpha, double beta, double delta)', 'void CalibrationMode::save_profile(const std::string& path, long double omega, long double alpha, long double beta, long double delta)')
content = content.replace('bool CalibrationMode::load_profile(const std::string& path, double& omega, double& alpha, double& beta, double& delta)', 'bool CalibrationMode::load_profile(const std::string& path, long double& omega, long double& alpha, long double& beta, long double& delta)')

with open('src/calibration.cpp', 'w') as f:
    f.write(content)

with open('mesh_legacy/mesh_discovery.hpp', 'r') as f:
    content = f.read()

content = content.replace('double omega;', 'long double omega;')
content = content.replace('double t_s;', 'long double t_s;')

with open('mesh_legacy/mesh_discovery.hpp', 'w') as f:
    f.write(content)
