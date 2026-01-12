#pragma once

struct ILayoutable {
	virtual ~ILayoutable() = default;

	virtual void layout() = 0;
};
