#include <Shared/Config.hh>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif


extern const uint64_t VERSION_HASH = 4728567265382323ll;
extern const uint32_t SERVER_PORT = 2053;
extern const uint32_t MAX_NAME_LENGTH = 16;

std::string get_ws_url() {
#ifdef __EMSCRIPTEN__
    char* url = (char*)EM_ASM_PTR({
        let currentHost = window.location.hostname;
        let wsUrl;

        if (currentHost === 'localhost' || currentHost === '127.0.0.1') {
            wsUrl = 'ws://localhost:' + $0;
        } else {
            wsUrl = 'wss://us1.rexor.io:' + $0;
        }

        let len = lengthBytesUTF8(wsUrl) + 1;
        let ptr = _malloc(len);
        stringToUTF8(wsUrl, ptr, len);
        return ptr;
    }, SERVER_PORT);

    std::string result(url);
    free(url);
    return result;
#else
    return "ws://localhost:" + std::to_string(SERVER_PORT);
#endif
}