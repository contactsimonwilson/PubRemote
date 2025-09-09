import React from "react";
import { Eraser, RefreshCcw, Save } from "lucide-react";
import useDeviceTools from "../hooks/useDeviceTools";
import { LogListener } from "../services/espService";

const SettingsPage: React.FC<unknown> = () => {
  const { deviceInfo, flashProgress, sendTerminalCommand, espService } =
    useDeviceTools();

  const settingsLoading = React.useRef<boolean>(false);
  const [showWifiPassword, setshowWifiPassword] =
    React.useState<boolean>(false);
  const [settings, setSettings] = React.useState({
    wifi_ssid: "",
    wifi_password: "",
  });

  const retrieveSettings = React.useCallback(async (): Promise<
    Record<string, string>
  > => {
    const timeout = 5000;
    let wifi_ssid = "";
    let wifi_password = "";

    return new Promise((resolve, reject) => {
      const timeoutId = setTimeout(() => {
        espService.removeLogListener(versionLogListener);
        reject(new Error("Timeout while waiting for settings response"));
      }, timeout);

      // Request firmware info
      espService.log("Fetching settings...");
      const versionLogListener: LogListener = (data) => {
        if (data.toLocaleLowerCase().startsWith("wifi_ssid:")) {
          // regex to match variant
          wifi_ssid = data.replace(/^wifi_ssid:\s*/i, "").trim();
        }

        if (data.toLowerCase().startsWith("wifi_password:")) {
          wifi_password = data.replace(/^wifi_password:\s*/i, "").trim();
        }

        if (data === "pubconsole>") {
          clearTimeout(timeoutId);
          espService.removeLogListener(versionLogListener);
          espService.log("Settings successfully loaded");
          resolve({ wifi_ssid, wifi_password });
        }

        return true; // Mark log as handled
      };
      espService.addLogListener(versionLogListener);
      espService.sendCommand("settings");
    });
  }, [espService]);

  const fetchSettings = React.useCallback(() => {
    if (deviceInfo.connected && flashProgress.status === "idle") {
      settingsLoading.current = true;
      retrieveSettings()
        .then((settings) => {
          setSettings({
            wifi_ssid: settings.wifi_ssid || "",
            wifi_password: settings.wifi_password || "",
          });
          settingsLoading.current = false;
        })
        .catch((err) => {
          console.error("Failed to retrieve settings:", err);
          settingsLoading.current = false;
        });
    }
  }, [deviceInfo.connected, flashProgress.status, retrieveSettings]);

  React.useEffect(() => {
    fetchSettings();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deviceInfo.connected, flashProgress.status]);

  return (
    <div className="rounded-lg bg-gray-900 p-6">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold mb-4">Remote Settings</h2>
        <button
          onClick={fetchSettings}
          className="p-1 text-gray-400 hover:text-gray-200 transition-colors"
          title="Reload settings"
        >
          <RefreshCcw className="h-4 w-4" />
        </button>
      </div>
      <div className="space-y-6">
        <div>
          <h3 className="font-medium mb-2">WiFi Network Name</h3>
          <input
            type="text"
            value={settings.wifi_ssid}
            autoComplete="wifi-ssid"
            onChange={(e) =>
              setSettings((prev) => ({ ...prev, wifi_ssid: e.target.value }))
            }
            placeholder="Enter WiFi network name"
            className="w-full px-3 py-2 bg-gray-800 border border-gray-600 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent text-white"
          />
        </div>

        <div>
          <h3 className="font-medium mb-2">WiFi Password</h3>
          <div className="relative">
            <input
              type={showWifiPassword ? "text" : "password"}
              value={settings.wifi_password}
              autoComplete="wifi-key"
              onChange={(e) =>
                setSettings((prev) => ({
                  ...prev,
                  wifi_password: e.target.value,
                }))
              }
              placeholder="Enter WiFi password"
              className="w-full px-3 py-2 bg-gray-800 border border-gray-600 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent text-white pr-16"
            />
            <button
              type="button"
              onClick={() => setshowWifiPassword((prev) => !prev)}
              className="absolute right-3 top-2 text-gray-400 hover:text-white text-sm"
            >
              {showWifiPassword ? "Hide" : "Show"}
            </button>
          </div>
        </div>

        {/* <div className="flex items-center justify-between">
          <div>
            <h3 className="font-medium">Email Notifications</h3>
            <p className="text-sm text-gray-500">
              Receive email updates about your account
            </p>
          </div>
          <label className="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" className="sr-only peer" defaultChecked />
            <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
          </label>
        </div>
        <div className="flex items-center justify-between">
          <div>
            <h3 className="font-medium">Dark Mode</h3>
            <p className="text-sm text-gray-500">Switch to dark theme</p>
          </div>
          <label className="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" className="sr-only peer" />
            <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
          </label>
        </div> */}
        <div className="border-t pt-4 flex items-center justify-between gap-4">
          <button
            className="flex items-center gap-2 rounded-lg px-4 py-2 text-sm font-medium text-white transition-colors bg-red-900/50 hover:bg-red-900/75"
            disabled={!deviceInfo.connected || flashProgress.status !== "idle"}
            onClick={() => sendTerminalCommand("erase")}
          >
            <Eraser className="h-4 w-4" />
            Factory Reset
          </button>
          <button
            onClick={() => {
              sendTerminalCommand(
                `save_settings wifi_ssid ${
                  settings.wifi_ssid ?? "/0"
                } wifi_password ${settings.wifi_password ?? "/0"}`
              );
            }}
            disabled={!deviceInfo.connected || flashProgress.status !== "idle"}
            className="flex items-center gap-2 rounded-lg px-4 py-2 text-sm font-medium text-white transition-colors bg-blue-600 hover:bg-blue-700 disabled:bg-gray-600 disabled:cursor-not-allowed"
          >
            <Save className="h-4 w-4" />
            Save
          </button>
        </div>
      </div>
    </div>
  );
};

export default SettingsPage;
