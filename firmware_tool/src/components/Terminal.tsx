import React, { useEffect, useRef } from 'react';
import { Terminal as TerminalIcon, Send, Trash2, Filter, Download } from 'lucide-react';
import { Dropdown } from './ui/Dropdown';
import { DeviceInfoData } from '../types';
import { LogEntry, TerminalService } from '../services/terminal';

interface Props {
  terminal: TerminalService;
  onSendCommand: (command: string) => void;
  disabled?: boolean;
  deviceInfo?: DeviceInfoData;
}

export function Terminal({ terminal, onSendCommand, disabled = false, deviceInfo }: Props) {
  const [command, setCommand] = React.useState('');
  const commandBuffer = React.useRef<string[]>([]);
  const commandBufferIndex = React.useRef<number>(0);
  const [logs, setLogs] = React.useState<LogEntry[]>([]);
  const [enabledLogTypes, setEnabledLogTypes] = React.useState<string[]>([
    'info',
    'error',
    'success',
  ]);
  const terminalRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    // Subscribe to terminal data
    const dataHandler = (data: LogEntry | null) => {
      if (data === null) {
        setLogs([]);
      } else {

        if (data) {
          setLogs(prev => [...prev, data]);
        }
      }
    };

    const remove = terminal.subscribe(dataHandler);

    return () => {
      remove();
    };
  }, [terminal]);

  useEffect(() => {
    if (terminalRef.current) {
      terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
    }
  }, [logs]);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (command.trim()) {
      const finalCommand = command.trim();
      commandBuffer.current.push(finalCommand);
      commandBufferIndex.current = commandBuffer.current.length; // Reset index to the end
      onSendCommand(finalCommand);
      setCommand('');
    }
  };

  const clearLogs = () => {
    setLogs([]);
    terminal.clear();
  };

  const downloadLogs = () => {
    const deviceInfoText = deviceInfo?.connected
      ? `Device Information:
- Chip Type: ${deviceInfo.chipId || 'N/A'}
- MAC Address: ${deviceInfo.macAddress || 'N/A'}
- Firmware Version: ${deviceInfo.version || 'N/A'}
- Firmware Variant: ${deviceInfo.variant || 'N/A'}

`
      : 'Device not connected\n\n';

    const logsText = logs
      .filter(log => enabledLogTypes.includes(log.type))
      .map(log => `[${log.timestamp}] ${log.type.toUpperCase()}: ${log.message}`)
      .join('\n');
    
    const fullText = deviceInfoText + 'Terminal Logs:\n' + logsText;
    
    const blob = new Blob([fullText], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `pubmote-${new Date().toISOString().slice(0, 19).replace(/[:]/g, '-')}.log`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  const filteredLogs = logs.filter(log => enabledLogTypes.includes(log.type));

  const logLevelOptions = [
    { value: 'info', label: 'Info', color: 'text-blue-500' },
    { value: 'error', label: 'Errors', color: 'text-red-500' },
    { value: 'success', label: 'Success', color: 'text-green-500' },
  ];

  return (
    <div className="rounded-lg bg-gray-800/50 p-4">
      <div className="flex items-center justify-between mb-2">
        <div className="flex items-center gap-2 text-gray-400">
          <TerminalIcon className="h-4 w-4" />
          <span className="text-sm font-medium">Terminal Monitor</span>
        </div>
        <div className="flex items-center gap-4">
          <Dropdown
            options={logLevelOptions}
            value={enabledLogTypes}
            onChange={(value) => setEnabledLogTypes(value as string[])}
            multiple={true}
            label="Log Levels"
            icon={<Filter className="h-4 w-4" />}
          />
          <button
            onClick={downloadLogs}
            className="p-1 text-gray-400 hover:text-gray-200 transition-colors"
            title="Download logs"
          >
            <Download className="h-4 w-4" />
          </button>
          <button
            onClick={clearLogs}
            className="p-1 text-gray-400 hover:text-gray-200 transition-colors"
            title="Clear logs"
          >
            <Trash2 className="h-4 w-4" />
          </button>
        </div>
      </div>

      <div
        ref={terminalRef}
        className="font-mono text-sm h-80 overflow-y-auto bg-gray-950 rounded p-3 text-gray-300 space-y-1"
      >
        {filteredLogs.length > 0 ? (
          filteredLogs.map((log, index) => (
            <div
              key={index}
              className={`leading-relaxed ${
                log.type === 'error'
                  ? 'text-red-400'
                  : log.type === 'success'
                  ? 'text-green-400'
                  : 'text-gray-300'
              }`}
            >
              [{log.timestamp}]{' '}
              {log.type === 'error'
                ? '❌'
                : log.type === 'success'
                ? '✅'
                : 'ℹ️'}{' '}
              {log.message}
            </div>
          ))
        ) : (
          <div className="text-gray-500 italic">
            {disabled ? 'Connect a device to see terminal output...' : 'No logs to display'}
          </div>
        )}
      </div>

      <form onSubmit={handleSubmit} className="mt-3 flex gap-2">
        <input
          type="text"
          value={command}
          onChange={(e) => setCommand(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === 'ArrowUp') {
              e.preventDefault();
              if (commandBufferIndex.current > 0) {
                commandBufferIndex.current -= 1;
                setCommand(commandBuffer.current[commandBufferIndex.current] || '');
              }
            } else if (e.key === 'ArrowDown') {
              e.preventDefault();
              if (commandBufferIndex.current < commandBuffer.current.length - 1) {
                commandBufferIndex.current += 1;
                setCommand(commandBuffer.current[commandBufferIndex.current] || '');
              }
            }
          }}
          disabled={disabled}
          placeholder={disabled ? 'Connect device to send commands...' : 'Enter command...'}
          className="flex-1 bg-gray-900 rounded-lg px-3 py-2 text-sm text-white placeholder-gray-500 focus:outline-none focus:ring-2 focus:ring-blue-500 disabled:opacity-50 disabled:cursor-not-allowed"
        />
        <button
          type="submit"
          disabled={disabled || !command.trim()}
          className="flex items-center gap-2 rounded-lg bg-blue-600 px-4 py-2 text-sm font-medium text-white transition-colors hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <Send className="h-4 w-4" />
          Send
        </button>
      </form>
    </div>
  );
}