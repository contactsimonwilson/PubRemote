import React from 'react';
import { Upload, Check, HardDrive, Table, Cpu, Tag, X } from 'lucide-react';
import { FirmwareFiles, FirmwareVersion } from '../types';
import { useFirmware } from '../hooks/useFirmware';
import { Dropdown } from './ui/Dropdown';
import { cn } from '../utils/cn';

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
  const [isDragging, setIsDragging] = React.useState(false);
  const [isInvalid, setIsInvalid] = React.useState(false);

  const handleDragEnter = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(true);
  };

  const handleDragLeave = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);
    setIsInvalid(false);
  };

  const handleDragOver = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
  };

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);

    const droppedFile = e.dataTransfer.files[0];
    if (!droppedFile) return;

    // Check if the file type is correct
    if (!droppedFile.name.endsWith('.bin')) {
      setIsInvalid(true);
      setTimeout(() => setIsInvalid(false), 2000);
      return;
    }

    onChange(droppedFile);
  };

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = e.target.files?.[0];
    if (selectedFile) {
      onChange(selectedFile);
    }
  };

  return (
    <div
      onDragEnter={handleDragEnter}
      onDragLeave={handleDragLeave}
      onDragOver={handleDragOver}
      onDrop={handleDrop}
      className={cn(
        'flex flex-col items-center gap-4 rounded-lg p-6 transition-colors duration-200',
        file ? 'bg-gray-800/50' : 'bg-gray-800/30',
        isDragging && !isInvalid && 'bg-blue-900/20 border-2 border-dashed border-blue-500',
        isInvalid && 'bg-red-900/20 border-2 border-dashed border-red-500',
        'relative'
      )}
    >
      <input
        ref={inputRef}
        type="file"
        accept={accept}
        onChange={handleFileChange}
        className="hidden"
      />
      
      {isInvalid ? (
        <div className="absolute inset-0 flex flex-col items-center justify-center bg-gray-900/95 rounded-lg">
          <X className="h-8 w-8 text-red-500 mb-2" />
          <p className="text-sm text-red-400">Invalid file type</p>
        </div>
      ) : file ? (
        <Check className="h-8 w-8 text-blue-500" />
      ) : isDragging ? (
        <Upload className="h-8 w-8 text-blue-500" />
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

  const handleVersionSelect = async (version: string) => {
    const selectedVersion = versions.find(v => v.version === version);
    if (!selectedVersion) return;

    try {
      const [bootloaderRes, partitionRes, applicationRes] = await Promise.all([
        fetch(selectedVersion.bootloader),
        fetch(selectedVersion.partitionTable),
        fetch(selectedVersion.application),
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

  const versionOptions = versions.map(version => ({
    value: version.version,
    label: `Version ${version.version} (${new Date(version.date).toLocaleDateString()})`,
  }));

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-semibold">Select Firmware Files</h2>
        <Dropdown
          options={versionOptions}
          value={selectedVersion}
          onChange={(value) => {
            setSelectedVersion(value as string);
            handleVersionSelect(value as string);
          }}
          label={loading ? 'Loading releases...' : versions.length === 0 ? 'No releases available' : 'Select Version'}
          icon={<Tag className="h-4 w-4" />}
          disabled={loading || versions.length === 0}
        />
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