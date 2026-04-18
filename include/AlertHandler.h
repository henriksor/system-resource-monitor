#pragma once

#include <memory>
#include <string>

#include "Alert.h"

class AlertHandler {
public:
    virtual ~AlertHandler() = default;
    virtual void handle(const Alert& alert) = 0;
};

class WebhookAlertHandler : public AlertHandler {
public:
    explicit WebhookAlertHandler(std::string webhookUrl);

    void handle(const Alert& alert) override;

private:
    std::string buildRequestBody(const Alert& alert) const;

    std::string webhookUrl;
};

class EmailAlertHandler : public AlertHandler {
public:
    explicit EmailAlertHandler(std::string recipient);

    void handle(const Alert& alert) override;

private:
    std::string recipient;
};
