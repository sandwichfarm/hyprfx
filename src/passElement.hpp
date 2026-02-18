#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include "globals.hpp"

class CHFXDecoration;

class CHFXPassElement : public IPassElement {
  public:
    struct SHFXData {
        CHFXDecoration*   deco        = nullptr;
        SClosingAnimation* closingAnim = nullptr;
        float              alpha       = 1.0f;
        PHLMONITORREF      pMonitor;
    };

    CHFXPassElement(const SHFXData& data);
    virtual ~CHFXPassElement() = default;

    virtual void                draw(const CRegion& damage);
    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();
    virtual const char*         passName() { return "CHFXPassElement"; }

  private:
    SHFXData m_data;

    void renderEffect(PHLMONITOR pMonitor, float alpha);
};
