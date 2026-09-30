#include "framework.h"
#include "config/persist.h"
#include "config/config.h"
#include "render/menu_state.h"

namespace sibalhook {

void PersistFlushNow() {
    MenuState::Instance().SaveToConfig();
    Config::Instance().Save();
}

} // namespace sibalhook
