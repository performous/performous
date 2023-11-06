#pragma once

#include <any>
#include <functional>
#include <map>
#include <string>
#include <vector>

class EventParameter {
public:
	EventParameter() = default;
	EventParameter(std::map<std::string, std::any> const&);

	EventParameter& add(std::string const& name, std::any const& value);

	bool hasParameter(std::string const&) const;
	std::any const& get(std::string const&) const;
	std::any const& get(std::string const&, std::any const& fallback) const;

	template<class ValueType>
	ValueType get(std::string const& name) const {
		auto value = get(name);

		return std::any_cast<ValueType>(value);
	}

	template<class ValueType>
	ValueType get(std::string const& name, ValueType const& fallback) const {
		if (!hasParameter(name))
			return fallback;

		auto value = get(name);

		return std::any_cast<ValueType>(value);
	}

private:
	std::map<std::string, std::any> m_parameters;
};

class EventManager {
public:
	using EventReceiver = std::function<void(EventParameter const&)>;

public:
	void addReceiver(std::string const& eventName, EventReceiver);
	void sendEvent(std::string const& eventName, EventParameter const& = {});

private:
	std::map<std::string, std::vector<EventReceiver>> m_eventReceiver;
};
