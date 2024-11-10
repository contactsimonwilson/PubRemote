import { ESPLoader, Transport, LoaderOptions } from 'esptool-js';
import { Terminal } from 'xterm';

export class ESPService {
  private espLoader: ESPLoader | null = null;
  private terminal: Terminal;
  private isConnecting: boolean = false;

  constructor() {
    this.terminal = new Terminal({
      cols: 80,
      rows: 24,
      theme: { background: '#1a1b1e' },
    });
  }

  private log(message: string, type: 'info' | 'error' | 'success' = 'info') {
    const timestamp = new Date().toLocaleTimeString();
    const prefix = type === 'error' ? '❌' : type === 'success' ? '✅' : 'ℹ️';
    const logMessage = `[${timestamp}] ${prefix} ${message}`;
    this.terminal.writeln(logMessage);

    // Only use success logging for significant achievements
    if (type === 'success') {
      console.log(`[ESP] ${message}`);
    } else if (type === 'error') {
      console.error(`[ESP] ${message}`);
    } else {
      console.info(`[ESP] ${message}`);
    }
  }

  async connect(): Promise<{
    connected: boolean;
    chipId: string;
    macAddress: string;
  }> {
    if (this.isConnecting) {
      throw new Error('Connection already in progress');
    }

    try {
      this.isConnecting = true;
      this.log('Requesting serial port...');

      const port = await navigator.serial.requestPort();
      const transport = new Transport(port);

      this.log('Initializing connection...');
      const loaderOptions: LoaderOptions = {
        transport,
        baudrate: 115200,
        romBaudrate: 115200,
        terminal: {
          clean: () => this.terminal.clear(),
          writeLine: (data: string) => this.terminal.writeln(data),
          write: (data: string) => this.terminal.write(data),
        },
      };

      const loader = new ESPLoader(loaderOptions);
      await loader.main();
      await loader.sync();

      this.log('Detecting chip...');

      const macAddress = await loader.chip.readMac(loader);
      const chipId = await loader.chip.getChipDescription(loader);

      // this.log('Rebooting into normal mode');
      // await loader.hardReset();

      this.espLoader = loader;

      const info = {
        connected: true,
        chipId: chipId.toUpperCase(),
        macAddress: macAddress.toUpperCase(),
      };

      this.log(`Connected to ${info.chipId}`, 'success');
      return info;
    } catch (error) {
      this.log(
        `Connection failed: ${
          error instanceof Error ? error.message : 'Unknown error'
        }`,
        'error'
      );
      await this.disconnect();
      throw error;
    } finally {
      this.isConnecting = false;
    }
  }

  async flash(firmware: {
    bootloader: File;
    partitionTable: File;
    application: File;
  }): Promise<void> {
    if (!this.espLoader) {
      throw new Error('Not connected to device');
    }

    try {
      this.log('Erasing flash...');
      await this.espLoader.eraseFlash();

      const files = [
        {
          data: await this.readFileAsString(firmware.bootloader),
          address: 0x0,
          name: 'Bootloader',
        },
        {
          data: await this.readFileAsString(firmware.partitionTable),
          address: 0x8000,
          name: 'Partition Table',
        },
        {
          data: await this.readFileAsString(firmware.application),
          address: 0x10000,
          name: 'Application',
        },
      ];

      this.log('Writing firmware...');
      await this.espLoader.writeFlash({
        fileArray: files.map(({ data, address }) => ({ data, address })),
        flashSize: 'keep',
        eraseAll: false,
        compress: true,
        flashFreq: 'keep',
        flashMode: 'keep',
        reportProgress: (fileIndex: number, written: number, total: number) => {
          const percentage = Math.round((written / total) * 100);
          this.log(`Writing ${files[fileIndex].name}: ${percentage}%`);
        },
      });

      this.log('Flash complete', 'success');
      this.log('Resetting device...');
      await this.espLoader.hardReset();
      this.log('Device reset and ready', 'success');
    } catch (error) {
      this.log(
        `Flash failed: ${
          error instanceof Error ? error.message : 'Unknown error'
        }`,
        'error'
      );
      throw error;
    }
  }

  private async readFileAsString(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(reader.result as string);
      reader.onerror = () => reject(reader.error);
      reader.readAsBinaryString(file);
    });
  }

  isConnected(): boolean {
    return this.espLoader !== null;
  }

  async disconnect(): Promise<void> {
    if (this.espLoader) {
      try {
        await this.espLoader.transport.disconnect();
      } catch (error) {
        // Ignore disconnect errors
      }
      this.espLoader = null;
      this.log('Disconnected from device');
    }
  }

  getTerminal(): Terminal {
    return this.terminal;
  }
}
