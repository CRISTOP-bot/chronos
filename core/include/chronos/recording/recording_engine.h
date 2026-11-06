#pragma once
#include "chronos/recording/session.h"
#include "chronos/common/result.h"
#include <memory>
#include <map>
#include <string>

namespace chronos {
namespace recording {

class RecordingEngine {
public:
    // Create a new session; returns session id.
    std::shared_ptr<Session> create_session(SessionConfig cfg);
    std::shared_ptr<Session> get_session(const std::string& name);
    std::vector<std::string> list_sessions() const;

private:
    std::map<std::string, std::shared_ptr<Session>> sessions_;
};

} // namespace recording
} // namespace chronos
