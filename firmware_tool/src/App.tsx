import React, { useRef, useState } from 'react';
import { Usb } from 'lucide-react';
import { DeviceInfo } from './components/DeviceInfo';
import { FirmwareSelector } from './components/FirmwareSelector';
import { FlashProgress } from './components/FlashProgress';
import { FloatAccessoriesSelector } from './components/FloatAccessoriesSelector';
import { Header } from './components/Header';
import { ESPService } from './services/espService';
import { TerminalService } from './services/terminal';
import { DeviceInfoData, FlashProgress as FlashProgressType } from './types';

function App() {
  const terminal = useRef<TerminalService>(new TerminalService()).current;
  const espService = useRef<ESPService>(new ESPService(terminal)).current;
  const [selectedFirmware, setSelectedFirmware] = useState<
    File | string | null
  >(null);
  const [flashProgress, setFlashProgress] = useState<FlashProgressType>({
    status: 'idle',
    progress: 0,
  });
  const [deviceInfo, setDeviceInfo] = useState<DeviceInfoData>({
    connected: false,
  });
  const [eraseFlash, setEraseFlash] = useState<boolean>(false);

  const handleConnect = async () => {
    try {
      setDeviceInfo({ connected: false });
      const info = await espService.connect();
      setDeviceInfo({
        ...info,
        connected: true,
      });
    } catch (error) {
      console.error('Connection error:', error);
      setDeviceInfo({ connected: false });
      throw error;
    }
  };

  const handleDisconnect = () => {
    espService.disconnect();
    setDeviceInfo({ connected: false });
  };

  const handleSendCommand = async (command: string) => {
    try {
      await espService.sendCommand(command);
    } catch (error) {
      console.error('Command error:', error);
    }
  };

  const handleFlash = async () => {
    if (!selectedFirmware) return;

    try {
      if (!espService.isConnected()) {
        setFlashProgress({ status: 'connecting', progress: 0 });
        await handleConnect();
      }

      if (eraseFlash) {
        setFlashProgress({ status: 'erasing', progress: 20 });
      }

      const currentProgress = 20;

      await espService.flash(selectedFirmware, eraseFlash, status => {
        setFlashProgress({ status: 'flashing', progress: Math.round(currentProgress + status.progress * (100 - currentProgress)) });
      });

      setFlashProgress({ status: 'complete', progress: 100 });
      handleDisconnect();
    } catch (error) {
      console.error('Flash error:', error);
      setFlashProgress({
        status: 'error',
        progress: 0,
        error:
          error instanceof Error ? error.message : 'Unknown error occurred',
      });
    }
  };

  return (
    <div className="min-h-screen bg-gray-950 text-gray-100">
      <Header />

      <main className="container mx-auto px-4 py-8">
        <div className="mx-auto max-w-3xl space-y-8">
          <DeviceInfo
            deviceInfo={deviceInfo}
            onConnect={handleConnect}
            onDisconnect={handleDisconnect}
            onSendCommand={handleSendCommand}
            terminal={terminal}
          />

          <div className="rounded-lg bg-gray-900 p-6">
            <FirmwareSelector
              onSelectFirmware={setSelectedFirmware}
            />
          </div>

          <div className="rounded-lg bg-gray-900 p-6">
            <h2 className="mb-6 text-xl font-semibold">Flash Firmware</h2>

            <div className="flex items-center justify-between mb-6">
              <p>Finally, flash your selected firmware to the connected device</p>
            </div>

            <div className="space-y-4">
              <FlashProgress
                progress={flashProgress}
                isDeviceConnected={deviceInfo.connected}
                eraseFlash={eraseFlash}
                onEraseFlashChange={setEraseFlash}
              />

              <button
                onClick={handleFlash}
                disabled={
                  !selectedFirmware ||
                  !deviceInfo.connected ||
                  !['complete', 'idle'].includes(flashProgress.status)
                }
                className="flex w-full items-center justify-center gap-2 rounded-lg bg-blue-600 px-4 py-2 font-medium text-white transition-colors hover:bg-blue-700 disabled:cursor-not-allowed disabled:bg-gray-700"
              >
                <Usb className="h-5 w-5" />
                Flash Device
              </button>
            </div>
          </div>

          <hr/>

          <div className="rounded-lg bg-gray-900 p-6">
            <FloatAccessoriesSelector />
          </div>
        </div>
      </main>
    </div>
  );
}

export default App;