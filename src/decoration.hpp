#pragma once

#define WLR_USE_UNSTABLE

#include <chrono>
#include <string>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include "globals.hpp"

class CHFXDecoration : public IHyprWindowDecoration {
  public:
    CHFXDecoration(PHLWINDOW pWindow, bool isClosing);
    virtual ~CHFXDecoration();

    virtual SDecorationPositioningInfo getPositioningInfo();
    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply);
    virtual void                       draw(PHLMONITOR, float const& a);
    virtual eDecorationType            getDecorationType();
    virtual void                       updateWindow(PHLWINDOW);
    virtual void                       damageEntire();
    virtual eDecorationLayer           getDecorationLayer();
    virtual std::string                getDisplayName();

    float getProgress() const;

  private:
    PHLWINDOWREF m_pWindow;
    bool         m_bIsClosing   = false;
    std::string  m_effectName;
    float        m_fDuration    = 1.0f;

    std::chrono::steady_clock::time_point m_startTime;
    bool m_bDone = false;

    SP<HOOK_CALLBACK_FN> m_pTickCb;

    friend class CHFXPassElement;
};
