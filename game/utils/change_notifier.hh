#pragma once

#include <set>

class ChangeReceiver;

class ChangeNotifier {
public:
    ~ChangeNotifier();

    void addChangeReceiver(ChangeReceiver* receiver);
    void removeChangeReceiver(ChangeReceiver* receiver);
    void notifyChangeReceivers();

private:
    std::set<ChangeReceiver*> m_receivers;
};

class ChangeReceiver {
public:
    ChangeReceiver() = default;
    ChangeReceiver(ChangeNotifier* notifier);
    ChangeReceiver(const ChangeReceiver&);
    ChangeReceiver(ChangeReceiver&&);
    virtual ~ChangeReceiver();

    ChangeReceiver& operator=(const ChangeReceiver&);
    ChangeReceiver& operator=(ChangeReceiver&&);

    void connectTo(ChangeNotifier* notifier);

    void disconnect();

    virtual void onChange(ChangeNotifier&) = 0;

private:
    ChangeNotifier* m_changeNofifier = nullptr;
};