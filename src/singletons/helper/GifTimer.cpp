#include "singletons/helper/GifTimer.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino {

void GIFTimer::initialize()
{
    this->timer.setInterval(GIF_FRAME_LENGTH);
    this->timer.setTimerType(Qt::PreciseTimer);

    getSettings()->animateEmotes.connect([this](bool enabled, auto) {
        if (enabled)
            this->timer.start();
        else
            this->timer.stop();
    });

    QObject::connect(&this->timer, &QTimer::timeout, [this] {
        if (getSettings()->animationsWhenFocused &&
            qApp->activeWindow() == nullptr)
            return;

        this->position_ += GIF_FRAME_LENGTH;
        this->signal.invoke();
        getApp()->windows->repaintGifEmotes();
    });
}

}  // namespace chatterino
