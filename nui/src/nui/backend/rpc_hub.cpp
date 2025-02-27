#include <nui/backend/rpc_hub.hpp>

#include <nui/backend/filesystem/file_dialog.hpp>
#include "rpc_addons/fetch.hpp"
#include "rpc_addons/file.hpp"
#include "rpc_addons/throttle.hpp"
#include "rpc_addons/timer.hpp"
#include "rpc_addons/screen.hpp"
#include "rpc_addons/environment_variables.hpp"

namespace Nui
{
    // #####################################################################################################################
    RpcHub::RpcHub(Window& window)
        : window_{&window}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableFileDialogs() const
    {
        registerFunction("Nui::showOpenDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            auto result = FileDialog::showOpenDialog(args.get<FileDialog::OpenDialogOptions>());
            nlohmann::json response;
            if (result)
                response = *result;
            callRemote(callbackId, response);
        });
        registerFunction("Nui::showDirectoryDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            const auto result = FileDialog::showDirectoryDialog(args.get<FileDialog::DirectoryDialogOptions>());
            nlohmann::json response;
            if (result)
                response = *result;
            callRemote(callbackId, response);
        });
        registerFunction("Nui::showSaveDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            const auto result = FileDialog::showSaveDialog(args.get<FileDialog::SaveDialogOptions>());
            nlohmann::json response;
            if (result)
                response = result->string();
            callRemote(callbackId, response);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableFile()
    {
        registerFile(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableThrottle()
    {
        registerThrottle(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableWindowFunctions() const
    {
        registerFunction("Nui::openDevTools", [this]() {
            window_->openDevTools();
        });
        registerFunction("Nui::terminate", [this]() {
            window_->terminate();
        });
        registerFunction("Nui::setWindowSize", [this](int width, int height, int hint) {
            window_->setSize(width, height, static_cast<WebViewHint>(hint));
        });
        registerFunction("Nui::setWindowTitle", [this](std::string const& title) {
            window_->setTitle(title);
        });
        registerFunction("Nui::setPosition", [this](int x, int y) {
            window_->setPosition(x, y);
        });
        registerFunction("Nui::centerOnPrimaryDisplay", [this]() {
            window_->centerOnPrimaryDisplay();
        });
        registerFunction("Nui::navigate", [this](std::string const& navTarget) {
            window_->navigate(navTarget);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableTimer()
    {
        registerTimer(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableFetch() const
    {
        registerFetch(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableScreen()
    {
        registerScreen(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableEnvironmentVariables()
    {
        registerEnvironmentVariables(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableAll()
    {
        enableFileDialogs();
        enableWindowFunctions();
        enableFetch();
        enableFile();
        enableThrottle();
        enableTimer();
        enableScreen();
        enableEnvironmentVariables();
    }
    // #####################################################################################################################
}