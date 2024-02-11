#include "change_notifier.hh"
#include "change_notifier.hh"
#include <iostream>

ChangeNotifier::~ChangeNotifier() {
    while (!m_receivers.empty()) {
        auto receiver = *m_receivers.begin();

        receiver->disconnect();
    }
}

void ChangeNotifier::addChangeReceiver(ChangeReceiver* receiver) {
    if (receiver && m_receivers.find(receiver) == m_receivers.end()) {
        m_receivers.insert(receiver);
        receiver->connectTo(this);
    }
}

void ChangeNotifier::removeChangeReceiver(ChangeReceiver* receiver) {
    if (receiver && m_receivers.find(receiver) != m_receivers.end()) {
        m_receivers.erase(receiver);
        receiver->disconnect();
    }
}

void ChangeNotifier::notifyChangeReceivers() {
    for (auto&& receiver : m_receivers) {
        receiver->onChange(*this);
    }
}

ChangeReceiver::ChangeReceiver(ChangeNotifier* notifier) {
    connectTo(notifier);
}

ChangeReceiver::ChangeReceiver(const ChangeReceiver& other) {
    connectTo(other.m_changeNofifier);
}

ChangeReceiver::ChangeReceiver(ChangeReceiver&& other) {
    connectTo(other.m_changeNofifier);
    other.disconnect();
}

ChangeReceiver::~ChangeReceiver() {
    disconnect();
}

ChangeReceiver& ChangeReceiver::operator=(const ChangeReceiver& other) {
    connectTo(other.m_changeNofifier);

    return *this;
}

ChangeReceiver& ChangeReceiver::operator=(ChangeReceiver&& other) {
    connectTo(other.m_changeNofifier);
    other.disconnect();

    return *this;
}

void ChangeReceiver::connectTo(ChangeNotifier* notifier) {
    if (notifier && m_changeNofifier != notifier) {
        disconnect();

        m_changeNofifier = notifier;

        if (m_changeNofifier)
            m_changeNofifier->addChangeReceiver(this);
    }
}

void ChangeReceiver::disconnect() {
    if (m_changeNofifier) {
        auto notifier = m_changeNofifier;

        m_changeNofifier = nullptr;
        notifier->removeChangeReceiver(this);
    }
}
