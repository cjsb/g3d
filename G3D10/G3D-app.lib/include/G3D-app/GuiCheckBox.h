/**
  \file G3D-app.lib/include/G3D-app/GuiCheckBox.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef G3D_GuiCheckBox_h
#define G3D_GuiCheckBox_h

#include "G3D-app/GuiControl.h"
#include "G3D-base/Pointer.h"
#include "G3D-app/GuiTheme.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** Check box or toggle button */
class GuiCheckBox : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
public:

    virtual void setRect(const Rect2D& rect) override;

protected:

    Pointer<bool>            m_value;
    GuiTheme::CheckBoxStyle  m_style;

public:    
    /** Called by GuiContainer.  See GuiPane::addCheckBox */
    GuiCheckBox(GuiContainer* parent, const GuiText& text, const Pointer<bool>& value, GuiTheme::CheckBoxStyle style = GuiTheme::NORMAL_CHECK_BOX_STYLE);

    /** Called by GuiContainer.*/
    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& skin, bool ancestorsEnabled) const override;

    /** Called by GuiContainer. Delivers events when this control is clicked on and when it has the key focus. */
    virtual bool onEvent(const GEvent& event) override;

    /** True if this is a tool button */
    virtual bool toolStyle() const override { 
        return m_style == GuiTheme::TOOL_CHECK_BOX_STYLE;
    }
};

} // G3D

#endif
