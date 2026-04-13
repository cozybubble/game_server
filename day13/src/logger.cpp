#include "logger.h"
LogStream::~LogStream() {
    event_.message = ss_.str();
    Logger::instance().log(event_);
}
