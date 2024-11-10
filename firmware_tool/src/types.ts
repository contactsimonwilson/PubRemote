export interface FirmwareVariant {
  bootloader: string;
  partitionTable: string;
  application: string;
  date: string;
}

export interface DeviceInfoData {
  connected: boolean;
  chipId?: string;
  macAddress?: string;
}

export interface FlashProgress {
  status: 'idle' | 'connecting' | 'erasing' | 'flashing' | 'verifying' | 'complete' | 'error';
  progress: number;
  error?: string;
}

export interface FirmwareVersion {
  version: string;
  date: string;
  bootloader: string;
  partitionTable: string;
  application: string;
}

export interface FirmwareFiles {
  bootloader: File | null;
  partitionTable: File | null;
  application: File | null;
}