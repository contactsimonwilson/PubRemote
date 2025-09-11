import React, { useRef, useState } from "react";
import Header from "./components/Header";
import { ESPService } from "./services/espService";
import { TerminalService } from "./services/terminal";
import { DeviceInfoData, FlashProgress as FlashProgressType } from "./types";
import SettingsPage from "./components/SettingsPage";
import FirmwarePage from "./components/FirmwarePage";
import DeviceInfoPage from "./components/DeviceInfoPage";
import { DeviceToolsProvider } from "./context/DeviceToolsContext";
import { DeviceInfo } from "./components/DeviceInfo";

const App = () => {
  const terminal = useRef<TerminalService>(new TerminalService()).current;
  const espService = useRef<ESPService>(new ESPService(terminal)).current;
  const [activeTab, setActiveTab] = useState(0);
  const [flashProgress, setFlashProgress] = useState<FlashProgressType>({
    status: "idle",
    progress: 0,
  });
  const [deviceInfo, setDeviceInfo] = useState<DeviceInfoData>({
    connected: false,
  });

  const handleSendTerminalCommand = React.useCallback(
    async (command: string) => {
      try {
        await espService.sendCommand(command);
      } catch (error) {
        console.error("Command error:", error);
      }
    },
    [espService]
  );

  const handleConnect = async () => {
    try {
      setDeviceInfo({ connected: false });
      const info = await espService.connect();
      setDeviceInfo({
        ...info,
        connected: true,
      });
    } catch (error) {
      console.error("Connection error:", error);
      setDeviceInfo({ connected: false });
      throw error;
    }
  };

  const handleDisconnect = () => {
    espService.disconnect();
    setDeviceInfo({ connected: false });
  };

  const tabs = [
    {
      label: "Home",
      content: <DeviceInfoPage />,
    },
    {
      label: "Firmware",
      content: <FirmwarePage />,
    },
    {
      label: "Settings",
      content: <SettingsPage />,
    },
  ];

  return (
    <div className="min-h-screen bg-gray-950 text-gray-100">
      <Header />
      <div className="container flex justify-center mx-auto px-4 pt-2">
        <nav className="flex space-x-8" aria-label="Tabs">
          {tabs.map((tab, index) => (
            <button
              key={index}
              onClick={() => setActiveTab(index)}
              className={`py-2 px-1 border-b-2 font-medium text-sm transition-colors duration-200 ${
                activeTab === index
                  ? "border-blue-500 text-blue-600"
                  : "border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300"
              }`}
              aria-selected={activeTab === index}
            >
              {tab.label}
            </button>
          ))}
        </nav>
      </div>
      <main className="container mx-auto px-4 pb-8 pt-4">
        <div className="mx-auto max-w-4xl space-y-8">
          <DeviceToolsProvider
            terminal={terminal}
            espService={espService}
            deviceInfo={deviceInfo}
            setDeviceInfo={setDeviceInfo}
            flashProgress={flashProgress}
            setFlashProgress={setFlashProgress}
            sendTerminalCommand={handleSendTerminalCommand}
            disconnect={handleDisconnect}
          >
            <DeviceInfo
              deviceInfo={deviceInfo}
              onConnect={handleConnect}
              onDisconnect={handleDisconnect}
              onSendCommand={handleSendTerminalCommand}
              terminal={terminal}
            />
            {tabs[activeTab].content}
          </DeviceToolsProvider>
        </div>
      </main>
    </div>
  );
};

export default App;
