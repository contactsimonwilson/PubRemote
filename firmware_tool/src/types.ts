export interface FirmwareVariant {
  zipUrl: string;
  date: string;
  variant: string;
}

export interface DeviceInfoData {
  connected: boolean;
  chipId?: string;
  macAddress?: string;
  version?: string;
  variant?: string;
}

export interface FlashProgress {
  status: 'idle' | 'connecting' | 'erasing' | 'flashing' | 'verifying' | 'complete' | 'error';
  progress: number;
  error?: string;
}

export interface FirmwareVersion {
  version: string;
  date: string;
  variants: FirmwareVariant[];
  prerelease: boolean;
}

export interface FirmwareFiles {
  bootloader: File | null;
  partitionTable: File | null;
  application: File | null;
}