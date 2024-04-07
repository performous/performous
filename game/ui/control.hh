#pragma once

#include <string>
#include <functional>

#include "border.hh"
#include "ui/effect/ieffect.hh"

class GraphicContext;

class Control {
  public:
	Control() = default;
	Control(Control* parent);
	virtual ~Control() = default;

	Control* getParent();
	Control const* getParent() const;
	void setParent(Control*);

	void setGeometry(float x, float y, float width, float height);
	float getX() const;
	float getY() const;
	float getWidth() const;
	float getHeight() const;

	void setName(std::string const& name);
	std::string getName() const;

	void enable();
	void disable();
	void setEnabled(bool enabled);
	bool isEnabled() const;

	virtual bool canFocused() const { return true; }
	virtual void setFocus(bool focused) { m_focused = focused; }
	bool hasFocus() const { return m_focused; }

	void setTabIndex(unsigned);
	unsigned getTabIndex() const;

	enum class Key { Up, Down, Left, Right, PageUp, PageDown, Space, Tab, BackTab, Escape, Return, Delete, BackSpace,
		Number0, Number1, Number2, Number3, Number4, Number5, Number6, Number7, Number8, Number9,
		a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
	};

	virtual void onKey(Key) {}
	virtual void onKeyDown(Key) {}
	virtual void onKeyUp(Key) {}

	void onKeyDown(std::function<void(Control&, Key)> const&);
	void onKeyUp(std::function<void(Control&, Key)> const&);

	virtual void draw(GraphicContext&) = 0;

	virtual void initialize(Game&);
	void setInitialized();
	bool isInitialied() const;

  protected:
	friend class Form;
	void sendOnKeyDown(Key);
	void sendOnKeyUp(Key);

	void drawFocus(GraphicContext&);

	virtual Game& getGame();

  protected:
	Control* m_parent = nullptr;

  private:
	std::string m_name;
	float m_x;
	float m_y;
	float m_width;
	float m_height;
	bool m_focused = false;
	bool m_enabled = true;
	unsigned m_tabIndex = 0;
	Border m_focus{ nullptr };
	Texture m_focusEffectImage;
	EffectPtr m_focusEffect;
	glmath::mat3 m_matrix{1.f};
	std::function<void(Control&, Key)> m_onKeyDown;
	std::function<void(Control&, Key)> m_onKeyUp;
	bool m_initialized = false;
};
