#include "chronos/recording/recording_engine.h"

namespace chronos {
namespace recording {

std::shared_ptr<Session> RecordingEngine::create_session(SessionConfig cfg) {
    std::string name = cfg.name;
    storage::RecordingMetadata meta;
    meta.name = name;
    meta.target_path = cfg.target_path;
    auto s = std::make_shared<Session>(std::move(cfg), std::move(meta));
    sessions_[name] = s;
    return s;
}

std::shared_ptr<Session> RecordingEngine::get_session(const std::string& name) {
    auto it = sessions_.find(name);
    if (it == sessions_.end()) return nullptr;
    return it->second;
}

std::vector<std::string> RecordingEngine::list_sessions() const {
    std::vector<std::string> v;
    for (const auto& kv : sessions_) v.push_back(kv.first);
    return v;
}

} // namespace recording
} // namespace chronos
