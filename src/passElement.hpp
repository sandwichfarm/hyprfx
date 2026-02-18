#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include "globals.hpp"

class CBMWDecoration;

class CBMWPassElement : public IPassElement {
  public:
    struct SBMWData {
        CBMWDecoration* deco     = nullptr;
        float           alpha    = 1.0f;
        PHLMONITORREF   pMonitor;
    };

    CBMWPassElement(const SBMWData& data);
    virtual ~CBMWPassElement() = default;

    virtual void                draw(const CRegion& damage);
    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();
    virtual const char*         passName() { return "CBMWPassElement"; }

  private:
    SBMWData m_data;

    void renderEffect(PHLMONITOR pMonitor, float alpha);
};
