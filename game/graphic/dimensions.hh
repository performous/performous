#pragma once


/// class for geometry stuff
class Dimensions {
public:
	/** Initialize with aspect ratio but no size, centered at screen center. **/
	Dimensions(float ar_ = 0.0f);
	/** Initialize with top-left corner and width & height **/
	Dimensions(float x1, float y1, float w, float h);
	/// sets middle
	Dimensions& middle(float x = 0.0f);
	/// sets left
	Dimensions& left(float x = 0.0f);
	/// sets right
	Dimensions& right(float x = 0.0f);
	/// sets center
	Dimensions& center(float y = 0.0f);
	/// sets top
	Dimensions& top(float y = 0.0f);
	/// sets bottom
	Dimensions& bottom(float y = 0.0f);
	/// reset aspect ratio and switch to fixed width mode
	Dimensions& ar(float ar);
	/// fixes width
	Dimensions& fixedWidth(float w);
	/// fixes height
	Dimensions& fixedHeight(float h);
	/// fits inside
	Dimensions& fitInside(float w, float h);
	/// fits outside
	Dimensions& fitOutside(float w, float h);
	/// stretches dimensions
	Dimensions& stretch(float w, float h);
	/// sets screen center
	Dimensions& screenCenter(float y = 0.0f);
	/// sets screen top
	Dimensions& screenTop(float y = 0.0f);
	/// sets screen bottom
	Dimensions& screenBottom(float y = 0.0f);
	/// move the object without affecting anchoring
	Dimensions& move(float x, float y);
	Dimensions& scale(float f);
	Dimensions& scale(float horizontal, float vertical);
	/// returns ar XXX
	float ar() const;
	/// returns left
	float x1() const;
	/// returns top
	float y1() const;
	/// returns right
	float x2() const;
	/// returns bottom
	float y2() const;
	/// returns x center
	float xc() const;
	/// returns y center
	float yc() const;
	/// returns width
	float w() const;
	/// returns height
	float h() const;

private:
	float screenY() const;

	float m_ar;
	float m_x, m_y, m_w, m_h;
	float m_scaleHorizontal = 1.f;
	float m_scaleVertical = 1.f;
	enum class XAnchor { MIDDLE, LEFT, RIGHT } m_xAnchor;
	enum class YAnchor { CENTER, TOP, BOTTOM } m_yAnchor, m_screenAnchor;
};

