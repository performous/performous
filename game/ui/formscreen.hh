#pragma once

#include "screen.hh"
#include "ui/form.hh"
#include "ui/graphiccontext.hh"
#include "ui/effect/effectmanager.hh"

class FormScreen: public Screen {
  public:
	FormScreen(std::string const& name);
	~FormScreen() override = default;

	void manageEvent(input::NavEvent const& event) override;
	void manageEvent(SDL_Event) override;
	void draw() override;

  protected:
	Form& getForm() { return m_control; }
	GraphicContext& getGraphicContext() { return m_gc; }

	virtual void onCancel() {}

  private:
	EffectManager m_effectManager;
	GraphicContext m_gc;
	Form m_control;
};

