#include "app/app.h"

namespace sibalhook {

App& App::Instance() {
    static App inst;
    return inst;
}

void App::RequestUnload() {
    unloading_.store(true);
}

} // namespace sibalhook
