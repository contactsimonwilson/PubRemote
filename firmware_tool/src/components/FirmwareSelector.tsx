import React from 'react';
import { Upload, Check, HardDrive, Table, Cpu } from 'lucide-react';
import { FirmwareFiles, FirmwareVersion } from '../types';
import { useFirmware } from '../hooks/useFirmware';

interface Props {
  onSelectFirmware: (files: FirmwareFiles) => void;
  selectedFirmware: FirmwareFiles | null;
}

interface FileUploadProps {
  label: string;
  icon: React.ReactNode;
  file: File | null;
  onChange: (file: File) => void;
  accept?: string;
}

function FileUpload({
  label,
  icon,
  file,
  onChange,
  accept = '.bin',
}: FileUploadProps) {
  const inputRef = React.useRef<HTMLInputElement>(null);

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = e.target.files?.[0];
    if (selectedFile) {
      onChange(selectedFile);
    }
  };

  return (
    <div
      className={`flex flex-col items-center gap-4 rounded-lg bg-gray-800/50 p-6 ${
        file ? 'bg-gray-800' : ''
      }`}
    >
      <input
        ref={inputRef}
        type="file"
        accept={accept}
        onChange={handleFileChange}
        className="hidden"
      />
      {file ? (
        <Check className="h-8 w-8 text-blue-500" />
      ) : (
        <div className="h-8 w-8 text-gray-600">{icon}</div>
      )}
      <div className="text-center">
        {file ? (
          <>
            <p className="text-sm text-gray-200">{file.name}</p>
            <button
              onClick={() => inputRef.current?.click()}
              className="text-blue-500 hover:text-blue-400"
            >
              Choose different file
            </button>
          </>
        ) : (
          <>
            <p className="text-sm text-gray-400">{label}</p>
            <button
              onClick={() => inputRef.current?.click()}
              className="text-blue-500 hover:text-blue-400"
            >
              Select file
            </button>
          </>
        )}
      </div>
    </div>
  );
}

export function FirmwareSelector({
  onSelectFirmware,
  selectedFirmware,
}: Props) {
  const { versions, loading, error } = useFirmware();
  const [files, setFiles] = React.useState<FirmwareFiles>({
    bootloader: null,
    partitionTable: null,
    application: null,
  });
  const [selectedVersion, setSelectedVersion] = React.useState<string>('');

  React.useEffect(() => {
    if (versions.length > 0 && !selectedVersion) {
      setSelectedVersion(versions[0].version);
    }
  }, [versions]);

  const handleFileChange = (type: keyof FirmwareFiles) => (file: File) => {
    const newFiles = { ...files, [type]: file };
    setFiles(newFiles);
    onSelectFirmware(newFiles);
  };

  const handleVersionSelect = async (version: FirmwareVersion) => {
    try {
      const [bootloaderRes, partitionRes, applicationRes] = await Promise.all([
        fetch(version.bootloader),
        fetch(version.partitionTable),
        fetch(version.application),
      ]);

      const [bootloaderBlob, partitionBlob, applicationBlob] =
        await Promise.all([
          bootloaderRes.blob(),
          partitionRes.blob(),
          applicationRes.blob(),
        ]);

      const newFiles = {
        bootloader: new File([bootloaderBlob], 'bootloader.bin', {
          type: 'application/octet-stream',
        }),
        partitionTable: new File([partitionBlob], 'partitions.bin', {
          type: 'application/octet-stream',
        }),
        application: new File([applicationBlob], 'firmware.bin', {
          type: 'application/octet-stream',
        }),
      };

      setFiles(newFiles);
      onSelectFirmware(newFiles);
    } catch (err) {
      console.error('Failed to download firmware files:', err);
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-semibold">Select Firmware Files</h2>
        <select
          value={selectedVersion}
          onChange={(e) => {
            setSelectedVersion(e.target.value);
            const version = versions.find((v) => v.version === e.target.value);
            if (version) handleVersionSelect(version);
          }}
          className="bg-gray-800 rounded-lg px-3 py-2 text-sm text-white focus:outline-none focus:ring-2 focus:ring-blue-500"
          disabled={loading || versions.length === 0}
        >
          {loading ? (
            <option>Loading releases...</option>
          ) : versions.length === 0 ? (
            <option>No releases available</option>
          ) : (
            versions.map((version) => (
              <option key={version.version} value={version.version}>
                Version {version.version} (
                {new Date(version.date).toLocaleDateString()})
              </option>
            ))
          )}
        </select>
      </div>

      {error && (
        <div className="p-3 rounded-lg bg-red-900/50 text-red-200">{error}</div>
      )}

      <div className="grid gap-4 md:grid-cols-3">
        <FileUpload
          label="Select Bootloader"
          icon={<HardDrive className="h-full w-full" />}
          file={files.bootloader}
          onChange={handleFileChange('bootloader')}
        />

        <FileUpload
          label="Select Partition Table"
          icon={<Table className="h-full w-full" />}
          file={files.partitionTable}
          onChange={handleFileChange('partitionTable')}
        />

        <FileUpload
          label="Select Application"
          icon={<Cpu className="h-full w-full" />}
          file={files.application}
          onChange={handleFileChange('application')}
        />
      </div>
    </div>
  );
}
