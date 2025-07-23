import React from "react";
import { Cpu, Wifi, Tag, Box, Link, Link2Off } from "lucide-react";
import { DeviceInfoData } from "../types";
import { Terminal } from "./Terminal";
import { TerminalService } from "../services/terminal";

interface Props {
  deviceInfo: DeviceInfoData;
  onConnect: () => Promise<void>;
  onDisconnect: () => void;
  onSendCommand: (command: string) => Promise<void>;
  terminal: TerminalService;
}

export function DeviceInfo({
  deviceInfo,
  onConnect,
  onDisconnect,
  onSendCommand,
  terminal,
}: Props) {
  const [isConnecting, setIsConnecting] = React.useState(false);
  const [error, setError] = React.useState<string | null>(null);

  const handleConnect = async () => {
    setIsConnecting(true);
    setError(null);
    try {
      await onConnect();
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Unknown error occurred";
      setError(errorMessage);
    } finally {
      setIsConnecting(false);
    }
  };

  const handleDisconnect = () => {
    onDisconnect();
  };

  return (
    <div className="rounded-lg bg-gray-900 p-6">
      <div className="flex items-center justify-between mb-6">
        <h2 className="text-xl font-semibold">Device Information</h2>
        {deviceInfo.connected ? (
          <button
            onClick={handleDisconnect}
            className="flex items-center gap-2 rounded-lg px-4 py-2 text-sm font-medium text-white transition-colors bg-red-900/50 hover:bg-red-900/75"
          >
            <Link2Off className="h-4 w-4" />
            Disconnect
          </button>
        ) : (
          <button
            onClick={handleConnect}
            disabled={isConnecting}
            className="flex items-center gap-2 rounded-lg px-4 py-2 text-sm font-medium text-white transition-colors bg-blue-600 hover:bg-blue-700 disabled:bg-gray-600 disabled:cursor-not-allowed"
          >
            <Link className="h-4 w-4" />
            {isConnecting ? "Connecting..." : "Connect Device"}
          </button>
        )}
      </div>

      {error && (
        <div className="mb-4 p-3 rounded-lg bg-red-900/50 text-red-200">
          {error}
        </div>
      )}

      {deviceInfo.connected ? (
        <div className="grid gap-4 md:grid-cols-2 mb-4">
          {deviceInfo.chipId && (
            <div className="flex items-center gap-3 rounded-lg bg-gray-800/50 p-4">
              <Cpu className="h-5 w-5 text-blue-500" />
              <div>
                <div className="text-sm text-gray-400">Chip Type</div>
                <div className="font-medium text-gray-200">
                  {deviceInfo.chipId}
                </div>
              </div>
            </div>
          )}

          {deviceInfo.macAddress && (
            <div className="flex items-center gap-3 rounded-lg bg-gray-800/50 p-4">
              <Wifi className="h-5 w-5 text-blue-500" />
              <div>
                <div className="text-sm text-gray-400">MAC Address</div>
                <div className="font-medium text-gray-200">
                  {deviceInfo.macAddress}
                </div>
              </div>
            </div>
          )}

          <div className="flex items-center gap-3 rounded-lg bg-gray-800/50 p-4">
            <Box className="h-5 w-5 text-blue-500" />
            <div>
              <div className="text-sm text-gray-400">Hardware</div>
              <div className="font-medium text-gray-200">
                {deviceInfo.hardware || "Unknown"}
              </div>
            </div>
          </div>

          <div className="flex items-center gap-3 rounded-lg bg-gray-800/50 p-4">
            <Tag className="h-5 w-5 text-blue-500" />
            <div>
              <div className="text-sm text-gray-400">Firmware Version</div>
              <div className="font-medium text-gray-200">
                {`${deviceInfo.version || "Unknown"} (${
                  deviceInfo.variant || "unknown"
                })`}
              </div>
            </div>
          </div>
        </div>
      ) : (
        <div className="flex items-center justify-between mb-6">
          <p>Start here by connecting your remote</p>
        </div>
      )}

      <Terminal
        terminal={terminal}
        onSendCommand={onSendCommand}
        disabled={!deviceInfo.connected}
        deviceInfo={deviceInfo}
      />
    </div>
  );
}
