#include "event_manager.hh"
#include <iostream>

EventParameter::EventParameter(std::map<std::string, std::any> const& parameters)
:m_parameters(parameters) {
}

EventParameter& EventParameter::add(std::string const& name, std::any const& value) {
	m_parameters[name] = value;

	return *this;
}

bool EventParameter::hasParameter(std::string const& name) const {
	return m_parameters.find(name) != m_parameters.end();
}

std::any const& EventParameter::get(std::string const& name) const {
	auto it = m_parameters.find(name);

	if (it != m_parameters.end())
		return it->second;

	throw std::runtime_error("Parameter value with name '" + name + "' not found!");
}

std::any const& EventParameter::get(std::string const& name, std::any const& fallback) const {
	auto it = m_parameters.find(name);

	if (it != m_parameters.end())
		return it->second;

	return fallback;
}


void EventManager::addReceiver(std::string const& eventName, EventReceiver receiver) {
	m_eventReceiver[eventName].emplace_back(receiver);
}

void EventManager::sendEvent(std::string const& eventName, EventParameter const& parameter) {
	auto it = m_eventReceiver.find(eventName);

	if (it == m_eventReceiver.end())
		return;

	for (auto const& receiver : it->second)
		receiver(parameter);
}
