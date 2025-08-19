#include <Shared/Config.hh>

extern const uint64_t VERSION_HASH = 4728567265382323ll;
extern const uint32_t SERVER_PORT = 2053;
extern const uint32_t MAX_NAME_LENGTH = 16;

//your ws host url may not follow this format, change it to fit your needs
extern std::string const WS_URL = "ws://localhost:"+std::to_string(SERVER_PORT);