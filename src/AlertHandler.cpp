#include "AlertHandler.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <windows.h>
#include <winhttp.h>

#include "SnapshotJson.h"

#pragma comment(lib, "winhttp.lib")

namespace {
std::string escapeJsonString(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value)
    {
        switch (ch)
        {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }

    return escaped;
}

std::wstring utf8ToWide(const std::string& value)
{
    if (value.empty())
    {
        return {};
    }

    int requiredSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );

    if (requiredSize <= 0)
    {
        return {};
    }

    std::wstring wideValue(requiredSize, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        &wideValue[0],
        requiredSize
    );

    return wideValue;
}

class WinHttpHandle {
public:
    explicit WinHttpHandle(HINTERNET handle = nullptr)
        : handle(handle)
    {
    }

    ~WinHttpHandle()
    {
        if (handle != nullptr)
        {
            WinHttpCloseHandle(handle);
        }
    }

    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;

    WinHttpHandle(WinHttpHandle&& other) noexcept
        : handle(other.handle)
    {
        other.handle = nullptr;
    }

    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept
    {
        if (this != &other)
        {
            if (handle != nullptr)
            {
                WinHttpCloseHandle(handle);
            }

            handle = other.handle;
            other.handle = nullptr;
        }

        return *this;
    }

    HINTERNET get() const
    {
        return handle;
    }

    explicit operator bool() const
    {
        return handle != nullptr;
    }

private:
    HINTERNET handle = nullptr;
};

void validateWebhookUrl(const std::string& webhookUrl)
{
    const std::wstring wideUrl = utf8ToWide(webhookUrl);
    if (wideUrl.empty())
    {
        throw std::runtime_error("Webhook URL is not valid UTF-8.");
    }

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(
            wideUrl.c_str(),
            static_cast<DWORD>(wideUrl.size()),
            0,
            &components
        ))
    {
        throw std::runtime_error("Webhook URL could not be parsed.");
    }

    if (components.nScheme != INTERNET_SCHEME_HTTPS)
    {
        throw std::runtime_error("Webhook URL must use HTTPS.");
    }

    if (components.dwHostNameLength == 0)
    {
        throw std::runtime_error("Webhook URL must include a host.");
    }
}
} // namespace

WebhookAlertHandler::WebhookAlertHandler(std::string webhookUrl)
    : webhookUrl(std::move(webhookUrl))
{
    validateWebhookUrl(this->webhookUrl);
}

std::string WebhookAlertHandler::buildRequestBody(const Alert& alert) const
{
    std::ostringstream stream;
    stream << "{"
           << "\"metric\":\"" << escapeJsonString(alert.metric) << "\","
           << "\"type\":\"" << alertTypeToString(alert.type) << "\","
           << "\"severity\":\"" << severityToString(alert.severity) << "\","
           << "\"value\":" << alert.value << ","
           << "\"reference\":" << alert.reference << ","
           << "\"message\":\"" << escapeJsonString(alert.message) << "\""
           << "}";
    return stream.str();
}

void WebhookAlertHandler::handle(const Alert& alert)
{
    std::wstring wideUrl = utf8ToWide(webhookUrl);
    if (wideUrl.empty())
    {
        std::cerr << "Webhook alert skipped: invalid webhook URL.\n";
        throw std::runtime_error("Webhook alert failed: invalid webhook URL.");
    }

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(
            wideUrl.c_str(),
            static_cast<DWORD>(wideUrl.size()),
            0,
            &components
        ))
    {
        std::cerr << "Webhook alert skipped: could not parse webhook URL.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not parse webhook URL."
        );
    }

    std::wstring host(
        components.lpszHostName,
        components.dwHostNameLength
    );
    std::wstring path(
        components.lpszUrlPath,
        components.dwUrlPathLength
    );
    if (components.dwExtraInfoLength > 0)
    {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    if (components.nScheme != INTERNET_SCHEME_HTTPS)
    {
        std::cerr << "Webhook alert skipped: webhook URL must use HTTPS.\n";
        throw std::runtime_error("Webhook alert failed: URL must use HTTPS.");
    }

    const DWORD requestFlags = WINHTTP_FLAG_SECURE;
    const std::string body = buildRequestBody(alert);
    const std::wstring headers =
        L"Content-Type: application/json\r\n";

    WinHttpHandle session(WinHttpOpen(
        L"SystemResourceMonitor/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    ));
    if (!session)
    {
        std::cerr << "Webhook alert skipped: could not open WinHTTP session.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not open WinHTTP session."
        );
    }

    WinHttpHandle connection(WinHttpConnect(
        session.get(),
        host.c_str(),
        components.nPort,
        0
    ));
    if (!connection)
    {
        std::cerr << "Webhook alert skipped: could not connect to host.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not connect to host."
        );
    }

    WinHttpHandle request(WinHttpOpenRequest(
        connection.get(),
        L"POST",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        requestFlags
    ));
    if (!request)
    {
        std::cerr << "Webhook alert skipped: could not open HTTP request.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not open HTTP request."
        );
    }

    if (!WinHttpSetTimeouts(request.get(), 5000, 5000, 5000, 5000))
    {
        std::cerr << "Webhook alert skipped: could not configure timeouts.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not configure WinHTTP timeouts."
        );
    }

    if (!WinHttpSendRequest(
            request.get(),
            headers.c_str(),
            static_cast<DWORD>(headers.size()),
            const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()),
            0
        ))
    {
        std::cerr << "Webhook alert skipped: failed to send HTTP request.\n";
        throw std::runtime_error(
            "Webhook alert failed: could not send HTTP request."
        );
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr))
    {
        std::cerr << "Webhook alert skipped: no HTTP response received.\n";
        throw std::runtime_error(
            "Webhook alert failed: no HTTP response received."
        );
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX
        ))
    {
        throw std::runtime_error(
            "Webhook alert failed: could not read HTTP status code."
        );
    }

    if (statusCode < 200 || statusCode >= 300)
    {
        throw std::runtime_error(
            "Webhook alert failed with HTTP status " +
            std::to_string(statusCode)
        );
    }
}

EmailAlertHandler::EmailAlertHandler(std::string recipient)
    : recipient(std::move(recipient))
{
}

void EmailAlertHandler::handle(const Alert& alert)
{
    std::cerr << "Mock email alert to " << recipient
              << ": [" << severityToString(alert.severity)
              << "] " << alert.message << "\n";
}
